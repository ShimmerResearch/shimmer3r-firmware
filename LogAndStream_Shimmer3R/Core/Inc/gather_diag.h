/*
 * gather_diag.h
 *
 * DEV-1363 diagnostic build only (GATHER_DIAG in log_and_stream_definitions.h):
 * which stage of a sensor gather goes missing when the packet ring's 100 ms
 * fail-safe releases it. Everything lands in the RAM struct gatherDiag, read
 * over SWD after a bench run and decoded by the DEV-1363 reader script.
 *
 * Every field is a uint32_t, or bytes packed four to a word, so the reader can
 * walk the struct without the compiler's layout. Bump GDIAG_MAGIC on any change.
 */

#ifndef GATHER_DIAG_H
#define GATHER_DIAG_H

#include <stdint.h>

#include "log_and_stream_definitions.h"
#include "stm32u5xx_hal.h"

#if GATHER_DIAG

#define GDIAG_MAGIC            0x32474447U /* "GDG2" */
#define GDIAG_TRACE_LEN        16U         /* power of two */
#define GDIAG_EVENTS           8U
#define GDIAG_ISR_LEN          24U
/* Bytes RTC_IRQHandler's prologue pushes before GDIAG_RTC_ENTRY() reads the
 * MSP: the exception frame (r0-r3, r12, lr, pc, xpsr) starts this far above
 * it. Checked against the disassembly of each build. */
#define GDIAG_RTC_FRAME_OFFSET 8U

/* Bus state at one moment. */
typedef struct
{
  uint32_t rtcNow;     /* RTC_get32() when taken */
  uint32_t stallCount; /* sample periods the gather had been outstanding */
  uint32_t slotTs;     /* stamp of the outstanding gather's packet */
  uint32_t cbFlags;    /* currentCbFlags | expectedCbFlags << 8 */
  uint32_t i2cBus;     /* currentI2cBusCbFlags | expectedI2cBusCbFlags << 8 */
  uint32_t spiBus;     /* currentSpiBusCbFlags | expectedSpiBusCbFlags << 8 */
  uint32_t i2c1Sens; /* status | sensorCnt << 8 | sensorLen << 16 | sensorList[sensorCnt] << 24 */
  uint32_t spi1Sens;   /* as i2c1Sens */
  uint32_t spi2Sens;   /* as i2c1Sens */
  uint32_t hi2c1State; /* State | Mode << 8 | XferCount << 16 */
  uint32_t hi2c1Err;   /* ErrorCode */
  uint32_t i2c1Isr;
  uint32_t i2c1Cr1;
  uint32_t i2c1Cr2;
  uint32_t hspi1State; /* State | RxXferCount << 16 */
  uint32_t hspi1Tx;    /* TxXferCount */
  uint32_t hspi1Err;   /* ErrorCode */
  uint32_t spi1Sr;
  uint32_t spi1Cr1;
  uint32_t hspi2;        /* State | ErrorCode << 16 */
  uint32_t dmaI2c1Rx[3]; /* GPDMA1 channel 10: CSR, CCR, CBR1 */
  uint32_t dmaSpi1Rx[3]; /* channel 4 */
  uint32_t dmaSpi1Tx[3]; /* channel 5 */
  uint32_t gpio;     /* bit 0 LSM6DSV INT1, bit 1 LIS2MDL DRDY, bit 2 BMP INT */
  uint32_t i2cStale; /* i2cStaleCompletions */
  uint32_t i2cSeqErr; /* i2cSequenceErrors */
  uint32_t ring;      /* ring diag stallResets | gathersRefused << 16 */
  uint32_t ring2;     /* ring diag completionsDropped | drainSkipped << 16 */
} GatherSnap;

/* One gather. Cycle counts are DWT CYCCNT, relative to the start; 0 = never
 * happened. Byte arrays are indexed ADC, I2C, SPI, spare, or SPI bus 1, 2. */
