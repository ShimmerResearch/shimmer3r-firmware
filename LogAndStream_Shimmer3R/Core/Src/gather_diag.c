/*
 * gather_diag.c
 *
 * DEV-1363 diagnostic build only - see gather_diag.h. The hooks are called
 * from log-and-stream-common's shimmer_sensing.c: onStart and
 * onBusy/onStallReset from the RTC wake-up interrupt (priority 1), onStage and
 * onComplete from wherever a stage completes (the RTC interrupt for a stage
 * that finished without a transfer, a priority-0 DMA/peripheral interrupt
 * otherwise). The bus hooks are called from i2c.c and spi.c, and
 * GatherDiag_isr() from every interrupt handler in stm32u5xx_it.c.
 *
 * The hooks only record. Nothing here changes what the gather does, so a
 * stall still costs its ~100 ms and still shows in the SD log.
 */

#include "gather_diag.h"

#if GATHER_DIAG

#include <string.h>

#include "i2c.h"
#include "rtc.h"
#include "shimmer_include.h"
#include "spi.h"

extern volatile uint8_t expectedCbFlags, currentCbFlags;
extern volatile uint8_t expectedI2cBusCbFlags, currentI2cBusCbFlags;
extern volatile uint8_t expectedSpiBusCbFlags, currentSpiBusCbFlags;
extern I2CTypeDef i2c1Sens;
extern SPITypeDef spi1Sens;
extern SPITypeDef spi2Sens;
extern volatile uint32_t i2cStaleCompletions;
extern volatile uint32_t i2cSequenceErrors;
extern SPI_HandleTypeDef hspi2;
uint32_t ShimTask_getExecutingTask(void);

GatherDiag gatherDiag __attribute__((used));

/* Busy ticks the current gather has seen. Written by the RTC interrupt, read
 * at completion. */
static volatile uint16_t curBusy;
/* Shortest start-to-start interval this session: one sample period. */
static uint32_t minPeriodCyc;

static uint32_t stageIdx(uint8_t stage)
{
  return (stage == STAT_PERI_ADC)   ? 0U :
      (stage == STAT_PERI_I2C_SENS) ? 1U :
                                      2U;
}

static uint32_t sensWord(uint32_t status, uint8_t cnt, uint8_t len, uint8_t listItem)
{
  return (status & 0xFFU) | ((uint32_t) cnt << 8) | ((uint32_t) len << 16)
      | ((uint32_t) listItem << 24);
}

static GatherTrace *liveTrace(void)
{
  return &gatherDiag.live[gatherDiag.liveHead];
}

static uint32_t relCyc(const GatherTrace *t)
{
  uint32_t d = DWT->CYCCNT - t->startCyc;
  return d ? d : 1U; /* 0 means "never" */
}

static void capture(GatherSnap *s, uint16_t stallCount)
{
  s->rtcNow = RTC_get32();
  s->stallCount = stallCount;
  s->slotTs = ShimSens_getPacketBuffAtWrIdx()->timestampTicks;
  s->cbFlags = currentCbFlags | ((uint32_t) expectedCbFlags << 8);
  s->i2cBus = currentI2cBusCbFlags | ((uint32_t) expectedI2cBusCbFlags << 8);
  s->spiBus = currentSpiBusCbFlags | ((uint32_t) expectedSpiBusCbFlags << 8);
  s->i2c1Sens = sensWord(i2c1Sens.status, i2c1Sens.sensorCnt, i2c1Sens.sensorLen,
      i2c1Sens.sensorList[i2c1Sens.sensorCnt % sizeof(i2c1Sens.sensorList)]);
  s->spi1Sens = sensWord(spi1Sens.status, spi1Sens.sensorCnt, spi1Sens.sensorLen,
      (uint8_t) spi1Sens.sensorList[spi1Sens.sensorCnt % 20U]);
  s->spi2Sens = sensWord(spi2Sens.status, spi2Sens.sensorCnt, spi2Sens.sensorLen,
      (uint8_t) spi2Sens.sensorList[spi2Sens.sensorCnt % 20U]);
  s->hi2c1State = (uint32_t) hi2c1.State | ((uint32_t) hi2c1.Mode << 8)
      | ((uint32_t) hi2c1.XferCount << 16);
  s->hi2c1Err = hi2c1.ErrorCode;
  s->i2c1Isr = I2C1->ISR;
  s->i2c1Cr1 = I2C1->CR1;
  s->i2c1Cr2 = I2C1->CR2;
  s->hspi1State = (uint32_t) hspi1.State | ((uint32_t) hspi1.RxXferCount << 16);
  s->hspi1Tx = hspi1.TxXferCount;
  s->hspi1Err = hspi1.ErrorCode;
  s->spi1Sr = SPI1->SR;
  s->spi1Cr1 = SPI1->CR1;
  s->hspi2 = (uint32_t) hspi2.State | (hspi2.ErrorCode << 16);
  s->dmaI2c1Rx[0] = GPDMA1_Channel10->CSR;
  s->dmaI2c1Rx[1] = GPDMA1_Channel10->CCR;
  s->dmaI2c1Rx[2] = GPDMA1_Channel10->CBR1;
  s->dmaSpi1Rx[0] = GPDMA1_Channel4->CSR;
  s->dmaSpi1Rx[1] = GPDMA1_Channel4->CCR;
  s->dmaSpi1Rx[2] = GPDMA1_Channel4->CBR1;
  s->dmaSpi1Tx[0] = GPDMA1_Channel5->CSR;
  s->dmaSpi1Tx[1] = GPDMA1_Channel5->CCR;
  s->dmaSpi1Tx[2] = GPDMA1_Channel5->CBR1;
  s->gpio = ((LSM6DSV_INT1_GPIO_Port->IDR & LSM6DSV_INT1_Pin) ? 1U : 0U)
      | ((LIS2MDL_DRDY_GPIO_Port->IDR & LIS2MDL_DRDY_Pin) ? 2U : 0U)
      | ((BMP390_INT_GPIO_Port->IDR & BMP390_INT_Pin) ? 4U : 0U);
  s->i2cStale = i2cStaleCompletions;
  s->i2cSeqErr = i2cSequenceErrors;
  s->ring = sensing.ring.diag.stallResets
      | ((uint32_t) sensing.ring.diag.gathersRefused << 16);
  s->ring2 = sensing.ring.diag.completionsDropped
      | ((uint32_t) sensing.ring.diag.drainSkipped << 16);
}

