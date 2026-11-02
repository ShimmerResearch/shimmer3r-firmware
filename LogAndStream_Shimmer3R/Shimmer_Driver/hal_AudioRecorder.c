/*
 * hal_AudioRecorder.c
 *
 * Audio recording while logging to SD (DEV-99). See hal_AudioRecorder.h.
 *
 * The capture (hal_Audio.c) fills an 8 s ring by DMA, one 128 ms block per
 * interrupt, and the interrupt sets TASK_AUDIO. The task moves the ring to the
 * open WAV in 2 KB writes, a few milliseconds at a time, giving way as soon as
 * any other task is pending. The data file reports its lifecycle through the
 * companion fan-out (SDCard/shimmer_sd_companion.h): session start, split and
 * the one-minute sync are only queued here, and stop closes at once.
 *
 * Every audio file is one WAV whose sample n is ring sample fileFirst + n: a
 * stretch the card was too slow for is written as silence, so time and sample
 * index stay in step. Its header (Audio/shimmer_audio_file.h) carries the
 * Shimmer clock of its first and last samples, from the capture's per-block
 * stamps, which lets an importer correct the audio clock's drift (a few tens
 * of ppm against the 32 kHz crystal) file by file.
 */

#include "hal_AudioRecorder.h"

#include <stdlib.h>
#include <string.h>

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
#define REC_JOB_BUDGET_MS 2U /* most one pass of the task spends writing */
#define REC_TICKS_PER_S   32768U

#if (REC_RING & (REC_RING - 1U)) != 0U || (REC_BLOCK % REC_WRITE) != 0U
#error "the ring must be a power of two and a block a whole number of writes"
#endif
#if AUDIO_CAPTURE_STAMPS < REC_BLOCKS
#error "the capture must keep a stamp for every block in the ring"
#endif

static int16_t ring[REC_RING] __attribute__((aligned(32)));
static int16_t silence[REC_WRITE] __attribute__((aligned(32)));
static uint8_t header[AUDIO_FILE_HEADER_LEN] __attribute__((aligned(32)));
static FIL wavFile;
static char wavPath[96];
static ShimAudioFileInfo info;

/* The session */
static uint8_t state = AUDIO_STATE_OFF;
static uint8_t fault;
static uint8_t mode;
static uint8_t fileOpen;
static uint8_t firstStamped;
static uint8_t openReason;
static uint8_t lastBits;
static volatile uint8_t openPending, splitPending, syncPending;
static uint64_t dataFileTs, nextDataFileTs;
static uint32_t tail; /* ring samples written (or skipped as silence), from the session's start */
static uint32_t fileFirst;      /* tail at the open file's first sample */
static uint32_t silencePending; /* silence still to write for an overrun */
static uint32_t sessionSilence;
static uint16_t escapeDrains;
static uint16_t writeErrors;

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

static uint8_t writeHeader(void)
{
  UINT bw;
  FSIZE_t end = f_tell(&wavFile);

  ShimAudioFile_packHeader(header, &info);
  if (f_lseek(&wavFile, 0) != FR_OK
      || f_write(&wavFile, header, AUDIO_FILE_HEADER_LEN, &bw) != FR_OK
      || bw != AUDIO_FILE_HEADER_LEN || f_lseek(&wavFile, end) != FR_OK)
  {
    writeErrors++;
    return 0;
  }
  return 1;
}

static void closeFile(void)
{
  AudioCapture_Errors e;

  if (!fileOpen)
  {
    return;
  }
  AudioCapture_errors(&e);
  info.closed = 1;
  info.fault = fault;
  if (tail > fileFirst && head() >= tail)
  {
    info.lastSampleTicks = sampleTicks(tail - 1U);
  }
  info.escapeDrains = escapeDrains;
  info.writeErrors = writeErrors;
  info.adfErrorEvents = e.overflow + e.rsfOverrun + e.clockAbsence
      + e.shortCircuit + e.saturation + e.other;
  info.adfSaturation = e.saturation;
  info.adfOverflow = e.overflow;
  (void) writeHeader();
  if (f_close(&wavFile) != FR_OK)
  {
    setFault(AUDIO_FAULT_WRITE);
  }
  fileOpen = 0;
}

/* Recording is over for this session; logging carries on */
static void failSession(uint8_t why)
{
  setFault(why);
  AudioCapture_stop();
  if (fileOpen)
  {
    closeFile();
  }
  openPending = splitPending = syncPending = 0;
  state = AUDIO_STATE_FAULT;
}

static uint16_t dataFileIndex(void)
{
  const char *name = ShimSdDataFile_getDataFileName();
  const char *slash = strrchr(name, '/');

  return (uint16_t) atoi(slash != NULL ? slash + 1 : name);
}