typedef struct
{
  uint32_t startCyc;
  uint32_t slotTs;
  uint32_t stageCyc[3];
  uint32_t doneCyc;
  uint8_t ipsr[4];      /* exception number the stage completed in */
  uint8_t flagsSeen[4]; /* currentCbFlags just before the stage set its bit */
  uint32_t rtcPc; /* where the RTC interrupt will return to: what it preempted */
  uint32_t rtcLr;
  uint32_t execTask;     /* task the main loop was running */
  uint32_t spiPollCyc;   /* SPI_pollSensors() entered */
  uint32_t spiBusCyc[2]; /* SPI_busGatherDataDone_cb() for bus 1 and bus 2 */
  uint8_t spiBusIpsr[2];
  uint8_t spiBusBefore[2]; /* currentSpiBusCbFlags before the bus set its bit */
  uint8_t spiBusAfter[2];  /* ... and the value the completion test read */
  uint8_t spiDone[2];      /* that call took the SPI stage as complete */
  uint8_t spiCplt[2];      /* SPI1, SPI2 transfer completions in this gather */
  uint8_t spiBusCalls;
  uint8_t spare;
  uint32_t i2cStartCyc; /* LIS2MDL DMA read started */
  uint32_t i2cRxCyc;    /* I2C1_MemRxCpltCallback() entered */
  uint8_t i2cHalRet;    /* what the read's start returned */
  uint8_t i2cRxStatus;  /* i2c1Sens.status the completion found */
  uint8_t i2cRxIpsr;
  uint8_t i2cRxCount;
  uint32_t nIsr;
  /* Every interrupt from this gather's start to the next one's:
   * cycles from start (23 bits) | exception number << 23 | exit << 31. */
  uint32_t isr[GDIAG_ISR_LEN];
} GatherTrace;

/* One fail-safe stall. */
typedef struct
{
  uint32_t session;
  uint32_t ticksIntoSession; /* RTC ticks from sensing start to the reset */
  GatherSnap first;          /* at the gather's first busy tick */
  GatherSnap atReset;        /* when the fail-safe released it */
  GatherTrace trace[GDIAG_TRACE_LEN]; /* oldest first; the last is the lost one */
} GatherEvent;

typedef struct
{
  uint32_t magic;
  uint32_t size;
  uint32_t sessions;
  uint32_t sessionStartRtc;
  uint32_t gathers;
  uint32_t completes;
  uint32_t busyTicks;     /* ticks that found a gather still outstanding */
  uint32_t lateCompletes; /* gathers that completed after at least one busy tick */
  uint32_t maxLateStall; /* most busy ticks before a gather still completed */
  uint32_t stallResets;
  uint32_t syncStages[3]; /* stage completions inside the RTC interrupt: ADC, I2C, SPI */
  uint32_t asyncStages[3]; /* ... and from anywhere else */
  uint32_t lostUpdates; /* stalls whose trace saw every expected stage arrive */
  uint32_t i2cErrors;
  uint32_t i2cLastErr; /* ErrorCode | i2c1Sens.status << 16 */
  uint32_t i2cLastErrRtc;
  uint32_t spiErrors;
  uint32_t spiLastErr; /* ErrorCode | bus (1, 2, 3) << 24 */
  uint32_t spiLastErrRtc;
  uint32_t nEvents;    /* stalls seen; the first GDIAG_EVENTS are kept */
  uint32_t rtcMsp;     /* MSP at the latest RTC interrupt entry */
  uint32_t lateStarts; /* gathers that started more than 2000 cycles late */
  GatherSnap pending;  /* first busy tick of the gather outstanding now */
  uint32_t liveHead;
  GatherTrace live[GDIAG_TRACE_LEN];
  GatherEvent ev[GDIAG_EVENTS];
} GatherDiag;

extern GatherDiag gatherDiag;

#define GDIAG_RTC_ENTRY() (gatherDiag.rtcMsp = __get_MSP())
#define GDIAG_ISR_IN()    GatherDiag_isr(0U)
#define GDIAG_ISR_OUT()   GatherDiag_isr(1U)

void GatherDiag_isr(uint32_t exit);
void GatherDiag_i2cError(I2C_HandleTypeDef *hi2c);
void GatherDiag_spiError(SPI_HandleTypeDef *hspi);
void GatherDiag_onSpiPoll(void);
void GatherDiag_onSpiBusDone(uint8_t flag, uint8_t before, uint8_t after, uint8_t done);
void GatherDiag_onSpiCplt(uint32_t bus);
void GatherDiag_onI2cStart(uint8_t halRet);
void GatherDiag_onI2cRx(uint8_t status);

#else

#define GDIAG_RTC_ENTRY()
#define GDIAG_ISR_IN()
#define GDIAG_ISR_OUT()

#endif /* GATHER_DIAG */
#endif /* GATHER_DIAG_H */
