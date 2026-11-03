/*
 * hal_AudioRecorder.c
 *
 * Audio recording while logging to SD (DEV-99). See hal_AudioRecorder.h.
 *
 * The capture (hal_Audio.c) fills an 8 s ring by DMA, one 128 ms block per
 * interrupt, and the interrupt sets TASK_AUDIO. The task does the rest a few
 * milliseconds at a time, giving way as soon as any other task is pending:
 * - raw audio (modes 2 and 3): it moves the ring to the open WAV in 2 KB
 *   writes;
 * - the fingerprint (modes 1 and 2): it copies the ring into a 5 s chunk
 *   buffer, analyses each full chunk one af_step() unit at a time, and writes
 *   the chunk's landmarks to the open .AFP. The next chunk starts 4 s into
 *   this one: its first second is carried over, and the ring holds what
 *   arrives while a chunk is analysed.
 * The data file reports its lifecycle through the companion fan-out
 * (SDCard/shimmer_sd_companion.h): session start, split and the one-minute
 * sync are only queued here, and stop closes at once.
 *
 * Every WAV is one file whose sample n is ring sample fileFirst + n: a stretch
 * the card was too slow for is written as silence, so time and sample index
 * stay in step. Every chunk of the fingerprint is numbered from the session's
 * first sample, and if analysis falls so far behind that the ring would lose a
 * chunk's audio, that chunk is skipped and counted. Both files' headers
 * (Audio/shimmer_audio_file.h) carry the Shimmer clock, from the capture's
 * per-block stamps, which lets an importer correct the audio clock's drift (a
 * few tens of ppm against the 32 kHz crystal).
 */

#include "hal_AudioRecorder.h"

#include <stdlib.h>
#include <string.h>

#include "audio_fingerprint.h"
#include "ff.h"
#include "hal_Audio.h"
#include "log_and_stream_externs.h"
#include "log_and_stream_includes.h"
#include "shimmer_definitions.h"
#include "version.h"

#define REC_DECIMATION \
  24U /* sinc5 x reshape 4: 16 kHz from the 1.536 MHz PDM clock */
#define REC_GAIN \
  6 /* ADF SCALE, the bench's; to be calibrated against a sound level */
#define REC_BLOCK  2048U /* samples per DMA node: 128 ms, one interrupt each */
#define REC_BLOCKS AUDIO_CAPTURE_MAX_BLOCKS
#define REC_RING   (REC_BLOCK * REC_BLOCKS) /* 8.2 s */
#define REC_SKIP_BLOCKS \
  2U /* the mic's start-up and the filters' settling: 256 ms dropped */
#define REC_WRITE \
  1024U /* samples per f_write: 2 KB, whole sectors, one short job */
#define REC_URGENT        (REC_RING / 2U)
#define REC_JOB_BUDGET_MS 2U /* most one pass of the task spends working */
#define REC_TICKS_PER_S   32768U

#define FP_CHUNK          AUDIO_FP_CHUNK_SAMPLES
#define FP_HOP            AUDIO_FP_CHUNK_HOP_SAMPLES
#define FP_CARRY          (FP_CHUNK - FP_HOP) /* the overlap: 1 s */
/* How far behind the fingerprint may fall before it skips: two blocks short
 * of the ring, so GPDMA never overwrites a sample it has still to copy */
#define FP_BEHIND         (REC_RING - 2U * REC_BLOCK)
#define FP_OUT            2048U /* bytes per .AFP write */

#if (REC_RING & (REC_RING - 1U)) != 0U || (REC_BLOCK % REC_WRITE) != 0U
#error "the ring must be a power of two and a block a whole number of writes"
#endif
#if AUDIO_CAPTURE_STAMPS < REC_BLOCKS
#error "the capture must keep a stamp for every block in the ring"
#endif
#if !defined(AF_FFT_CMSIS) || AF_MAX_FRAMES != 313 || AF_N_HOP != AUDIO_FP_FRAME_HOP_SAMPLES
#error "the fingerprint is built for 5 s chunks with CMSIS-DSP (.cproject)"
#endif
#if FP_CHUNK > FP_BEHIND || AUDIO_FP_FILE_HEADER_LEN != AUDIO_FILE_HEADER_LEN
#error "a chunk must fit in the ring, and both headers in one buffer"
#endif

