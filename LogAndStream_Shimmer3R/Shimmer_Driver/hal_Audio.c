/*
 * hal_Audio.c
 *
 * Microphone capture: the one owner of ADF1 and GPDMA1 channel 11 (DEV-99).
 * See hal_Audio.h.
 *
 * Why the ring is filled this way: the first audio prototypes took GPDMA's
 * half-transfer interrupts and copied each 4 KB half into a ring inside the
 * interrupt, at the RTC's own priority. That stamped 0.37% of sensor samples late
 * by up to 0.95 ms and lost single samples at 1024 Hz. With GPDMA writing into
 * the ring itself and the interrupt only stamping, 1024 Hz logging lost nothing
 * and its stamp timing matched the firmware without audio (bench, 2 Nov 2026).
 */

#include "hal_Audio.h"

#include <string.h>

#include <PCM/pcm_config.h>

#include "dcache.h"
#include "gpdma.h"
#include "log_and_stream_externs.h"
#include "log_and_stream_includes.h"
#include "mdf.h"
#include "rtc.h"

/* GPDMA links nodes by 16-bit offsets from one base address (CLBAR), so every
 * node must sit in the same 64 KB page. The 4 KB alignment keeps the array (64
 * nodes of 36 bytes) inside one. */
static DMA_NodeTypeDef nodes[AUDIO_CAPTURE_MAX_BLOCKS] __attribute__((aligned(4096)));
static DMA_QListTypeDef ringQueue;
static MDF_DmaConfigTypeDef dmaCfg;

/* The ADF's sample rate is the PDM clock over the sinc5 ratio and the reshape
 * filter's 4. The PDM clock is HAL_MDF_MspInit()'s PLL3 Q output (16 MHz HSE
 * x 24 / 25 = 15.36 MHz) over MX_ADF1_Init()'s output clock divider of 10. */
#define PDM_CLOCK_HZ   1536000U
#define RSF_DECIMATION 4U
#define TICKS_PER_S    32768U

static volatile uint8_t running;
static volatile uint32_t blocksDone;
static volatile uint64_t stamps[AUDIO_CAPTURE_STAMPS];
static uint32_t blockBytes; /* bytes per block, as GPDMA counts them (BNDT) */
static uint32_t ticksPerSampleQ16; /* Shimmer clock ticks per sample, x 65536 */
static volatile uint32_t errorEvents; /* error interrupts, whatever their flags */
static volatile AudioCapture_Errors errors;
static void (*volatile blockCallback)(void);

/* The factory test's capture, as before DEV-99: a double buffer of
 * DEFAULT_AUDIO_IN_BUFFER_SIZE samples at decimation 64 and gain -8 (DEV-1154). */
#define TEST_BLOCK_SAMPLES (DEFAULT_AUDIO_IN_BUFFER_SIZE / 2U)
#define TEST_DECIMATION    64U
#define TEST_GAIN          (-8)

/* The ring as GPDMA's circular queue: node i fills block i and raises its own
 * transfer-complete interrupt. The node settings are MX_MICQueue_Config()'s. */
static HAL_StatusTypeDef linkRing(const AudioCapture_Config *cfg)
{
  DMA_NodeConfTypeDef c;
  HAL_StatusTypeDef ret = HAL_OK;
  uint32_t i;

  /* MX_ADF1_Init() has just linked its one-node MICQueue; the ring replaces it */
  (void) HAL_DMAEx_List_UnLinkQ(&handle_GPDMA1_Channel11);
  if (ringQueue.Head != NULL)
  {
    (void) HAL_DMAEx_List_ResetQ(&ringQueue);
  }

  memset(&c, 0, sizeof(c));
  c.NodeType = DMA_GPDMA_LINEAR_NODE;
  c.Init.Request = GPDMA1_REQUEST_ADF1_FLT0;
  c.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
  c.Init.Direction = DMA_PERIPH_TO_MEMORY;
  c.Init.SrcInc = DMA_SINC_FIXED;
  c.Init.DestInc = DMA_DINC_INCREMENTED;
  c.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
  c.Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
  c.Init.SrcBurstLength = 1;
  c.Init.DestBurstLength = 1;
  c.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
  c.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER; /* an interrupt per node */
  c.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
  c.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
  c.DataHandlingConfig.DataAlignment = DMA_DATA_UNPACK;
  c.SrcAddress = ((uint32_t) &AdfHandle0.Instance->DFLTDR) + 2U; /* the top 16 bits (MsbOnly) */
  c.DataSize = cfg->blockSamples * sizeof(int16_t);
  for (i = 0U; i < cfg->blocks; i++)
  {
    c.DstAddress = (uint32_t) &cfg->ring[i * cfg->blockSamples];
    ret |= HAL_DMAEx_List_BuildNode(&c, &nodes[i]);
    ret |= HAL_DMAEx_List_InsertNode_Tail(&ringQueue, &nodes[i]);
  }
  ret |= HAL_DMAEx_List_SetCircularMode(&ringQueue);
  ret |= HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel11, &ringQueue);
  return ret;
}