void GatherDiag_onSessionStart(void)
{
  if (gatherDiag.magic != GDIAG_MAGIC)
  {
    memset(&gatherDiag, 0, sizeof(gatherDiag));
    gatherDiag.magic = GDIAG_MAGIC;
    gatherDiag.size = sizeof(gatherDiag);
  }
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  gatherDiag.sessions++;
  gatherDiag.sessionStartRtc = RTC_get32();
  minPeriodCyc = 0xFFFFFFFFU;
}

void GatherDiag_onStart(void)
{
  GatherTrace *prev = liveTrace();
  uint32_t head = (gatherDiag.liveHead + 1U) & (GDIAG_TRACE_LEN - 1U);
  GatherTrace *t = &gatherDiag.live[head];
  const uint32_t *frame = (const uint32_t *) (gatherDiag.rtcMsp + GDIAG_RTC_FRAME_OFFSET);
  uint32_t now = DWT->CYCCNT;
  uint32_t period = now - prev->startCyc;

  /* Filled before the head moves: a completion or interrupt that preempts this
   * lands in the previous gather's entry, never in a half-cleared one. */
  memset(t, 0, sizeof(*t));
  t->startCyc = now;
  t->slotTs = ShimSens_getPacketBuffAtWrIdx()->timestampTicks;
  t->rtcLr = frame[5];
  t->rtcPc = frame[6];
  t->execTask = ShimTask_getExecutingTask();
  gatherDiag.liveHead = head;
  gatherDiag.gathers++;
  curBusy = 0;

  if (period < minPeriodCyc)
  {
    minPeriodCyc = period;
  }
  else if (period > minPeriodCyc + 2000U)
  {
    gatherDiag.lateStarts++;
  }
}

void GatherDiag_onStage(uint8_t stage)
{
  GatherTrace *t = liveTrace();
  uint32_t idx = stageIdx(stage);
  uint32_t ipsr = __get_IPSR();

  /* Separate words and bytes per stage: a store here never read-modify-writes
   * a location another stage's completion also writes. */
  t->stageCyc[idx] = relCyc(t);
  t->ipsr[idx] = (uint8_t) ipsr;
  t->flagsSeen[idx] = currentCbFlags;
  if (ipsr == (uint32_t) (RTC_IRQn + 16))
  {
    gatherDiag.syncStages[idx]++;
  }
  else
  {
    gatherDiag.asyncStages[idx]++;
  }
}

void GatherDiag_onComplete(void)
{
  GatherTrace *t = liveTrace();

  t->doneCyc = relCyc(t);
  gatherDiag.completes++;
  if (curBusy)
  {
    gatherDiag.lateCompletes++;
    if (curBusy > gatherDiag.maxLateStall)
    {
      gatherDiag.maxLateStall = curBusy;
    }
  }
}

