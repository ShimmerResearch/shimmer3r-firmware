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

/* DEV-1161: move the running clock onto the sub-second part of a time-set.
 *
 * RTC_SSR is read-only on the STM32U5 (RM0456 RTC_SSR; the SVD agrees), so the
 * fraction cannot be written directly - the old `SSR = ...` store did nothing
 * and every set was truncated to the whole second. Leaving init mode restarts
 * the calendar at the top of the second, so instead read back what the clock
 * now says and shift it by the difference with RTC_SHIFTR (RM0456 "RTC
 * synchronization"):
 *   delay   by SUBFS / (PREDIV_S + 1) s           (ADD1S = 0)
 *   advance by 1 - SUBFS / (PREDIV_S + 1) s       (ADD1S = 1)
 * PREDIV_S + 1 = 32768 here, so one SUBFS step is one tick.
 *
 * RM0456 constraints: no shift may be pending (SHPF = 0, HAL waits for it),
 * REFCKON must be 0 (HAL refuses otherwise; it is never set here), and SS[15]
 * must be 0 so the addition cannot overflow - always true straight after
 * SetTime, because SSR then holds PREDIV_S = 0x7FFF at most. */
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
   * second, so leave the clock on the whole second instead. */
  if (delta == 0 || delta > (int64_t) RTC_SHIFTR_SUBFS
      || -delta > (int64_t) RTC_SHIFTR_SUBFS || (hrtc.Instance->SSR & 0x8000U) != 0U)
  {
    return;
  }
  if (delta > 0)
  {
    status = HAL_RTCEx_SetSynchroShift(
        &hrtc, RTC_SHIFTADD1S_SET, (uint32_t) (32768 - delta));
  }
  else
  {
    status = HAL_RTCEx_SetSynchroShift(&hrtc, RTC_SHIFTADD1S_RESET, (uint32_t) (-delta));
  }

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

  data->ticks = ((uint64_t) data->unix * 32768) + 32768 - data->subseconds;
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

  return ((uint64_t) ShimRtc_rtc2Unix(&data) * 32768) + 32768 - ssr;
}

uint32_t RTC_get32(void)
{
  return (uint32_t) RTC_get64();
}

void RTC_wakeUpOff(void)
{
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  //if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 15, RTC_WAKEUPCLOCK_RTCCLK_DIV2) != HAL_OK)
  //{
  //   Error_Handler();
  //}
}

void RTC_wakeUpSet(uint16_t period)
{
  uint16_t prescalar;

  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  if (period > 32768)
  {
    Error_Handler();
  }

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

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  __HAL_RTC_ALARMA_DISABLE(hrtc);
  __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRA);

  //Get current time
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  HAL_RTC_GetTime(hrtc, &sTime, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BIN);

  struct tm current_tm = { .tm_sec = sTime.Seconds,
    .tm_min = sTime.Minutes,
    .tm_hour = sTime.Hours,
    .tm_mday = sDate.Date,
    .tm_mon = sDate.Month - 1,
    .tm_year = sDate.Year + 100 };
  time_t now = mktime(&current_tm);

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
  time_t now = (time_t) RTC_get64() / 32768; //approx seconds from rtc64
  if (nextAlarmTime <= now)
  {
    nextAlarmTime = now + RTC_MIN_ALARM_OFFSET_SECONDS;
    nextAlarms[nextAlarmIdx] = nextAlarmTime;
  }

  //SHIMMER_PRINTF("RTC: Setting Alarm A for context %d at time %lu (now %lu)\r\n",
  //    nextAlarmIdx, (unsigned long)nextAlarmTime, (unsigned long)now);

  struct tm alarm_tm;
  time_t t = nextAlarmTime;
  gmtime_r(&t, &alarm_tm);

  RTC_AlarmTypeDef sAlarm = { 0 };
  sAlarm.AlarmTime.Hours = alarm_tm.tm_hour;
  sAlarm.AlarmTime.Minutes = alarm_tm.tm_min;
  sAlarm.AlarmTime.Seconds = alarm_tm.tm_sec;
  sAlarm.AlarmDateWeekDay = alarm_tm.tm_mday;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  sAlarm.Alarm = RTC_ALARM_A;

  //Disable Alarm A and its IRQ before reprogramming
  __HAL_RTC_ALARMA_DISABLE(hrtc);
  __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRA);
  //Small barrier to allow hardware to settle
  for (volatile int i = 0; i < 1000; ++i)
  {
    __NOP();
  }

  HAL_StatusTypeDef ret = HAL_RTC_SetAlarm_IT(hrtc, &sAlarm, RTC_FORMAT_BIN);
  if (ret != HAL_OK)
  {
    //retry once after a short delay
    for (volatile int i = 0; i < RTC_ALARM_RETRY_DELAY_CYCLES; ++i)
    {
      __NOP();
    }
    ret = HAL_RTC_SetAlarm_IT(hrtc, &sAlarm, RTC_FORMAT_BIN);
    if (ret != HAL_OK)
    {
      Error_Handler();
    }
  }
}

