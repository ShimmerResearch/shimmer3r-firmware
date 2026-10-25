/*
 * hal_PowerProfile.c
 *
 * See hal_PowerProfile.h. Rules the run keeps, because a power analyser is
 * watching:
 *
 * - The unit is quieted first: LED blink timer stopped, SD unmounted and
 *   unpowered, Bluetooth stopped.
 * - Every wait sleeps (WFI) and is timed from the RTC, which runs from the
 *   32.768 kHz LSE, not from SysTick.
 * - The record is written only at step boundaries, never inside a step or
 *   its return-to-baseline window.
 * - Nothing here may reach Error_Handler() or wait for ever: a part that
 *   does not answer clears its step_ok bit and the run carries on.
 */

#include "hal_PowerProfile.h"

#include <string.h>

#include "ADXL371/adxl371.h"
#include "main.h"
#include "shimmer_include.h"

#define PP_TICKS_PER_S 32768U

/* Each step's state, for the record */
static pp_record_t ppRecord;

/* --- Timing ------------------------------------------------------------------ */

static uint64_t PowerProfile_msToTicks(uint32_t ms)
{
  return ((uint64_t) ms * PP_TICKS_PER_S + 500U) / 1000U;
}

/* Sleeps until `ms` after `startTicks` (an RTC_get64() reading). SysTick
 * wakes the core every millisecond, so the resolution is about 1 ms. The
 * HAL tick bounds the wait as well, at twice the time asked for, so a clock
 * that has stopped cannot hold the boot here. */
static void PowerProfile_waitUntil(uint64_t startTicks, uint32_t ms)
{
  uint64_t endTicks = startTicks + PowerProfile_msToTicks(ms);
  uint32_t halStart = HAL_GetTick();
  uint32_t halLimit = 2U * ms + 100U;

  while (RTC_get64() < endTicks)
  {
    if ((HAL_GetTick() - halStart) > halLimit)
    {
      break;
    }
    __WFI();
  }
}

static void PowerProfile_sleepMs(uint32_t ms)
{
  PowerProfile_waitUntil(RTC_get64(), ms);
}

/* --- LEDs ---------------------------------------------------------------------- */

static void PowerProfile_ledsAll(uint8_t on)
{
  uint32_t colour = on ? (uint32_t) LED_RGB_ALL_ON : (uint32_t) LED_RGB_ALL_OFF;
  Board_ledLwrSetColour(colour);
  Board_ledUprSetColour(colour);
}

/* Every LED channel full on for onMs, from startTicks */
static void PowerProfile_flash(uint64_t startTicks, uint32_t onMs)
{
  PowerProfile_ledsAll(1);
  PowerProfile_waitUntil(startTicks, onMs);
  PowerProfile_ledsAll(0);
}

/* PP_MARKER_NONCE16: sync, then the nonce MSB first */
static void PowerProfile_marker(uint16_t nonce)
{
  uint64_t start = RTC_get64();
  uint8_t bit;

  PowerProfile_flash(start, PP_MARKER_SYNC_MS);
  for (bit = 0; bit < 16U; bit++)
  {
    uint32_t slotStartMs = PP_MARKER_SYNC_MS + (uint32_t) bit * PP_MARKER_SLOT_MS;
    uint32_t onMs = (nonce & (0x8000U >> bit)) ? PP_MARKER_BIT1_MS : PP_MARKER_BIT0_MS;

    PowerProfile_waitUntil(start, slotStartMs);
    PowerProfile_ledsAll(1);
    PowerProfile_waitUntil(start, slotStartMs + onMs);
    PowerProfile_ledsAll(0);
  }
  PowerProfile_waitUntil(start, PP_MARKER_DURATION_MS);
}

/* --- The schedule ---------------------------------------------------------------- */

static uint8_t PowerProfile_hasExpansionPowerSwitch(void)
{
  return ShimBrd_isExpBrdId(EXP_BRD_GSR_UNIFIED)
      || ShimBrd_isExpBrdId(EXP_BRD_BR_AMP_UNIFIED)
      || ShimBrd_isExpBrdId(EXP_BRD_PROTO3_DELUXE);
}

