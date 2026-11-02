/*
 * hal_Audio.h
 *
 * Microphone capture: the one owner of ADF1 and GPDMA1 channel 11 (DEV-99).
 *
 * GPDMA writes the ADF's output straight into the caller's ring, a circular
 * linked list of blocks. Each block completing raises one interrupt, which only
 * stamps the block with the Shimmer clock and counts it; nothing is copied at
 * interrupt level. Both interrupts sit below the RTC's sampling interrupt
 * (.ioc: GPDMA1_Channel11 at 2, ADF1 at 7, RTC at 1), so capture never delays a
 * sensor sample.
 *
 * The factory test (S3R_TEST_0026) and audio recording both capture through
 * here, one at a time.
 */

#ifndef HAL_AUDIO_H_
#define HAL_AUDIO_H_

#include <stdint.h>

#define AUDIO_CAPTURE_MAX_BLOCKS 64U /* nodes in the ring queue */
/* Block stamps kept: a power of two; each new block overwrites the oldest.
 * Enough for every block of the largest ring. */
#define AUDIO_CAPTURE_STAMPS     64U

typedef struct
{
  uint32_t decimation; /* sinc5 ratio: 24 gives 16 kHz, 64 the factory test's 6 kHz */
  int32_t gain;        /* ADF SCALE, about 3 dB a step */
  int16_t *ring;       /* blocks x blockSamples samples, written by GPDMA */
  uint32_t blockSamples; /* samples per block: one interrupt each */
  uint32_t blocks;       /* 2 to AUDIO_CAPTURE_MAX_BLOCKS */
} AudioCapture_Config;

typedef struct
{
  uint32_t overflow;     /* MDF_ERROR_ACQUISITION_OVERFLOW */
  uint32_t rsfOverrun;   /* MDF_ERROR_RSF_OVERRUN */
  uint32_t clockAbsence; /* MDF_ERROR_CLOCK_ABSENCE */
  uint32_t shortCircuit; /* MDF_ERROR_SHORT_CIRCUIT */
  uint32_t saturation;   /* MDF_ERROR_SATURATION */
  uint32_t other;
  uint32_t mask; /* every MDF_ERROR_* flag seen since the capture started */
} AudioCapture_Errors;

/* Powers the mic, initialises ADF1, links the ring and starts. Returns 0 if
 * capture is already running or the ADF would not start. */
uint8_t AudioCapture_start(const AudioCapture_Config *cfg);
void AudioCapture_stop(void);
uint8_t AudioCapture_isRunning(void);

/* Blocks completed since the start. Block n holds ring samples
 * (n % blocks) * blockSamples onward until GPDMA comes round again. */
uint32_t AudioCapture_blocksDone(void);
/* Shimmer clock (RTC_get64 ticks) when block n completed. Valid for the last
 * AUDIO_CAPTURE_STAMPS blocks only. */
uint64_t AudioCapture_blockEndTicks(uint32_t block);
void AudioCapture_errors(AudioCapture_Errors *out);
/* Called from the DMA interrupt after each block is stamped, e.g. to wake the
 * main loop. NULL for none. */
void AudioCapture_setBlockCallback(void (*cb)(void));
/* Before the CPU reads samples GPDMA wrote. */
void AudioCapture_prepareRead(const int16_t *samples, uint32_t count);

/* S3R_TEST_0026. Returns FACTORY_TEST_MIC_PASS, a FACTORY_TEST_MIC_FAIL_* code
 * or the MDF_ERROR_* flags of a failed capture (mdf.h). */
uint8_t AudioCapture_factoryTest(void);

/* For mdf.c's HAL callbacks only */
void AudioCapture_onDmaBlock(void);
void AudioCapture_onAdfError(void);

#endif /* HAL_AUDIO_H_ */