enum
{
  FP_FILL,    /* copying the ring into the chunk buffer */
  FP_ANALYSE, /* af_step() on the full chunk */
  FP_EMIT     /* writing its landmarks */
};

static int16_t ring[REC_RING] __attribute__((aligned(32)));
static int16_t silence[REC_WRITE] __attribute__((aligned(32)));
static uint8_t header[AUDIO_FILE_HEADER_LEN] __attribute__((aligned(32)));
static FIL wavFile;
static char wavPath[96];
static ShimAudioFileInfo wavInfo;

/* The fingerprint. The workspace's spectrogram can be inverted back to
 * speech: it is never logged, stored or sent anywhere. */
static af_workspace ws;
static int16_t fpChunk[FP_CHUNK] __attribute__((aligned(32)));
static uint8_t fpOut[FP_OUT] __attribute__((aligned(32)));
static FIL afpFile;
static char afpPath[96];
static ShimAudioFileInfo afpInfo;
static ShimAudioFpSummary afpSum;

/* The session */
static uint8_t state = AUDIO_STATE_OFF;
static uint8_t fault;
static uint8_t mode;
static uint8_t wantWav, wantFp;
static uint8_t wavOpen, afpOpen;
static uint8_t firstStamped;
static uint8_t openReason;
static uint8_t lastBits;
static volatile uint8_t openPending, splitPending, syncPending;
static uint64_t dataFileTs, nextDataFileTs;
static uint32_t tail; /* ring samples written (or skipped as silence), from the session's start */
static uint32_t fileFirst;      /* tail at the open WAV's first sample */
static uint32_t silencePending; /* silence still to write for an overrun */
static uint32_t sessionSilence;
static uint16_t escapeDrains;
static uint16_t writeErrors;

/* The fingerprint's progress through the session */
static uint8_t fpPhase;
static uint32_t fpTail; /* ring samples copied into fpChunk, from the session's start */
static uint32_t fpFill; /* samples in fpChunk */
static uint32_t fpIndex; /* the chunk in fpChunk: its first sample is fpIndex x FP_HOP */
static uint64_t fpTicks, fpNextTicks; /* its first sample's Shimmer clock, and the next's */
static uint32_t fpRecords; /* landmarks of the analysed chunk that fit a record */
static uint32_t emitIdx;
static uint32_t chunksDone, chunksSkipped;

/* Ring samples captured, not counting the skipped start-up blocks */
static uint32_t head(void)
{
  uint32_t blocks = AudioCapture_blocksDone();

  return blocks > REC_SKIP_BLOCKS ? (blocks - REC_SKIP_BLOCKS) * REC_BLOCK : 0U;
}

static uint32_t ringAt(uint32_t sample)
{
  return (sample + REC_SKIP_BLOCKS * REC_BLOCK) & (REC_RING - 1U);
}

/* The Shimmer clock of ring sample s, from the stamp of the block it is in.
 * Valid while that block is among the capture's last AUDIO_CAPTURE_STAMPS. */
static uint64_t sampleTicks(uint32_t s)
{
  uint32_t block = s / REC_BLOCK;
  uint32_t toEnd = (block + 1U) * REC_BLOCK - s;

  return AudioCapture_blockEndTicks(block + REC_SKIP_BLOCKS)
      - ((uint64_t) toEnd * REC_TICKS_PER_S) / AUDIO_FILE_RATE_HZ;
}

static void onBlock(void)
{
  (void) ShimTask_set(TASK_AUDIO);
}

static void setFault(uint8_t why)
{
  if (fault == AUDIO_FAULT_NONE)
  {
    fault = why;
  }
}