static void PowerProfile_setExpansionPowerSwitch(uint8_t state)
{
  /* The per-board switches (hal_Board.h), all of them, whatever the
   * configuration enables - Board_setExpansionBrdPower() follows the config */
  if (ShimBrd_isExpBrdId(EXP_BRD_GSR_UNIFIED))
  {
    Board_SW_PPG_POWER(state);
  }
  else if (ShimBrd_isExpBrdId(EXP_BRD_BR_AMP_UNIFIED))
  {
    Board_SW_BRIDGE_AMP_PWR(state);
    Board_SW_VOLTAGE_DIVIDER_PWR(state);
  }
  else if (ShimBrd_isExpBrdId(EXP_BRD_PROTO3_DELUXE))
  {
    Board_SW_PROTO3_DELUXE_PWR(state);
  }
}

/* Overrides the weak default in log-and-stream-common: the one place the
 * schedule is built, for the run and for the dock readback alike. Which steps
 * run follows the board: the SR number's presence gates, and the pressure
 * sensor identified at boot. */
uint8_t platform_powerProfileSchedule(pp_schedule_t *s)
{
  uint8_t i;

  memset(s, 0, sizeof(*s));
  s->scheduleVer = PP_SCHEDULE_VER;
  s->markerId = PP_MARKER_NONCE16;
  s->baselineWindowMs = PP_BASELINE_WINDOW_MS;

  if (ShimBrd_isLis3mdlPresent())
  {
    s->partsExpected |= PP_PART_LIS3MDL;
  }
  if (ShimBrd_isAdxl371Present())
  {
    s->partsExpected |= PP_PART_ADXL371;
  }
  if (isAds7028Present())
  {
    s->partsExpected |= PP_PART_ADS7028;
  }
  s->partsExpected |= ShimBrd_isBmp581PresentPerSrNumber() ? PP_PART_BMP581 : PP_PART_BMP390;
  if (ShimBrd_isAds1292Present())
  {
    s->partsExpected |= PP_PART_ADS1292R;
  }

  ShimFactoryRecord_addPpStep(s, PP_STEP_MARKER, PP_MARKER_DURATION_MS);
  ShimFactoryRecord_addPpStep(s, PP_STEP_BASELINE, PP_STEP_MS_BASELINE);
  ShimFactoryRecord_addPpStep(s, PP_STEP_SENSING_RAIL, PP_STEP_MS_SENSING_RAIL);
  ShimFactoryRecord_addPpStep(s, PP_STEP_LSM6DSV, PP_STEP_MS_SENSOR);
  if (ShimBrd_isLis3mdlPresent())
  {
    ShimFactoryRecord_addPpStep(s, PP_STEP_LIS3MDL, PP_STEP_MS_SENSOR);
  }
  ShimFactoryRecord_addPpStep(s, PP_STEP_LIS2DW12, PP_STEP_MS_SENSOR);
  ShimFactoryRecord_addPpStep(s, PP_STEP_LIS2MDL, PP_STEP_MS_SENSOR);
  if (ShimBrd_isAdxl371Present())
  {
    ShimFactoryRecord_addPpStep(s, PP_STEP_ADXL371, PP_STEP_MS_SENSOR);
  }
  ShimFactoryRecord_addPpStep(
      s, isBmp581InUse() ? PP_STEP_BMP581 : PP_STEP_BMP390, PP_STEP_MS_SENSOR);
  if (ShimBrd_isAds1292Present())
  {
    ShimFactoryRecord_addPpStep(s, PP_STEP_ADS1292R, PP_STEP_MS_SENSOR);
  }
  if (isAds7028Present())
  {
    ShimFactoryRecord_addPpStep(s, PP_STEP_ADS7028_GSR, PP_STEP_MS_ADS7028_GSR);
  }
  if (PowerProfile_hasExpansionPowerSwitch())
  {
    ShimFactoryRecord_addPpStep(s, PP_STEP_EXP_POWER, PP_STEP_MS_EXP_POWER);
  }
  ShimFactoryRecord_addPpStep(s, PP_STEP_MICROPHONE, PP_STEP_MS_MICROPHONE);
  ShimFactoryRecord_addPpStep(s, PP_STEP_SD, PP_STEP_MS_SD);
  for (i = PP_STEP_LED_LWR_RED; i <= PP_STEP_LED_UPR_BLUE; i++)
  {
    ShimFactoryRecord_addPpStep(s, i, PP_STEP_MS_LED);
  }
  ShimFactoryRecord_addPpStep(s, PP_STEP_BLUETOOTH, PP_STEP_MS_BLUETOOTH);
  return 1;
}