static void openFile(void)
{
  const uint8_t *mac = ShimBt_macIdBytesPtrGet();
  UINT bw;

  openPending = 0;
  if (!ShimSdCompanion_path("audio", ".WAV", wavPath, sizeof(wavPath))
      || (openReason == AUDIO_FILE_OPEN_SESSION && !ShimSdCompanion_makeDirs(wavPath))
      || f_open(&wavFile, wavPath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
  {
    failSession(AUDIO_FAULT_OPEN);
    return;
  }
  fileOpen = 1;

  memset(&info, 0, sizeof(info));
  info.hwId = DEVICE_VER;
  info.mode = mode;
  info.openReason = openReason;
  info.fileIndex = dataFileIndex();
  info.dataFileTs = dataFileTs;
  info.configTime = ShimConfig_configTimeGet();
  info.fwId = FW_IDENTIFIER;
  info.fwMajor = FW_VERSION_MAJOR;
  info.fwMinor = FW_VERSION_MINOR;
  info.fwInternal = FW_VERSION_PATCH;
  memcpy(info.mac, mac, sizeof(info.mac));
  info.decimation = REC_DECIMATION;
  info.gain = REC_GAIN;
  fileFirst = tail;
  firstStamped = 0;

  ShimAudioFile_packHeader(header, &info);
  if (f_write(&wavFile, header, AUDIO_FILE_HEADER_LEN, &bw) != FR_OK || bw != AUDIO_FILE_HEADER_LEN)
  {
    writeErrors++;
    failSession(AUDIO_FAULT_WRITE);
    return;
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
    info.silenceSamples += n;
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
  info.samples += n;

  if (!firstStamped && src != silence)
  {
    /* The first ring sample in this file is now written, so its block is in
     * and stamped. Put the stamp in the header at once, so a file cut short
     * by a flat battery still says when it started. */
    info.firstSampleTicks = sampleTicks(first)
        - ((uint64_t) (first - fileFirst) * REC_TICKS_PER_S) / AUDIO_FILE_RATE_HZ;
    firstStamped = 1;
    (void) writeHeader();
  }
  return 1;
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
          && (silencePending != 0U || head() - tail >= REC_WRITE));
}

void PlatAudio_sessionStart(uint8_t newMode, uint64_t newDataFileTs)
{
  if (state == AUDIO_STATE_STARTING || state == AUDIO_STATE_RECORDING)
  {
    return;
  }
  mode = newMode;
  dataFileTs = newDataFileTs;
  openReason = AUDIO_FILE_OPEN_SESSION;
  fault = AUDIO_FAULT_NONE;
  tail = fileFirst = 0U;
  silencePending = sessionSilence = 0U;
  escapeDrains = writeErrors = 0U;
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
  if (fileOpen)
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
  if (fileOpen && state == AUDIO_STATE_RECORDING)
  {
    /* Everything captured goes in before the card can go to a host */
    while (writeJob(1))
    {
    }
  }
  closeFile();
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

  if (openPending)
  {
    openFile();
  }
  else if (splitPending && fileOpen)
  {
    splitPending = 0;
    closeFile();
    openReason = AUDIO_FILE_OPEN_SPLIT;
    dataFileTs = nextDataFileTs;
    openFile();
  }
  else if (state == AUDIO_STATE_RECORDING && fileOpen)
  {
    while (writeJob(0))
    {
      if (ShimTask_getList() != 0U || HAL_GetTick() - start >= REC_JOB_BUDGET_MS)
      {
        break;
      }
    }
    if (syncPending && fileOpen && ShimTask_getList() == 0U)
    {
      syncPending = 0;
      if (f_sync(&wavFile) != FR_OK)
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
  if (state == AUDIO_STATE_RECORDING && fileOpen && head() - tail >= REC_URGENT)
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
 * 16 u32 ADF error events  20 u32 ADF saturation events */
uint8_t PlatAudio_buildStatus(uint8_t *buf)
{
  AudioCapture_Errors e;

  AudioCapture_errors(&e);
  buf[0] = 1U;
  buf[1] = ShimConfig_audioMode();
  buf[2] = state;
  buf[3] = fault;
  put32(buf + 4, tail / AUDIO_FILE_RATE_HZ);
  put32(buf + 8, sessionSilence);
  put16(buf + 12, escapeDrains);
  put16(buf + 14, writeErrors);
  put32(buf + 16,
      e.overflow + e.rsfOverrun + e.clockAbsence + e.shortCircuit + e.saturation + e.other);
  put32(buf + 20, e.saturation);
  return AUDIO_STATUS_LEN;
}
