/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rtc.c
 * @brief   This file provides code for the configuration
 *          of the RTC instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */

#include "shimmer_definitions.h"
#include "time.h"
#include <CYW20820/CYW20820.h>
#include <TaskList/shimmer_taskList.h>
#include <log_and_stream_externs.h>

uint32_t SHIM_RTC_Status = RTC_STATUS_ZERO;

/* minimum offset (s) to ensure alarm is in the future */
#define RTC_MIN_ALARM_OFFSET_SECONDS 1U
/* Number of NOP cycles to delay before retrying a failed RTC alarm set operation */
#define RTC_ALARM_RETRY_DELAY_CYCLES 10000

volatile time_t nextAlarms[RTC_NUM_ALARMS] = { RTC_ALARM_CONTEXT_NONE };

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  ShimRtc_setRwcConfigTime(0);

  /* USER CODE END RTC_Init 0 */

  RTC_PrivilegeStateTypeDef privilegeState = { 0 };
  RTC_TimeTypeDef sTime = { 0 };
  RTC_DateTypeDef sDate = { 0 };
  RTC_AlarmTypeDef sAlarm = { 0 };

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
   */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 0;
  hrtc.Init.SynchPrediv = 32767;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  hrtc.Init.BinMode = RTC_BINARY_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  privilegeState.rtcPrivilegeFull = RTC_PRIVILEGE_FULL_NO;
  privilegeState.backupRegisterPrivZone = RTC_PRIVILEGE_BKUP_ZONE_NONE;
  privilegeState.backupRegisterStartZone2 = RTC_BKP_DR0;
  privilegeState.backupRegisterStartZone3 = RTC_BKP_DR0;
  if (HAL_RTCEx_PrivilegeModeSet(&hrtc, &privilegeState) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  //Copied from RTC_Calendar example for STM32U575

  /* Set Date and Time (if not already done before)*/
  /* Read the Back Up Register 0 Data */
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2)
  {
    /* Configure RTC Calendar */

    /* USER CODE END Check_RTC_BKUP */

    /** Initialize RTC and set the Time and Date
     */
    sTime.Hours = 0x0;
    sTime.Minutes = 0x0;
    sTime.Seconds = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
    {
      Error_Handler();
    }
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month = RTC_MONTH_JANUARY;
    sDate.Date = 0x1;
    sDate.Year = 0x0;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
    {
      Error_Handler();
    }

    /** Enable the Alarm A
     */
    sAlarm.AlarmTime.Hours = 0x0;
    sAlarm.AlarmTime.Minutes = 0x1;
    sAlarm.AlarmTime.Seconds = 0x0;
    sAlarm.AlarmTime.SubSeconds = 0x0;
    sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS | RTC_ALARMMASK_SECONDS;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    sAlarm.AlarmDateWeekDay = 0x1;
    sAlarm.Alarm = RTC_ALARM_A;
    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
    {
      Error_Handler();
    }

    /** Enable the WakeUp
     */
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV2, 0) != HAL_OK)
    {
      Error_Handler();
    }
    /* USER CODE BEGIN RTC_Init 2 */
    /* DEV-1161: the backup domain was reset, so the calendar above (2000-01-01)
     * is not real time. Clear the time-set marker explicitly rather than
     * relying on the reset having zeroed it - RTC_isRwcTimeSet() reads it. */
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_STATUS_REG, RTC_STATUS_ZERO);
    /* Writes a data in a RTC Backup data Register0 */
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);
  }
  else
  {
    ///* Check if the Power On Reset flag is set */
    //if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET)
    //{
    //  /* Turn on LED6: Power on reset occurred */
    //  BSP_LED_On(LED6);
    //}
    //
    ///* Check if Pin Reset flag is set */
    //if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET)
    //{
    //  /* Turn on LED6: External reset occurred */
    //  BSP_LED_On(LED6);
    //}
  }
  ///* Clear source Reset Flag */
  //__HAL_RCC_CLEAR_RESET_FLAGS();

  RTC_wakeUpOff();

  /* USER CODE END RTC_Init 2 */
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (rtcHandle->Instance == RTC)
  {
    /* USER CODE BEGIN RTC_MspInit 0 */

    /* USER CODE END RTC_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    /* DEV-866: when the boot LSE recovery ladder exhausted every step, run
     * the RTC from the LSI so the device boots and can report the fault
     * rather than hanging. Prescalers/SSR math stay at 32768 Hz (see
     * DEV866_BringUpLse in main.c) - time runs ~2.4% slow +/- LSI tolerance,
     * which is acceptable for a flagged limp-home mode. */
    PeriphClkInit.RTCClockSelection
        = Boot_rtcIsOnLsiFallback() ? RCC_RTCCLKSOURCE_LSI : RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    __HAL_RCC_RTCAPB_CLKAM_ENABLE();

    /* RTC interrupt Init */
    HAL_NVIC_SetPriority(RTC_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);
    /* USER CODE BEGIN RTC_MspInit 1 */

    /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle)
{

  if (rtcHandle->Instance == RTC)
  {
    /* USER CODE BEGIN RTC_MspDeInit 0 */

    /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
    __HAL_RCC_RTCAPB_CLK_DISABLE();
    __HAL_RCC_RTCAPB_CLKAM_DISABLE();

    /* RTC interrupt Deinit */
    HAL_NVIC_DisableIRQ(RTC_IRQn);
    /* USER CODE BEGIN RTC_MspDeInit 1 */

    /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* DEV-1162: keep the RTC interrupt off while task code owns hrtc's HAL lock.
 *
 * Every HAL RTC write takes that lock. The RTC interrupt (Alarm A, and the
 * wake-up timer that drives sampling) can pre-empt task code holding it, and
 * the alarm callback then calls HAL_RTC_SetAlarm_IT, which returns HAL_BUSY.
 * It cannot wait: the task that would release the lock cannot run until the
 * interrupt returns. The callback used to call Error_Handler() at that point,
 * freezing the device with interrupts masked.
 *
 * So task-context writes disable RTC_IRQn while they hold the lock. An alarm
 * or sample raised meanwhile stays pending and runs as soon as they finish -
 * well under a millisecond after a clock set. A nested hold keeps the outer
 * state, and a hold taken inside the RTC interrupt itself is harmless. */
static uint32_t RTC_irqHold(void)
{
  uint32_t wasEnabled = NVIC_GetEnableIRQ(RTC_IRQn);

  NVIC_DisableIRQ(RTC_IRQn);
  __DSB();
  __ISB();
  return wasEnabled;
}

static void RTC_irqRelease(uint32_t wasEnabled)
{
  if (wasEnabled)
  {
    NVIC_EnableIRQ(RTC_IRQn);
  }
}

/* DEV-1161: move the running clock onto the sub-second part of a time-set.
 *
 * RTC_SSR is read-only on the STM32U5 (RM0456 RTC_SSR; the SVD agrees), so the
 * fraction cannot be written directly - the old `SSR = ...` store did nothing
 * and every set was truncated to the whole second. Leaving init mode restarts
 * the calendar at the top of the second, so instead read back what the clock
 * now says and advance it by the difference with RTC_SHIFTR.
 *
 * DEV-1162: what a shift actually does here is not what RM0456 describes.
 * With PREDIV_S = 0x7FFF the synchronous prescaler is SS[14:0]. A shift adds
 * SUBFS to SS, setting SS[15], and when SS[14:0] next wraps the RTC counts
 * that as a second: TR increments and SS[15] clears. Traced on the bench
 * (TR/SSR sampled every 50 ms after a shift). So SUBFS does not delay the
 * clock by SUBFS ticks - it advances it by 32768 - SUBFS ticks, and ADD1S adds
 * a whole second more. DEV-1161 used ADD1S with SUBFS = 32768 - delta, which
 * therefore left every set clock exactly one second fast. Here:
 *   advance by delta ticks:  ADD1S = 0, SUBFS = 32768 - delta  (0 < delta <
 * 32768) A delay cannot be expressed; it is never needed beyond a few ticks,
 * because the clock restarts at the top of the target's second and the
 * read-back follows within microseconds. RTC_getDateTime() reads SS[14:0] only,
 * which keeps the time continuous while SS[15] is set.
 *
 * RM0456 constraints: no shift may be pending (SHPF = 0, HAL waits for it),
 * REFCKON must be 0 (HAL refuses otherwise; it is never set here), and SS[15]
 * must be 0 - always true straight after SetTime, because SSR then holds
 * PREDIV_S = 0x7FFF at most. */
static void RTC_shiftToTicks(uint64_t ticks)
{
  SHIM_RTC_t now;
  int64_t delta;
  HAL_StatusTypeDef status;
  uint32_t tickstart;

  /* Read back with RTC_getDateTime (the HAL path) so the result does not
   * depend on how RTC_get64 is implemented. */
  RTC_getDateTime(&now);
  delta = (int64_t) (ticks - now.ticks);

  /* The clock was just set to the target's whole second, so a true delta lies
   * strictly inside one second. Anything else is a torn read-back - an
   * interrupt reading the RTC between HAL_RTC_GetTime and HAL_RTC_GetDate
   * unlocks the shadow registers, and across midnight that pairs one day's
   * time with the next day's date. Shifting on it would be wrong by up to a
   * second, so leave the clock on the whole second instead.
   *
   * SS[15] is taken from the read-back, never from a fresh read of RTC_SSR
   * (DEV-1162). With BYPSHAD = 0, reading SSR locks the TR/DR shadow
   * registers until DR is read. A bare SSR read here left them locked, so RSF
   * never set and HAL_RTCEx_SetSynchroShift's HAL_RTC_WaitForSynchro ran to
   * its 1 s timeout while holding the RTC HAL lock. */
  if (delta <= 0 || delta > (int64_t) RTC_SHIFTR_SUBFS || (now.subseconds & 0x8000U) != 0U)
  {
    /* delta <= 0: the target is within the few ticks the set took, so the
     * clock is already as close as a shift could make it */
    return;
  }
  status = HAL_RTCEx_SetSynchroShift(
      &hrtc, RTC_SHIFTADD1S_RESET, (uint32_t) (32768 - delta));

  /* The HAL waits for a pending shift before writing SHIFTR, but not for the
   * new one to finish, and HAL_RTC_GetTime's sub-second value is only valid
   * once SHPF has cleared. Wait here so a GET_RWC straight after the set reads
   * the shifted clock. It takes a few RTCCLK cycles; the timeout only bounds a
   * fault. If the shift fails the clock stays on the whole second - still a
   * set clock, as accurate as before DEV-1161 - so the caller's time-set
   * marker stands either way. */
  if (status == HAL_OK)
  {
    tickstart = HAL_GetTick();
    while (READ_BIT(RTC->ICSR, RTC_ICSR_SHPF) != 0U && (HAL_GetTick() - tickstart) <= RTC_TIMEOUT_VALUE)
    {
    }
  }
}

uint8_t RTC_setDateTime(SHIM_RTC_t *data)
{
  uint32_t format = RTC_FORMAT_BIN;
  //S4_RTC_t tmp;
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  /* Check date and time validation */
  //if (format == RTC_FORMAT_BCD) {
  //  tmp.date = RTC_BCD2BIN(data->date);
  //  tmp.month = RTC_BCD2BIN(data->month);
  //  tmp.year = RTC_BCD2BIN(data->year);
  //  tmp.weekday = RTC_BCD2BIN(data->weekday);
  //  tmp.hours = RTC_BCD2BIN(data->hours);
  //  tmp.minutes = RTC_BCD2BIN(data->minutes);
  //  tmp.seconds = RTC_BCD2BIN(data->seconds);
  //} else {
  //  tmp.date = data->date;
  //  tmp.month = data->month;
  //  tmp.year = data->year;
  //  tmp.weekday = data->weekday;
  //  tmp.hours = data->hours;
  //  tmp.minutes = data->minutes;
  //  tmp.seconds = data->seconds;
  //}

  /* Check year and month */
  if (!ShimRtc_isDateValid(data))
  {
    /* Invalid date */
    return 1;
  }

  /* TODO Can be removed as RTC is always initialized on boot. Keeping here for the moment so we can see how to read status from RTC module */
  ///**Initialize RTC and set the Time and Date
  //*/
  //SHIM_RTC_Status = HAL_RTCEx_BKUPRead(&hrtc, RTC_STATUS_REG);
  //if (SHIM_RTC_Status == RTC_STATUS_ZERO)
  //{
  // MX_RTC_Init();
  //}

  uint32_t irqHeld = RTC_irqHold();

  sTime.Hours = data->hours;
  sTime.Minutes = data->minutes;
  sTime.Seconds = data->seconds;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  HAL_RTC_SetTime(&hrtc, &sTime, format);

  sDate.WeekDay = data->weekday;
  sDate.Month = data->month;
  sDate.Date = data->date;
  sDate.Year = data->year;
  HAL_RTC_SetDate(&hrtc, &sDate, format);

  RTC_shiftToTicks(data->ticks);
  ///**Enable the Alarm A
  //*/
  //sAlarm.AlarmTime.Hours = 0x0;
  //sAlarm.AlarmTime.Minutes = 0x0;
  //sAlarm.AlarmTime.Seconds = 0x0;
  //sAlarm.AlarmTime.SubSeconds = 0x0;
  //sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  //sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  //sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  //sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  //sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  //sAlarm.AlarmDateWeekDay = 0x1;
  //sAlarm.Alarm = RTC_ALARM_A;
  //HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD);

  /* Write backup registers */
  SHIM_RTC_Status = RTC_STATUS_TIME_OK;
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_STATUS_REG, RTC_STATUS_TIME_OK);

  RTC_irqRelease(irqHeld);

  /* Return OK */
  return 0;
}

void RTC_getDateTime(SHIM_RTC_t *data)
{
  uint32_t format = RTC_FORMAT_BIN;
  uint32_t unix;
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  /* Get time */
  HAL_RTC_GetTime(&hrtc, &sTime, format);
  /* Get date */
  HAL_RTC_GetDate(&hrtc, &sDate, format);

  /* Format hours */
  data->hours = sTime.Hours;
  data->minutes = sTime.Minutes;
  data->seconds = sTime.Seconds;
  /* Get subseconds */
  data->subseconds = sTime.SubSeconds;

  /* Format date */
  data->year = sDate.Year;
  data->month = sDate.Month;
  data->date = sDate.Date;
  data->weekday = sDate.WeekDay;

  /* Calculate unix offset */
  unix = ShimRtc_rtc2Unix(data);
  data->unix = unix;

  /* SS[14:0] only (DEV-1162). After a shift SS[15] is set until SS[14:0]
   * wraps, and the RTC then increments TR at that wrap, so SS[15] is not part
   * of the sub-second count: including it reads the clock one second slow
   * until the wrap. data->subseconds keeps the raw value. */
  data->ticks = ((uint64_t) data->unix * 32768) + 32768 - (data->subseconds & 0x7FFFU);
}

void RTC_setTimeFromTicksPtr(uint8_t *ticksPtr)
{
  uint64_t temp64;
  memcpy((uint8_t *) (&temp64), ticksPtr, 8); //64bits = 8bytes
  RTC_setTimeFromTicks(temp64);
}

void RTC_setTimeFromTicks(uint64_t ticks)
{
  SHIM_RTC_t data;
  ShimRtc_ticks2Rtc(&data, ticks);
  RTC_setDateTime(&data);
  ShimRtc_setRwcConfigTime(ticks);
}

/* The clock in 32768 Hz ticks since 1970, read straight from the calendar
 * registers. Gives exactly what RTC_getDateTime() gives - same fields, same
 * ShimRtc_rtc2Unix(), same tick formula - without two HAL calls per read. The
 * sample timer stamps every sample with this, from its interrupt.
 *
 * Reading SSR freezes the TR and DR shadow registers until DR is read
 * (BYPSHAD = 0, the default), so the three reads are one consistent snapshot
 * as long as nothing else reads the RTC between them. Interrupts are masked for
 * those three reads only: the HAL path left that window open, and an
 * interrupt reading the clock inside it could pair one second's time with the
 * next second's date. */
uint64_t RTC_get64(void)
{
  SHIM_RTC_t data;
  uint32_t ssr, tr, dr, primask;

  primask = __get_PRIMASK();
  __disable_irq();
  ssr = READ_REG(RTC->SSR);
  tr = READ_REG(RTC->TR);
  dr = READ_REG(RTC->DR);
  __set_PRIMASK(primask);

  data.seconds = RTC_Bcd2ToByte((uint8_t) ((tr & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_SU_Pos));
  data.minutes = RTC_Bcd2ToByte(
      (uint8_t) ((tr & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos));
  data.hours = RTC_Bcd2ToByte((uint8_t) ((tr & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos));
  data.date = RTC_Bcd2ToByte((uint8_t) ((dr & (RTC_DR_DT | RTC_DR_DU)) >> RTC_DR_DU_Pos));
  data.month = RTC_Bcd2ToByte((uint8_t) ((dr & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos));
  data.year = RTC_Bcd2ToByte((uint8_t) ((dr & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos));

  /* SS[14:0] only: after a time-set shift SS[15] is set until SS[14:0] wraps,
   * and the RTC increments TR at that wrap, so SS[15] is not part of the
   * sub-second count (DEV-1162, which applies the same mask in
   * RTC_getDateTime). */
  return ((uint64_t) ShimRtc_rtc2Unix(&data) * 32768) + 32768 - (ssr & 0x7FFFU);
}

uint32_t RTC_get32(void)
{
  return (uint32_t) RTC_get64();
}

void RTC_wakeUpOff(void)
{
  uint32_t irqHeld = RTC_irqHold();
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  RTC_irqRelease(irqHeld);
  //if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 15, RTC_WAKEUPCLOCK_RTCCLK_DIV2) != HAL_OK)
  //{
  //   Error_Handler();
  //}
}

void RTC_wakeUpSet(uint16_t period)
{
  uint16_t prescalar;
  uint32_t irqHeld;

  if (period > 32768)
  {
    Error_Handler();
  }

  irqHeld = RTC_irqHold();
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

  prescalar = period / 2 - 1;

  //changing either prescalar or RTC_WAKEUPCLOCK_RTCCLK_DIV wouldn't change the power consumption.
  //only the product matters: prescalar*RTC_WAKEUPCLOCK_RTCCLK_DIV

  /* HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock,
   uint32_t WakeUpAutoClr)
   uint32_t WakeUpAutoClr missing argument in the old Shimmer4*/
#if defined(SHIMMER3R)
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, prescalar, RTC_WAKEUPCLOCK_RTCCLK_DIV2, 0) != HAL_OK)
#elif defined(SHIMMER4_SDK)
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, prescalar, RTC_WAKEUPCLOCK_RTCCLK_DIV2) != HAL_OK)
#endif
  {
    Error_Handler();
  }
  RTC_irqRelease(irqHeld);
}

void RTC_wakeUpSetSlow(void)
{
  RTC_wakeUpSet(3276);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  ShimSens_sampleTimerTriggered();

  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);
}

/* DEV-1464: Alarm A shares RTC_IRQn with the sample timer and fires on a
 * whole second - every 2 s while docked, for the battery read. A sample that
 * falls due while its callback runs waits for it, and is stamped with the
 * clock as read when it finally runs. The callback used to convert the time
 * with HAL_RTC_GetTime/GetDate and mktime(), and re-arm with gmtime_r() after
 * a 1000-iteration NOP "settle" loop: about 13 ticks at 48 MHz. A 1024 Hz
 * stream whose sample grid sat a few ticks past the second therefore carried
 * one stamp up to 13 ticks late every 2 s. The alarm path now works from
 * RTC_get64(), which the sample stamps use too, and the integer conversion in
 * RTC/shimmer_rtc.c, and RTC_IRQHandler (stm32u5xx_it.c) services the sample
 * timer before the alarms. */
static time_t RTC_nowSeconds(void)
{
  return (time_t) (RTC_get64() / 32768U);
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  __HAL_RTC_ALARMA_DISABLE(hrtc);
  __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRA);

  time_t now = RTC_nowSeconds();

  //Check all alarms
  for (int i = 0; i < RTC_NUM_ALARMS; i++)
  {
    if (nextAlarms[i] != 0 && now >= nextAlarms[i])
    {
      //Alarm is due, handle action
      switch (i)
      {
      case RTC_ALARM_CONTEXT_BATT_READ:
        ShimTask_set(TASK_BATT_READ);
        break;
        //case RTC_ALARM_CONTEXT_BT_SYNC:
        //  RTC_stopSdSyncAlarm();
        //  ShimSdSync_handleSyncTimerTrigger();
        //  RTC_setupAndStartSdSyncAlarm();
        //  break;
      case RTC_ALARM_CONTEXT_AUTO_STOP_RECORDING:
        ShimTask_setStopSensing();
        break;
      case RTC_ALARM_CONTEXT_REBOOT_TO_BOOTLOADER:
        ShimTask_set(TASK_JUMP_TO_BOOT_LOADER);
        break;
      case RTC_ALARM_CONTEXT_REBOOT_AFTER_BT_MODULE_RESTART:
        BT_rebootAfterModuleRestart();
        break;
      default:
        //No action or error log
        break;
      }
      //Clear the alarm
      nextAlarms[i] = 0;
    }
  }

  RTC_setNextRtcAlarmA(hrtc); //Re-setup the next minute alarm
}