/* --- The steps ------------------------------------------------------------------ */

/* Sensing rail on, then the part probe: the SPI parts whose fitting varies by
 * board. The BMP was identified at boot, the ADS1292R is probed only where the
 * SR number says ExG, and the microphone is not probed. Returns 1 if the rail
 * came up and SPI1/SPI2 were initialised. */
static uint8_t PowerProfile_sensingRailAndProbe(void)
{
  Board_enableSensingPower(SENSE_PWR_FACTORY_TEST, 1);

  MX_SPI1_Init();
  if (isAdxl371Detected())
  {
    ppRecord.partsFound |= PP_PART_ADXL371;
  }
  if (isAds7028Present() && ads7028_factoryTestChipId() == SELF_TEST_PASS)
  {
    ppRecord.partsFound |= PP_PART_ADS7028;
  }
  if (PressureSensor_wasIdentifiedByChipId())
  {
    ppRecord.partsFound |= isBmp581InUse() ? PP_PART_BMP581 : PP_PART_BMP390;
  }

  MX_SPI2_Init();
  if (lis3mdl_isDetected())
  {
    ppRecord.partsFound |= PP_PART_LIS3MDL;
  }

  if (ShimBrd_isAds1292Present())
  {
    ADS1292_init();
    MX_SPI3_Init();
    if (EXG_init(&hspi3) == 0U)
    {
      ppRecord.partsFound |= PP_PART_ADS1292R;
    }
    EXG_powerOff();
    SPI3_DeInit();
  }
  return 1;
}

/* Runs one step: brings the part or rail up, holds it until the step's time
 * is up, and takes it down again. Returns 1 if every call it made succeeded. */