uint8_t AudioCapture_start(const AudioCapture_Config *cfg)
{
  uint32_t i;

  if (running || cfg->ring == NULL || cfg->blockSamples == 0U
      || cfg->blocks < 2U || cfg->blocks > AUDIO_CAPTURE_MAX_BLOCKS)
  {
    return 0U;
  }

  MX_ADF1_Init(); /* mic power, ADF clock and pins, HAL_MDF_Init */
  if (linkRing(cfg) != HAL_OK)
  {
    MDF1_DeInit();
    return 0U;
  }
  AdfFilterConfig0.DecimationRatio = cfg->decimation;
  AdfFilterConfig0.Gain = cfg->gain;
  /* The HAL writes these into the queue's head node: block 0 */
  dmaCfg.Address = (uint32_t) &cfg->ring[0];
  dmaCfg.DataLength = cfg->blockSamples * sizeof(int16_t);
  dmaCfg.MsbOnly = ENABLE;

  blockBytes = cfg->blockSamples * sizeof(int16_t);
  ticksPerSampleQ16
      = (uint32_t) ((((uint64_t) TICKS_PER_S * cfg->decimation * RSF_DECIMATION) << 16)
          / PDM_CLOCK_HZ);
  blocksDone = 0U;
  for (i = 0U; i < AUDIO_CAPTURE_STAMPS; i++)
  {
    stamps[i] = 0U;
  }
  errorEvents = 0U;
  memset((void *) &errors, 0, sizeof(errors));
  AdfHandle0.ErrorCode = MDF_ERROR_NONE;

  running = 1U;
  if (HAL_MDF_AcqStart_DMA(&AdfHandle0, &AdfFilterConfig0, &dmaCfg) != HAL_OK)
  {
    running = 0U;
    MDF1_DeInit();
    return 0U;
  }
  /* The HAL enables half-transfer too; one interrupt per block is all this uses */
  __HAL_DMA_DISABLE_IT(&handle_GPDMA1_Channel11, DMA_IT_HT);
  if (HAL_MDF_GenerateTrgo(&AdfHandle0) != HAL_OK)
  {
    AudioCapture_stop();
    return 0U;
  }
  shimmerStatus.micSensing = 1U;
  return 1U;
}

void AudioCapture_stop(void)
{
  if (!running)
  {
    return;
  }
  running = 0U;
  (void) HAL_MDF_AcqStop_DMA(&AdfHandle0);
  MDF1_DeInit(); /* ADF clock off, mic power off */
  shimmerStatus.micSensing = 0U;
}

uint8_t AudioCapture_isRunning(void)
{
  return running;
}

uint32_t AudioCapture_blocksDone(void)
{
  return blocksDone;
}

uint64_t AudioCapture_blockEndTicks(uint32_t block)
{
  return stamps[block % AUDIO_CAPTURE_STAMPS];
}

void AudioCapture_errors(AudioCapture_Errors *out)
{
  memcpy(out, (const void *) &errors, sizeof(*out));
}

void AudioCapture_setBlockCallback(void (*cb)(void))
{
  blockCallback = cb;
}

void AudioCapture_prepareRead(const int16_t *samples, uint32_t count)
{
  /* GPDMA wrote these; make sure the CPU reads memory, not a stale line.
   * Whether DCACHE1 caches SRAM at all on the U5 is still to be confirmed
   * (RM0456); if it does not, this costs a few microseconds for nothing. */
  HAL_DCACHE_InvalidateByAddr(
      &hdcache1, (const uint32_t *) samples, count * sizeof(int16_t));
}

/* GPDMA1 channel 11, one per block: stamp and count, nothing else.
 *
 * The stamp is when the block ended, not when this interrupt ran (DEV-1588).
 * The interrupt sits below the sensors' (I2C1 and GPDMA channels 0-10 at 0, the
 * RTC at 1), so it can be taken a fraction of a millisecond late. Stamping with
 * the clock alone put a fingerprint chunk now and then up to 8 ticks late.
 *
 * By the time the interrupt runs, GPDMA has moved on into the next block, and
 * the channel's remaining byte count (GPDMA_CxBR1.BNDT, RM0456) says how far.
 * The stamp is the clock less that many samples, which leaves it less than one
 * sample (2 ticks) after the block's end, whatever held the interrupt off.
 *
 * The clock and the count are read with interrupts masked, so nothing runs
 * between them. RTC_get64() masks its register reads anyway; this adds its
 * constant-time conversion and one register read, well under a microsecond. */