void RTC_setAlarmBattRead(void)
{
  battAlarmInterval_t nextBattReadInS = ShimBatt_getBatteryInterval();
  RTC_setAlarmAFromNow((uint32_t) nextBattReadInS, RTC_ALARM_CONTEXT_BATT_READ);
}

void RTC_setAlarmBattReadAfterDockUnDock(void)
{
  /* Set the alarm to read battery a short time after dock/undock to allow
   * charger status to settle. Arbitrarily chosen as 1s delay here. */
  RTC_setAlarmAFromNow(1, RTC_ALARM_CONTEXT_BATT_READ);
}

void RTC_setAlarmAutoStopLogging(uint16_t minutesFromNow)
{
  //Set the alarm to stop logging after a specified number of minutes
  RTC_setAlarmAFromNow(minutesFromNow * 60, RTC_ALARM_CONTEXT_AUTO_STOP_RECORDING);
}

void RTC_setAlarmRebootAfterBtModuleRestart(uint8_t secondsFromNow)
{
  RTC_setAlarmAFromNow(secondsFromNow, RTC_ALARM_CONTEXT_REBOOT_AFTER_BT_MODULE_RESTART);
}

void RTC_setAlarmRebootToBootloader(uint8_t secondsFromNow)
{
  //Set the alarm to reboot to bootloader after a specified time
  RTC_setAlarmAFromNow(secondsFromNow, RTC_ALARM_CONTEXT_REBOOT_TO_BOOTLOADER);
}