static uint8_t PowerProfile_runStep(uint8_t stepId, uint16_t durationMs)
{
  uint64_t start = RTC_get64();
  uint8_t ok = 1;

  switch (stepId)
  {
  case PP_STEP_MARKER:
    PowerProfile_marker(ppRecord.nonce);
    break;

  case PP_STEP_BASELINE:
    break;

  case PP_STEP_SENSING_RAIL:
    ok = PowerProfile_sensingRailAndProbe();
    break;

  /* Each sensor at its fastest rate in its highest-current mode. A
   * sampling frequency of 0 keeps every data-ready interrupt off. */
  case PP_STEP_LSM6DSV:
    lsm6dsv_configure(0.0f, 1, 1, LSM6DSV_ODR_AT_7680Hz, LSM6DSV_4000dps, LSM6DSV_16g);
    PowerProfile_waitUntil(start, durationMs);
    ok = (lsm6dsv_restore_default_config() == 0);
    break;

  case PP_STEP_LIS3MDL:
    lis3mdl_configure(0.0f, LIS3MDL_UHP_155Hz, LIS3MDL_16_GAUSS);
    PowerProfile_waitUntil(start, durationMs);
    ok = (lis3mdl_restore_default_config() == 0);
    break;

  case PP_STEP_LIS2DW12:
    ok = (lis2dw12_configure(0.0f, LIS2DW12_XL_ODR_1k6Hz, LIS2DW12_16g, LIS2DW12_HIGH_PERFORMANCE)
        == 0);
    PowerProfile_waitUntil(start, durationMs);
    ok &= (lis2dw12_standby() == 0);
    break;

  case PP_STEP_LIS2MDL:
    MX_I2C1_Init();
    lis2mdl_configure(0.0f, LIS2MDL_ODR_100Hz);
    PowerProfile_waitUntil(start, durationMs);
    lis2mdl_sleep();
    I2C1_DeInit();
    break;

  case PP_STEP_ADXL371:
    adxl371_configure(ADXL371_ODR_5120HZ);
    PowerProfile_waitUntil(start, durationMs);
    adxl371_reset_chip();
    break;

  case PP_STEP_BMP390:
    /* 100 Hz and up selects the BMP390's fastest output rate, 200 Hz */
    ok = (bmp3_configure(100.0f, BMP3_NO_OVERSAMPLING) == BMP3_OK);
    PowerProfile_waitUntil(start, durationMs);
    ok &= (bmp3_restore_default_config() == BMP3_OK);
    break;

  case PP_STEP_BMP581:
    /* Just under the BMP581's fastest output rate, 240 Hz, selects it */
    ok = (bmp5_configure(230.0f, BMP5_OVERSAMPLING_1X) == BMP5_OK);
    PowerProfile_waitUntil(start, durationMs);
    ok &= (bmp5_power_down() == BMP5_OK);
    break;

  case PP_STEP_ADS1292R:
    ADS1292_init();
    MX_SPI3_Init();
    ok = (EXG_init(&hspi3) == 0U);
    if (ok)
    {
      EXG_start(2); /* both chips converting */
    }
    PowerProfile_waitUntil(start, durationMs);
    if (ok)
    {
      EXG_stop(1);
      EXG_stop(0);
    }
    EXG_powerOff();
    SPI3_DeInit();
    break;

  case PP_STEP_ADS7028_GSR:
    /* ADS7028 converting the GSR channel continuously, and on a GSR board
     * the front end on its lowest range (40 kohm feedback). With nothing on
     * the GSR leads that is a fixed, known load. */
    ads7028_factoryTestGsrInit();
    if (ShimBrd_isExpBrdId(EXP_BRD_GSR_UNIFIED))
    {
      GSR_setActiveResistor(HW_RES_40K);
    }
    PowerProfile_waitUntil(start, durationMs);
    stopAds7028Conversions();
    GSR_resetGsrRange();
    break;

  case PP_STEP_EXP_POWER:
    PowerProfile_setExpansionPowerSwitch(1);
    PowerProfile_waitUntil(start, durationMs);
    PowerProfile_setExpansionPowerSwitch(0);
    break;

  case PP_STEP_MICROPHONE:
    Board_setMicPower(1);
    PowerProfile_waitUntil(start, durationMs);
    Board_setMicPower(0);
    break;

  case PP_STEP_SD:
    /* The SD card is left powered and mounted, as an undocked boot leaves
     * it */
    if (LogAndStream_checkSdInSlot())
    {
      Board_sd2Mcu();
      ok = (hsd1.Instance != NULL) && shimmerStatus.sdPeripheralInit;
    }
    else
    {
      ok = 0;
    }
    break;

  case PP_STEP_LED_LWR_RED:
  case PP_STEP_LED_LWR_GREEN:
  case PP_STEP_LED_LWR_BLUE:
  case PP_STEP_LED_UPR_RED:
  case PP_STEP_LED_UPR_GREEN:
  case PP_STEP_LED_UPR_BLUE:
  {
    static const uint32_t colour[3] = { LED_RGB_RED, LED_RGB_GREEN, LED_RGB_BLUE };
    uint8_t idx = (uint8_t) (stepId - PP_STEP_LED_LWR_RED);
    if (idx < 3U)
    {
      Board_ledLwrSetColour(colour[idx]);
    }
    else
    {
      Board_ledUprSetColour(colour[idx - 3U]);
    }
    PowerProfile_waitUntil(start, durationMs);
    PowerProfile_ledsAll(0);
    break;
  }

  case PP_STEP_BLUETOOTH:
    InitialiseBtAfterBoot();
    PowerProfile_waitUntil(start, durationMs);
    ok = shimmerStatus.btPowerOn;
    break;

  default:
    ok = 0;
    break;
  }

  PowerProfile_waitUntil(start, durationMs);
  return ok;
}

/* --- The run ---------------------------------------------------------------------- */