/* Rewrites a header in place, leaving the file position at the end */
static uint8_t rewriteHeader(FIL *f)
{
  UINT bw;
  FSIZE_t end = f_tell(f);

  if (f_lseek(f, 0) != FR_OK || f_write(f, header, AUDIO_FILE_HEADER_LEN, &bw) != FR_OK
      || bw != AUDIO_FILE_HEADER_LEN || f_lseek(f, end) != FR_OK)
  {
    writeErrors++;
    return 0;
  }
  return 1;
}

static uint8_t writeWavHeader(void)
{
  ShimAudioFile_packHeader(header, &wavInfo);
  return rewriteHeader(&wavFile);
}

static uint8_t writeAfpHeader(void)
{
  ShimAudioFile_packFpHeader(header, &afpInfo, &afpSum);
  return rewriteHeader(&afpFile);
}

/* The session's counters, as both files' closing summaries carry them */
static void summarise(ShimAudioFileInfo *info)
{
  AudioCapture_Errors e;

  AudioCapture_errors(&e);
  info->closed = 1;
  info->fault = fault;
  info->escapeDrains = escapeDrains;
  info->writeErrors = writeErrors;
  info->adfErrorEvents = e.overflow + e.rsfOverrun + e.clockAbsence
      + e.shortCircuit + e.saturation + e.other;
  info->adfSaturation = e.saturation;
  info->adfOverflow = e.overflow;
}

static void closeFiles(void)
{
  if (wavOpen)
  {
    summarise(&wavInfo);
    if (tail > fileFirst && head() >= tail)
    {
      wavInfo.lastSampleTicks = sampleTicks(tail - 1U);
    }
    (void) writeWavHeader();
    if (f_close(&wavFile) != FR_OK)
    {
      setFault(AUDIO_FAULT_WRITE);
    }
    wavOpen = 0;
  }
  if (afpOpen)
  {
    summarise(&afpInfo);
    afpSum.chunksSkipped = chunksSkipped;
    (void) writeAfpHeader();
    if (f_close(&afpFile) != FR_OK)
    {
      setFault(AUDIO_FAULT_WRITE);
    }
    afpOpen = 0;
  }
}

/* Recording is over for this session; logging carries on */
static void failSession(uint8_t why)
{
  setFault(why);
  AudioCapture_stop();
  closeFiles();
  openPending = splitPending = syncPending = 0;
  state = AUDIO_STATE_FAULT;
}

static uint16_t dataFileIndex(void)
{
  const char *name = ShimSdDataFile_getDataFileName();
  const char *slash = strrchr(name, '/');

  return (uint16_t) atoi(slash != NULL ? slash + 1 : name);
}

/* The fields both headers share, for the data file now open */
static void identify(ShimAudioFileInfo *info)
{
  memset(info, 0, sizeof(*info));
  info->hwId = DEVICE_VER;
  info->mode = mode;
  info->openReason = openReason;
  info->fileIndex = dataFileIndex();
  info->dataFileTs = dataFileTs;
  info->configTime = ShimConfig_configTimeGet();
  info->fwId = FW_IDENTIFIER;
  info->fwMajor = FW_VERSION_MAJOR;
  info->fwMinor = FW_VERSION_MINOR;
  info->fwInternal = FW_VERSION_PATCH;
  memcpy(info->mac, ShimBt_macIdBytesPtrGet(), sizeof(info->mac));
  info->decimation = REC_DECIMATION;
  info->gain = REC_GAIN;
}

/* Creates one companion file and writes its first header. Returns 0 and
 * fails the session if it cannot. */