void GatherDiag_onBusy(uint16_t stallCount)
{
  gatherDiag.busyTicks++;
  curBusy = stallCount;
  if (stallCount == 1U)
  {
    capture(&gatherDiag.pending, stallCount);
  }
}

void GatherDiag_onStallReset(void)
{
  uint32_t n = gatherDiag.nEvents;

  gatherDiag.stallResets++;
  if (n < GDIAG_EVENTS)
  {
    GatherEvent *e = &gatherDiag.ev[n];
    uint32_t head = gatherDiag.liveHead;
    GatherTrace *lost = &gatherDiag.live[head];
    uint32_t seen = 0;
    uint32_t i;

    capture(&e->atReset, sensing.ring.stallCount);
    e->first = gatherDiag.pending;
    e->session = gatherDiag.sessions;
    e->ticksIntoSession = e->atReset.rtcNow - gatherDiag.sessionStartRtc;
    for (i = 0; i < GDIAG_TRACE_LEN; i++)
    {
      e->trace[i] = gatherDiag.live[(head + 1U + i) & (GDIAG_TRACE_LEN - 1U)];
    }

    /* Every expected stage reached ShimSens_stageCompleteCb, yet the flags
     * never matched: a lost update rather than a lost completion. */
    seen |= lost->stageCyc[0] ? STAT_PERI_ADC : 0U;
    seen |= lost->stageCyc[1] ? STAT_PERI_I2C_SENS : 0U;
    seen |= lost->stageCyc[2] ? STAT_PERI_SPI_SENS : 0U;
    if ((seen & expectedCbFlags) == expectedCbFlags)
    {
      gatherDiag.lostUpdates++;
    }
  }
  gatherDiag.nEvents = n + 1U;
}

void GatherDiag_isr(uint32_t exit)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  GatherTrace *t = liveTrace();
  uint32_t n = t->nIsr;
  if (n < GDIAG_ISR_LEN)
  {
    t->isr[n] = ((DWT->CYCCNT - t->startCyc) & 0x7FFFFFU)
        | ((__get_IPSR() & 0xFFU) << 23) | (exit << 31);
    t->nIsr = n + 1U;
  }
  __set_PRIMASK(primask);
}

void GatherDiag_onSpiPoll(void)
{
  GatherTrace *t = liveTrace();
  t->spiPollCyc = relCyc(t);
}

void GatherDiag_onSpiBusDone(uint8_t flag, uint8_t before, uint8_t after, uint8_t done)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  GatherTrace *t = liveTrace();
  uint32_t idx = (flag == SPI1_BUS_FLAG) ? 0U : 1U;
  if (flag == SPI1_BUS_FLAG || flag == SPI2_BUS_FLAG)
  {
    t->spiBusCyc[idx] = relCyc(t);
    t->spiBusIpsr[idx] = (uint8_t) __get_IPSR();
    t->spiBusBefore[idx] = before;
    t->spiBusAfter[idx] = after;
    t->spiDone[idx] = done;
  }
  t->spiBusCalls++;
  __set_PRIMASK(primask);
}

void GatherDiag_onSpiCplt(uint32_t bus)
{
  GatherTrace *t = liveTrace();
  if (bus == 1U || bus == 2U)
  {
    t->spiCplt[bus - 1U]++;
  }
}

void GatherDiag_onI2cStart(uint8_t halRet)
{
  GatherTrace *t = liveTrace();
  t->i2cStartCyc = relCyc(t);
  t->i2cHalRet = halRet;
}

void GatherDiag_onI2cRx(uint8_t status)
{
  GatherTrace *t = liveTrace();
  t->i2cRxCyc = relCyc(t);
  t->i2cRxStatus = status;
  t->i2cRxIpsr = (uint8_t) __get_IPSR();
  t->i2cRxCount++;
}

void GatherDiag_i2cError(I2C_HandleTypeDef *hi2c)
{
  gatherDiag.i2cErrors++;
  gatherDiag.i2cLastErr
      = (hi2c->ErrorCode & 0xFFFFU) | ((uint32_t) i2c1Sens.status << 16);
  gatherDiag.i2cLastErrRtc = RTC_get32();
}

void GatherDiag_spiError(SPI_HandleTypeDef *hspi)
{
  uint32_t bus = (hspi->Instance == SPI1) ? 1U :
      (hspi->Instance == SPI2)            ? 2U :
                                            3U;

  gatherDiag.spiErrors++;
  gatherDiag.spiLastErr = (hspi->ErrorCode & 0xFFFFFFU) | (bus << 24);
  gatherDiag.spiLastErrRtc = RTC_get32();
}

#endif /* GATHER_DIAG */