static void PowerProfile_quiet(void)
{
  stopLedBlinkTimer();
  PowerProfile_ledsAll(0);

  /* SD unmounted, its peripheral released and the card unpowered */
  ShimSd_mount(SD_UNMOUNT);
  if (hsd1.Instance != NULL)
  {
    HAL_SD_Abort(&hsd1);
    mmc1DeInit();
    hsd1.Instance = NULL;
  }
  shimmerStatus.sdPeripheralInit = 0;
  Board_setSdPower(0);

  BtStop(1);

  Board_setMicPower(0);
  if (PowerProfile_hasExpansionPowerSwitch())
  {
    PowerProfile_setExpansionPowerSwitch(0);
  }
}

/* Puts back what the run changed and Init() expects: the sensing rail off,
 * the SD card mounted if there is one, Bluetooth on, the LEDs blinking */
static void PowerProfile_restore(void)
{
  PowerProfile_ledsAll(0);
  Board_setMicPower(0);
  if (PowerProfile_hasExpansionPowerSwitch())
  {
    PowerProfile_setExpansionPowerSwitch(0);
  }
  SPI1_DeInit();
  SPI2_DeInit();
  Board_enableSensingPower(SENSE_PWR_FACTORY_TEST, 0);

  if (LogAndStream_checkSdInSlot() && hsd1.Instance == NULL)
  {
    Board_sd2Mcu();
  }
  if (!shimmerStatus.btPowerOn)
  {
    InitialiseBtAfterBoot();
  }
  startLedBlinkTimer();
}

static uint8_t PowerProfile_isCableAttached(void)
{
  return Board_isDocked() || Board_isUsbPluggedIn();
}

void PowerProfile_runIfArmed(void)
{
  pp_schedule_t schedule;
  uint8_t i;

  if (!ShimEeprom_readPowerProfile(&ppRecord))
  {
    return;
  }

  if (ppRecord.state == PP_STATE_RUNNING)
  {
    /* A run was cut short. It is never re-run: its trace is already wrong */
    ppRecord.state = PP_STATE_ABORTED;
    (void) ShimEeprom_writePowerProfile(&ppRecord);
    return;
  }
  if (ppRecord.state != PP_STATE_ARMED)
  {
    return;
  }

  /* The arm is consumed by this boot, whatever happens next */
  ppRecord.runSeq = 0;
  ppRecord.stepOk = 0;
  ppRecord.partsFound = 0;
  if (LogAndStream_isDockedOrUsbIn())
  {
    ppRecord.state = PP_STATE_SKIPPED;
    (void) ShimEeprom_writePowerProfile(&ppRecord);
    return;
  }

  (void) platform_powerProfileSchedule(&schedule);
  ppRecord.scheduleVer = schedule.scheduleVer;
  ppRecord.state = PP_STATE_RUNNING;
  if (!ShimEeprom_writePowerProfile(&ppRecord))
  {
    /* Unless RUNNING is on record, a reset mid-run would run it again */
    return;
  }

  LogAndStream_setBootStage(BOOT_STAGE_POWER_PROFILE);
  PowerProfile_quiet();

  for (i = 0; i < schedule.n; i++)
  {
    /* Step boundary: the record says which step is next, a short pulse
     * marks the boundary in the trace (the marker step marks itself) */
    ppRecord.runSeq = i;
    if (PowerProfile_isCableAttached())
    {
      ppRecord.state = PP_STATE_ABORTED;
      break;
    }
    (void) ShimEeprom_writePowerProfile(&ppRecord);
    if (i > 0U)
    {
      PowerProfile_flash(RTC_get64(), PP_MARKER_PULSE_MS);
    }

    if (PowerProfile_runStep(schedule.steps[i].stepId, schedule.steps[i].durationMs) && i < 32U)
    {
      ppRecord.stepOk |= (uint32_t) 1U << i;
    }
    PowerProfile_sleepMs(schedule.baselineWindowMs);
  }

  if (ppRecord.state == PP_STATE_RUNNING)
  {
    ppRecord.runSeq = schedule.n;
    ppRecord.state = PP_STATE_DONE;
  }
  (void) ShimEeprom_writePowerProfile(&ppRecord);

  PowerProfile_restore();
}