void AudioCapture_onDmaBlock(void)
{
  void (*cb)(void);
  uint32_t primask, left, moved;
  uint64_t now;

  if (!running)
  {
    return;
  }
  primask = __get_PRIMASK();
  __disable_irq();
  now = RTC_get64();
  left = READ_REG(handle_GPDMA1_Channel11.Instance->CBR1) & DMA_CBR1_BNDT;
  __set_PRIMASK(primask);
  /* BNDT counts down through the block now filling, reloaded from its node when
   * the last one ended. 0 is the moment before that reload: nothing of the new
   * block has moved yet. */
  moved = (left != 0U && left < blockBytes) ? (blockBytes - left) / sizeof(int16_t) : 0U;
  stamps[blocksDone % AUDIO_CAPTURE_STAMPS]
      = now - (((uint64_t) moved * ticksPerSampleQ16 + 0x8000U) >> 16);
  blocksDone++;
  cb = blockCallback;
  if (cb != NULL)
  {
    cb();
  }
}

void AudioCapture_onAdfError(void)
{
  /* The HAL ORs flags into ErrorCode and never clears them; take and clear them
   * here so each event is counted by type */
  const uint32_t e = AdfHandle0.ErrorCode;

  AdfHandle0.ErrorCode = MDF_ERROR_NONE;
  if (!running)
  {
    return;
  }
  errorEvents++;
  errors.mask |= e;
  if (e & MDF_ERROR_ACQUISITION_OVERFLOW)
  {
    errors.overflow++;
  }
  if (e & MDF_ERROR_RSF_OVERRUN)
  {
    errors.rsfOverrun++;
  }
  if (e & MDF_ERROR_CLOCK_ABSENCE)
  {
    errors.clockAbsence++;
  }
  if (e & MDF_ERROR_SHORT_CIRCUIT)
  {
    errors.shortCircuit++;
  }
  if (e & MDF_ERROR_SATURATION)
  {
    errors.saturation++;
  }
  if (e
      & ~(MDF_ERROR_ACQUISITION_OVERFLOW | MDF_ERROR_RSF_OVERRUN | MDF_ERROR_CLOCK_ABSENCE
          | MDF_ERROR_SHORT_CIRCUIT | MDF_ERROR_SATURATION))
  {
    errors.other++;
  }
}

/* S3R_TEST_0026, with the same steps and results as before DEV-99:
 * - skip the first FACTORY_TEST_MIC_BUFFER_SKIP_COUNT blocks, up to 1 s, while
 *   the mic starts and the filters settle, then wait 100 ms more;
 * - fail with the error flags if FACTORY_TEST_MIC_ERROR_CNT_THRESHOLD error
 *   interrupts arrived after the skip;
 * - otherwise pass if any sample is non-zero. */
uint8_t AudioCapture_factoryTest(void)
{
  static int16_t testRing[2U * TEST_BLOCK_SAMPLES] __attribute__((aligned(32)));
  const AudioCapture_Config cfg = { .decimation = TEST_DECIMATION,
    .gain = TEST_GAIN,
    .ring = testRing,
    .blockSamples = TEST_BLOCK_SAMPLES,
    .blocks = 2U };
  uint32_t eventsAtSkip;
  uint32_t i;

  if (running)
  {
    return FACTORY_TEST_MIC_FAIL_BUSY; /* recording owns the mic */
  }
  memset(testRing, 0, sizeof(testRing));
  if (!AudioCapture_start(&cfg))
  {
    return (uint8_t) MDF_ERROR_DMA;
  }

  for (i = 0U; i < 100U && blocksDone < FACTORY_TEST_MIC_BUFFER_SKIP_COUNT; i++)
  {
    HAL_Delay(10);
  }
  eventsAtSkip = errorEvents;
  HAL_Delay(100);
  AudioCapture_stop();

  if (errorEvents - eventsAtSkip >= FACTORY_TEST_MIC_ERROR_CNT_THRESHOLD)
  {
    return (uint8_t) errors.mask;
  }
  if (blocksDone <= FACTORY_TEST_MIC_BUFFER_SKIP_COUNT)
  {
    return FACTORY_TEST_MIC_FAIL_NO_DATA_IN_BUFFER; /* nothing arrived after the skip */
  }
  AudioCapture_prepareRead(testRing, 2U * TEST_BLOCK_SAMPLES);
  for (i = 0U; i < 2U * TEST_BLOCK_SAMPLES; i++)
  {
    if (testRing[i] != 0)
    {
      return FACTORY_TEST_MIC_PASS;
    }
  }
  return FACTORY_TEST_MIC_FAIL_NO_DATA_IN_BUFFER;
}