static uint8_t createFile(FIL *f, char *path, const char *ext)
{
  UINT bw;

  if (!ShimSdCompanion_path("audio", ext, path, 96U)
      || (openReason == AUDIO_FILE_OPEN_SESSION && !ShimSdCompanion_makeDirs(path))
      || f_open(f, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
  {
    failSession(AUDIO_FAULT_OPEN);
    return 0;
  }
  if (f_write(f, header, AUDIO_FILE_HEADER_LEN, &bw) != FR_OK || bw != AUDIO_FILE_HEADER_LEN)
  {
    (void) f_close(f);
    writeErrors++;
    failSession(AUDIO_FAULT_WRITE);
    return 0;
  }
  return 1;
}

static void openFiles(void)
{
  openPending = 0;
  if (wantWav)
  {
    identify(&wavInfo);
    fileFirst = tail;
    firstStamped = 0;
    ShimAudioFile_packHeader(header, &wavInfo);
    if (!createFile(&wavFile, wavPath, ".WAV"))
    {
      return;
    }
    wavOpen = 1;
  }
  if (wantFp)
  {
    identify(&afpInfo);
    memset(&afpSum, 0, sizeof(afpSum));
    ShimAudioFile_packFpHeader(header, &afpInfo, &afpSum);
    if (!createFile(&afpFile, afpPath, ".AFP"))
    {
      return;
    }
    afpOpen = 1;
  }

  if (openReason == AUDIO_FILE_OPEN_SESSION)
  {
    const AudioCapture_Config cfg = { .decimation = REC_DECIMATION,
      .gain = REC_GAIN,
      .ring = ring,
      .blockSamples = REC_BLOCK,
      .blocks = REC_BLOCKS };

    AudioCapture_setBlockCallback(onBlock);
    if (!AudioCapture_start(&cfg))
    {
      failSession(AUDIO_FAULT_CAPTURE);
      return;
    }
  }
  state = AUDIO_STATE_RECORDING;
}

/* One write to the WAV: silence owed for an overrun, else REC_WRITE ring
 * samples, else, when draining, whatever is left. Returns 1 if it wrote. */
static uint8_t writeJob(uint8_t drain)
{
  uint32_t available = head() - tail;
  uint32_t n;
  uint32_t first = tail;
  const int16_t *src;
  UINT bw;

  if (silencePending == 0U && available > REC_RING - REC_BLOCK)
  {
    /* The card fell further behind than the ring holds: GPDMA is overwriting
     * what has not been written. Skip to two blocks inside the ring and owe
     * the skipped samples as silence, so the file keeps time. */
    uint32_t lost = available - (REC_RING - 2U * REC_BLOCK);

    lost = (lost + REC_WRITE - 1U) / REC_WRITE * REC_WRITE;
    tail += lost;
    silencePending += lost;
    sessionSilence += lost;
    setFault(AUDIO_FAULT_OVERRUN);
    available = head() - tail;
  }

  if (silencePending != 0U)
  {
    n = silencePending < REC_WRITE ? silencePending : REC_WRITE;
    src = silence;
    silencePending -= n;
    wavInfo.silenceSamples += n;
  }
  else if (available >= REC_WRITE || (drain && available != 0U))
  {
    n = available < REC_WRITE ? available : REC_WRITE;
    src = &ring[ringAt(tail)];
    AudioCapture_prepareRead(src, n);
    tail += n;
  }
  else
  {
    return 0;
  }

  if (f_write(&wavFile, src, n * sizeof(int16_t), &bw) != FR_OK || bw != n * sizeof(int16_t))
  {
    writeErrors++;
    failSession(AUDIO_FAULT_WRITE);
    return 0;
  }
  wavInfo.samples += n;

  if (!firstStamped && src != silence)
  {
    /* The first ring sample in this file is now written, so its block is in
     * and stamped. Put the stamp in the header at once, so a file cut short
     * by a flat battery still says when it started. */
    wavInfo.firstSampleTicks = sampleTicks(first)
        - ((uint64_t) (first - fileFirst) * REC_TICKS_PER_S) / AUDIO_FILE_RATE_HZ;
    firstStamped = 1;
    (void) writeWavHeader();
  }
  return 1;
}

/* The chunk in fpChunk is done with: carry its last second over as the start
 * of the next */
static void fpNextChunk(void)
{
  memmove(fpChunk, &fpChunk[FP_HOP], FP_CARRY * sizeof(int16_t));
  fpFill = FP_CARRY;
  fpIndex++;
  fpTicks = fpNextTicks;
  fpPhase = FP_FILL;
}

/* One copy from the ring into the chunk buffer, at most a block. Returns 1 if
 * it copied. */
static uint8_t fpFeedJob(void)
{
  uint32_t h = head();
  uint32_t n, at, next;

  if (h - fpTail > FP_BEHIND)
  {
    /* Analysis fell so far behind that the ring is about to lose audio this
     * chunk needs. Drop it, and any after it that has gone the same way, and
     * start afresh at the first chunk still half a ring clear. */
    uint32_t restart = (h - REC_RING / 2U + FP_HOP - 1U) / FP_HOP;

    chunksSkipped += restart - fpIndex;
    setFault(AUDIO_FAULT_FP_BEHIND);
    fpIndex = restart;
    fpTail = restart * FP_HOP;
    fpFill = 0U;
  }
  if (h <= fpTail)
  {
    return 0;
  }
  at = ringAt(fpTail);
  n = h - fpTail;
  n = n < REC_BLOCK ? n : REC_BLOCK;
  n = n < REC_RING - at ? n : REC_RING - at;
  n = n < FP_CHUNK - fpFill ? n : FP_CHUNK - fpFill;

  /* Stamp this chunk's first sample and the next chunk's, 4 s in, as they
   * pass: they are recent now, and their block stamps still kept */
  if (fpFill == 0U)
  {
    fpTicks = sampleTicks(fpTail);
  }
  next = (fpIndex + 1U) * FP_HOP;
  if (next >= fpTail && next < fpTail + n)
  {
    fpNextTicks = sampleTicks(next);
  }

  AudioCapture_prepareRead(&ring[at], n);
  memcpy(&fpChunk[fpFill], &ring[at], n * sizeof(int16_t));
  fpTail += n;
  fpFill += n;
  if (fpFill == FP_CHUNK)
  {
    if (af_begin_i16(&ws, fpChunk, FP_CHUNK) == AF_BUSY)
    {
      fpPhase = FP_ANALYSE;
    }
    else
    {
      chunksSkipped++; /* never expected: the length is fixed */
      fpNextChunk();
    }
  }
  return 1;
}

/* One af_step() unit: a few milliseconds at most */
static void fpAnalyseJob(void)
{
  af_status st = af_step(&ws, 1U);
  uint32_t i;

  if (st == AF_DONE)
  {
    /* Every landmark fits a record (t1 < 313, dt < 63), but the chunk header
     * gives the count first, so count what packs before writing any */
    fpRecords = 0U;
    for (i = 0U; i < ws.n_landmarks; i++)
    {
      fpRecords += ws.landmarks[i].t1 <= 0x1FFU && ws.landmarks[i].dt <= 0x3FU;
    }
    emitIdx = 0U;
    fpPhase = FP_EMIT;
  }
  else if (st != AF_BUSY)
  {
    chunksSkipped++;
    fpNextChunk();
  }
}

/* One write of the analysed chunk's records, FP_OUT bytes at most: its chunk
 * header first. Returns 0 if a write failed. */
static uint8_t fpEmitJob(void)
{
  uint32_t len = 0U;
  UINT bw;

  if (emitIdx == 0U)
  {
    ShimAudioFile_packFpChunkHeader(fpOut, fpIndex, fpTicks, (uint16_t) fpRecords);
    len = AUDIO_FP_CHUNK_HEADER_LEN;
  }
  while (emitIdx < ws.n_landmarks && len + AUDIO_FP_RECORD_LEN <= FP_OUT)
  {
    const af_landmark *lm = &ws.landmarks[emitIdx++];

    if (ShimAudioFile_packFpRecord(&fpOut[len], lm->t1, lm->f1, lm->f2, lm->dt))
    {
      len += AUDIO_FP_RECORD_LEN;
    }
  }
  if (len != 0U && (f_write(&afpFile, fpOut, len, &bw) != FR_OK || bw != len))
  {
    writeErrors++;
    failSession(AUDIO_FAULT_WRITE);
    return 0;
  }
  if (emitIdx >= ws.n_landmarks)
  {
    afpSum.chunks++;
    afpSum.records += fpRecords;
    afpSum.lastChunkTicks = fpTicks;
    chunksDone++;
    if (afpSum.chunks == 1U)
    {
      /* As for the WAV: say when the file starts as soon as it does */
      afpInfo.firstSampleTicks = fpTicks;
      (void) writeAfpHeader();
    }
    fpNextChunk();
  }
  return 1;
}

/* One job of the fingerprint's. Returns 1 if it did one. */
static uint8_t fpJob(void)
{
  switch (fpPhase)
  {
  case FP_FILL:
    return fpFeedJob();
  case FP_ANALYSE:
    fpAnalyseJob();
    return 1;
  default:
    return fpEmitJob();
  }
}

static uint8_t fpHasWork(void)
{
  return wantFp && afpOpen && (fpPhase != FP_FILL || head() > fpTail);
}

static void notifyIfChanged(void)
{
  uint8_t bits = PlatAudio_statusBits();

  if (bits != lastBits)
  {
    lastBits = bits;
    ShimBt_instreamStatusRespSend();
  }
}

static uint8_t moreToDo(void)
{
  return openPending || splitPending || syncPending
      || (state == AUDIO_STATE_RECORDING
          && ((wavOpen && (silencePending != 0U || head() - tail >= REC_WRITE))
              || fpHasWork()));
}

void AudioRecorder_init(void)
{
  af_init(&ws);
}

void PlatAudio_sessionStart(uint8_t newMode, uint64_t newDataFileTs)
{
  if (state == AUDIO_STATE_STARTING || state == AUDIO_STATE_RECORDING)
  {
    return;
  }
  mode = newMode;
  wantWav = (mode == AUDIO_MODE_FINGERPRINT_RAW) || (mode == AUDIO_MODE_RAW);
  wantFp = (mode == AUDIO_MODE_FINGERPRINT) || (mode == AUDIO_MODE_FINGERPRINT_RAW);
  dataFileTs = newDataFileTs;
  openReason = AUDIO_FILE_OPEN_SESSION;
  fault = AUDIO_FAULT_NONE;
  tail = fileFirst = 0U;
  silencePending = sessionSilence = 0U;
  escapeDrains = writeErrors = 0U;
  fpPhase = FP_FILL;
  fpTail = fpFill = fpIndex = 0U;
  chunksDone = chunksSkipped = 0U;
  splitPending = syncPending = 0;
  state = AUDIO_STATE_STARTING;
  openPending = 1;
  (void) ShimTask_set(TASK_AUDIO);
}

void PlatAudio_fileSplit(uint64_t newDataFileTs)
{
  if (state == AUDIO_STATE_RECORDING)
  {
    nextDataFileTs = newDataFileTs;
    splitPending = 1;
    (void) ShimTask_set(TASK_AUDIO);
  }
}

void PlatAudio_syncTick(void)
{
  if (wavOpen || afpOpen)
  {
    syncPending = 1;
    (void) ShimTask_set(TASK_AUDIO);
  }
}

void PlatAudio_sessionStop(void)
{
  if (state == AUDIO_STATE_OFF || state == AUDIO_STATE_STOPPED)
  {
    return;
  }
  AudioCapture_stop();
  if (state == AUDIO_STATE_RECORDING)
  {
    /* Everything captured goes in before the card can go to a host. A chunk
     * already analysed is written; one still filling or being analysed is
     * dropped, rather than hold the dock up for most of a second. */
    while (wavOpen && writeJob(1))
    {
    }
    while (afpOpen && fpPhase == FP_EMIT && fpEmitJob())
    {
    }
  }
  closeFiles();
  openPending = splitPending = syncPending = 0;
  if (state != AUDIO_STATE_FAULT)
  {
    state = AUDIO_STATE_STOPPED;
  }
  notifyIfChanged();
}

void PlatAudio_process(void)
{
  uint32_t start = HAL_GetTick();
  uint8_t did;

  if (openPending)
  {
    openFiles();
  }
  else if (splitPending && fpPhase != FP_EMIT)
  {
    /* A chunk's records all go in one file: a split waits for the chunk being
     * written to finish */
    splitPending = 0;
    closeFiles();
    openReason = AUDIO_FILE_OPEN_SPLIT;
    dataFileTs = nextDataFileTs;
    openFiles();
  }
  else if (state == AUDIO_STATE_RECORDING)
  {
    do
    {
      did = wavOpen ? writeJob(0) : 0U;
      if (state == AUDIO_STATE_RECORDING && fpHasWork())
      {
        did |= fpJob();
      }
    } while (did && state == AUDIO_STATE_RECORDING && ShimTask_getList() == 0U
        && HAL_GetTick() - start < REC_JOB_BUDGET_MS);

    if (syncPending && state == AUDIO_STATE_RECORDING && ShimTask_getList() == 0U)
    {
      syncPending = 0;
      if ((wavOpen && f_sync(&wavFile) != FR_OK) || (afpOpen && f_sync(&afpFile) != FR_OK))
      {
        writeErrors++;
        failSession(AUDIO_FAULT_WRITE);
      }
    }
  }
  notifyIfChanged();
  if (moreToDo())
  {
    (void) ShimTask_set(TASK_AUDIO);
  }
}

void AudioRecorder_drainIfUrgent(void)
{
  if (state == AUDIO_STATE_RECORDING && wavOpen && head() - tail >= REC_URGENT)
  {
    escapeDrains++;
    (void) writeJob(0);
  }
}

uint8_t PlatAudio_statusBits(void)
{
  return (state == AUDIO_STATE_RECORDING ? AUDIO_STATUS_RECORDING : 0U)
      | (fault != AUDIO_FAULT_NONE ? AUDIO_STATUS_FAULT : 0U);
}

static void put16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t) v;
  p[1] = (uint8_t) (v >> 8);
}