void RTC_setNextRtcAlarmA(RTC_HandleTypeDef *hrtc)
{
  //Find the next soonest alarm
  time_t nextAlarmTime = 0;
  int nextAlarmIdx = -1;
  for (int i = 0; i < RTC_NUM_ALARMS; i++)
  {
    if (nextAlarms[i] != 0)
    {
      if (nextAlarmTime == 0 || nextAlarms[i] < nextAlarmTime)
      {
        nextAlarmTime = nextAlarms[i];
        nextAlarmIdx = i;
      }
    }
  }

  if (nextAlarmIdx == -1)
  {
    //no pending alarms
    return;
  }

  //Ensure alarm is in the future
  time_t now = RTC_nowSeconds();
  if (nextAlarmTime <= now)
  {
    nextAlarmTime = now + RTC_MIN_ALARM_OFFSET_SECONDS;
    nextAlarms[nextAlarmIdx] = nextAlarmTime;
  }

  //SHIMMER_PRINTF("RTC: Setting Alarm A for context %d at time %lu (now %lu)\r\n",
  //    nextAlarmIdx, (unsigned long)nextAlarmTime, (unsigned long)now);

  /* The integer conversion the clock set uses (ShimRtc_ticks2Rtc), not
   * gmtime_r() (DEV-1464) */
  SHIM_RTC_t alarmAt;
  ShimRtc_unix2Rtc(&alarmAt, (uint32_t) nextAlarmTime);

  RTC_AlarmTypeDef sAlarm = { 0 };
  sAlarm.AlarmTime.Hours = alarmAt.hours;
  sAlarm.AlarmTime.Minutes = alarmAt.minutes;
  sAlarm.AlarmTime.Seconds = alarmAt.seconds;
  sAlarm.AlarmDateWeekDay = alarmAt.date;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  sAlarm.Alarm = RTC_ALARM_A;

  /* Disable Alarm A and its IRQ before reprogramming. There is nothing to
   * wait for: the U5 RTC has no ALRAWF, and HAL_RTC_SetAlarm_IT writes the
   * alarm registers straight after clearing ALRAE. The 1000-iteration NOP
   * "settle" loop that stood here cost about 7 ticks of sample interrupt
   * latency (DEV-1464). */
  __HAL_RTC_ALARMA_DISABLE(hrtc);
  __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRA);

  HAL_StatusTypeDef ret = HAL_RTC_SetAlarm_IT(hrtc, &sAlarm, RTC_FORMAT_BIN);
  if (ret != HAL_OK && __get_IPSR() == 0U)
  {
    /* Retry once after a short delay, in task context only. In the RTC
     * interrupt a retry cannot succeed (below), and the delay - about 2 ms -
     * would hold off the sample timer for that long (DEV-1464). */
    for (volatile int i = 0; i < RTC_ALARM_RETRY_DELAY_CYCLES; ++i)
    {
      __NOP();
    }
    ret = HAL_RTC_SetAlarm_IT(hrtc, &sAlarm, RTC_FORMAT_BIN);
  }
  if (ret != HAL_OK)
  {
    if (__get_IPSR() != 0U)
    {
      /* DEV-1162: in the RTC interrupt the lock's owner cannot run until we
       * return, so neither a retry nor Error_Handler() can help - the latter
       * froze the device with interrupts masked. Nothing is lost by
       * deferring:
       * - HAL_RTC_AlarmAEventCallback has already queued every due context's
       *   task (stop sensing, jump to bootloader, battery read) before
       *   clearing that context's nextAlarms[] entry.
       * - The contexts not yet due are untouched in nextAlarms[].
       * TASK_BATT_READ re-arms Alarm A from task context, through
       * RTC_setAlarmBattRead -> RTC_setAlarmAFromNow -> RTC_setNextRtcAlarmA,
       * which picks the soonest of them. The only cost is one extra battery
       * read. */
      ShimTask_set(TASK_BATT_READ);
    }
    else
    {
      Error_Handler();
    }
  }
}

