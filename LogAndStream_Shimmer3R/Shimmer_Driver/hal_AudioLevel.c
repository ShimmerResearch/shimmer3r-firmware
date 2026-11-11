/*
 * hal_AudioLevel.c
 *
 * The sound-level channel (DEV-1413). See hal_AudioLevel.h.
 *
 * The capture's DMA interrupt sets TASK_AUDIO_LEVEL for each 128 ms block.
 * The task runs the blocks that have completed since its last pass through
 * the A-weighting filter, in order, so the filter's state carries from one to
 * the next, and keeps the newest block's level for the packets. The filter
 * runs on the mic's start-up blocks too, so it has settled by the first level
 * it reports. If the task has fallen more than LEVEL_MAX_BEHIND blocks behind
 * (a stall in the main loop), it restarts the filter on the block before the
 * newest rather than work through old audio.
 *
 * A level describes the block that completed last: the 128 ms that ended up to
 * one block before the packet, plus however long the task waited.
 */

#include "hal_AudioLevel.h"

#include "hal_Audio.h"
#include "hal_AudioRecorder.h"
#include "log_and_stream_externs.h"
#include "log_and_stream_includes.h"

#define LEVEL_MAX_BEHIND 4U

static ShimAudioLevel_Filter filt;
static volatile int16_t level = AUDIO_LEVEL_NONE;
static uint8_t active;
static uint8_t primed; /* the filter has run on a block since it was reset */
static uint32_t next;  /* the next capture block to run through it */

int16_t AudioLevel_get(void)
{
  return level;
}

void PlatAudio_levelStart(void)
{
  level = AUDIO_LEVEL_NONE;
  ShimAudioLevel_reset(&filt);
  primed = 0;
  if (!active && AudioRecorder_captureAcquire(AUDIO_OWNER_LEVEL))
  {
    active = 1;
    next = AudioCapture_blocksDone();
  }
}

void PlatAudio_levelStop(void)
{
  if (active)
  {
    AudioRecorder_captureRelease(AUDIO_OWNER_LEVEL);
    active = 0;
  }
  level = AUDIO_LEVEL_NONE;
}

void PlatAudio_levelProcess(void)
{
  uint32_t done;
  int16_t offset;

  if (!active)
  {
    return;
  }
  done = AudioCapture_blocksDone();
  if (done - next > LEVEL_MAX_BEHIND)
  {
    ShimAudioLevel_reset(&filt);
    primed = 0;
    next = done - 2U;
  }
  /* The capture's gain is the part's (hal_AudioRecorder.c REC_GAIN), and so is
   * the offset to dB SPL */
  offset = ShimBrd_isIm68d121PresentPerSrNumber() ? AUDIO_LEVEL_OFFSET_IM68D121 :
                                                    AUDIO_LEVEL_OFFSET_MP23DB01HP;
  while (next != done)
  {
    const int16_t *samples = AudioRecorder_captureBlock(next);
    float meanSquare;

    AudioCapture_prepareRead(samples, AUDIO_REC_BLOCK_SAMPLES);
    meanSquare = ShimAudioLevel_filter(&filt, samples, AUDIO_REC_BLOCK_SAMPLES);
    if (primed && next >= AUDIO_REC_SETTLE_BLOCKS)
    {
      level = ShimAudioLevel_centiDb(meanSquare, offset);
    }
    primed = 1;
    next++;
  }
}