static void put32(uint8_t *p, uint32_t v)
{
  put16(p, (uint16_t) v);
  put16(p + 2, (uint16_t) (v >> 16));
}

/* AUDIO_STATUS_RESPONSE, AUDIO_STATUS_LEN bytes after the opcode,
 * little-endian: 0 u8 version  1 u8 mode (configured)  2 u8 state  3 u8 fault
 *  4 u32 seconds recorded this session  8 u32 silence samples (overruns)
 * 12 u16 escape drains  14 u16 write errors
 * 16 u32 ADF error events  20 u32 ADF saturation events
 * 24 u32 fingerprint chunks written  28 u32 fingerprint chunks skipped */
uint8_t PlatAudio_buildStatus(uint8_t *buf)
{
  AudioCapture_Errors e;

  AudioCapture_errors(&e);
  buf[0] = 1U;
  buf[1] = ShimConfig_audioMode();
  buf[2] = state;
  buf[3] = fault;
  put32(buf + 4, (wantWav ? tail : fpTail) / AUDIO_FILE_RATE_HZ);
  put32(buf + 8, sessionSilence);
  put16(buf + 12, escapeDrains);
  put16(buf + 14, writeErrors);
  put32(buf + 16,
      e.overflow + e.rsfOverrun + e.clockAbsence + e.shortCircuit + e.saturation + e.other);
  put32(buf + 20, e.saturation);
  put32(buf + 24, chunksDone);
  put32(buf + 28, chunksSkipped);
  return AUDIO_STATUS_LEN;
}