void RTC_setAlarmAFromNow(uint32_t secondsFromNow, RTC_AlarmB_Context_t context)
{
  /* Read the clock before taking the hold: RTC_get64() is safe against the
   * sample interrupt on its own, and RTC_IRQn stays held only for the
   * bookkeeping and the alarm write (DEV-1464). The old HAL_RTC_GetTime/
   * GetDate + mktime() conversion ran inside the hold, holding off the sample
   * timer for about as long as the alarm callback did. */
  time_t now = RTC_nowSeconds();
  uint32_t irqHeld = RTC_irqHold();

  if (secondsFromNow == 0)
  {
    nextAlarms[context] = 0; //Clear the alarm if 0 is passed in
  }
  else
  {
    nextAlarms[context] = now + (time_t) secondsFromNow; //The future time for this alarm
  }
  RTC_setNextRtcAlarmA(&hrtc); //Set up the next alarm
  RTC_irqRelease(irqHeld);
}

/* SD sync's one-second ticks counted by Alarm B's interrupt and not yet run by
 * TASK_SD_SYNC_TICK (DEV-1466) */
static volatile uint32_t sdSyncTicksPending;

/* Alarm B is SD sync's one-second tick (SDSync/shimmer_sd_sync.c). It shares
 * RTC_IRQn with the sample timer, whose callback stamps each sample with the
 * clock as read when it runs, so any work in the alarm's callback delays the
 * stamp of a sample falling due meanwhile (DEV-1464, DEV-1466). The alarm is
 * therefore armed once, to fire on every whole second (every calendar field
 * masked, no sub-second comparison: RM0456 RTC alarms), and its callback only
 * counts the tick and queues TASK_SD_SYNC_TICK. The sync state machine runs
 * from that task. It used to run in the callback, and the callback then
 * re-armed the alarm for the next second with HAL_RTC_GetTime/GetDate and
 * HAL_RTC_SetAlarm_IT. On the ticks where the state machine stops the radio
 * (BtStop: UART abort and de-init, pin de-init) or opens a connection
 * (BT_connect), it held the sample timer off for longer than a 4096 Hz sample
 * period. */