void RTC_setAlarmAFromNow(uint32_t secondsFromNow, RTC_AlarmB_Context_t context)
{
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  if (secondsFromNow == 0)
  {
    nextAlarms[context] = 0; //Clear the alarm if 0 is passed in
  }
  else
  {
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); //Must be called after GetTime()

    struct tm current_tm = {
      .tm_sec = sTime.Seconds,
      .tm_min = sTime.Minutes,
      .tm_hour = sTime.Hours,
      .tm_mday = sDate.Date,
      .tm_mon = sDate.Month - 1,  //struct tm uses 0-11 for months
      .tm_year = sDate.Year + 100 //struct tm uses years since 1900
    };

    //Add offset
    time_t future_time = mktime(&current_tm) + secondsFromNow;
    nextAlarms[context] = future_time; //Store the future time for this alarm
  }
  RTC_setNextRtcAlarmA(&hrtc); //Set up the next alarm
}

void RTC_setupAndStartSdSyncAlarm(void)
{
  RTC_AlarmTypeDef sAlarm;
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  /* Get time added since it was randomly missing interrupt when using HAL_RTC_GetAlarm().*/
  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  /* From GetTime() notes : You must call HAL_RTC_GetDate() after HAL_RTC_GetTime() to unlock the values....*/
  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  uint8_t sec = sTime.Seconds + 1;
  uint8_t min = sTime.Minutes;
  uint8_t hr = sTime.Hours;

  if (sec >= 60)
  {
    sec = 0;
    min++;
    if (min >= 60)
    {
      min = 0;
      hr++;
      if (hr >= 24)
      {
        hr = 0;
      }
    }
  }

  sAlarm.AlarmTime.Hours = hr;
  sAlarm.AlarmTime.Minutes = min;
  sAlarm.AlarmTime.Seconds = sec;

  sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY; //Match Hours, Minutes, Seconds
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 1; //ignored due to mask
  sAlarm.Alarm = RTC_ALARM_B;

  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
  //stopAlarm(); //stopping from triggering the Alarm multiple times.
  //HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0); //toggle to confirm the callback
  //printf("Alarm B fired!\r\n");           // Debug print
  //__HAL_RTC_ALARMB_DISABLE(&hrtc);
  //__HAL_RTC_ALARM_DISABLE_IT(&hrtc, RTC_IT_ALRB);
  //RTC_setupAndStartSdSyncAlarm(); //for the next interval (testing purposes)

  RTC_stopSdSyncAlarm();

  ShimSdSync_handleSyncTimerTrigger();

  RTC_setupAndStartSdSyncAlarm();
}

void RTC_stopSdSyncAlarm(void)
{
  __HAL_RTC_ALARMB_DISABLE(hrtc);                //disable Alarm B
  __HAL_RTC_ALARM_DISABLE_IT(hrtc, RTC_IT_ALRB); //disable Alarm trigger
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