void RTC_setupAndStartSdSyncAlarm(void)
{
  /* DEV-1309: zeroed, as for Alarm A. HAL_RTC_SetAlarm_IT() writes every field,
   * not just the ones set below: AlarmSubSecondMask to RTC_ALRMBSSR,
   * AlarmTime.SubSeconds to RTC_ALRBBINR, and FlagAutoClr can set ALRBFCLR.
   * Left as stack garbage they could ask for a sub-second match that never
   * comes, and the alarm never fires: SD sync stopped after its first
   * one-second tick, with no offsets as centre or node. What the garbage was
   * depended on what last used that stack, so unrelated changes turned it on
   * and off (v1.01.022's deeper packet ring did). */
  RTC_AlarmTypeDef sAlarm = { 0 };

  sAlarm.AlarmMask = RTC_ALARMMASK_ALL; //No calendar field compared
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL; //No sub-second match: every second
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 1; //ignored due to mask
  sAlarm.Alarm = RTC_ALARM_B;

  uint32_t irqHeld = RTC_irqHold();
  sdSyncTicksPending = 0U;
  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  RTC_irqRelease(irqHeld);
}

void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
  UNUSED(hrtc);

  /* Count rather than flag: a main loop held for more than a second must not
   * drop a tick, because the sync schedule counts them */
  sdSyncTicksPending++;
  ShimTask_set(TASK_SD_SYNC_TICK);
}

uint32_t RTC_takeSdSyncTicks(void)
{
  uint32_t irqHeld = RTC_irqHold();
  uint32_t ticks = sdSyncTicksPending;
  sdSyncTicksPending = 0U;
  RTC_irqRelease(irqHeld);
  return ticks;
}

void RTC_stopSdSyncAlarm(void)
{
  uint32_t irqHeld = RTC_irqHold();
  __HAL_RTC_ALARMB_DISABLE(hrtc);                //disable Alarm B
  __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRB); //disable Alarm trigger
  sdSyncTicksPending = 0U;                       //and drop any tick not yet run
  RTC_irqRelease(irqHeld);
}

/* DEV-1161: "a host has set this clock since the backup domain last lost
 * power", from the marker RTC_setDateTime() writes. A backup-domain reset
 * clears it along with the calendar (and MX_RTC_Init clears it explicitly), so
 * no date threshold is needed. The old test compared ticks with a millisecond
 * constant (DEV-998) and passed for the 2070 power-loss default regardless. */
uint8_t RTC_isRwcTimeSet(void)
{
  return HAL_RTCEx_BKUPRead(&hrtc, RTC_STATUS_REG) == RTC_STATUS_TIME_OK;
}

/* USER CODE END 1 */
