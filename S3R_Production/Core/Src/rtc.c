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

uint64_t rtcConfigTime64;
uint32_t S4_RTC_Status = RTC_STATUS_ZERO;

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_PrivilegeStateTypeDef privilegeState = {0};
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

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

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
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

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
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

void     S4_RTC_setConfigTime(uint64_t val){   rtcConfigTime64 = val;}// 64bits = 8bytes
uint64_t S4_RTC_getConfigTime(void)        {   return rtcConfigTime64;}

/* Days in a month */
uint8_t S4_RTC_Months[2][12] = {
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, /* Not leap year */
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}  /* Leap year */
};
uint64_t rtc64_reg;

void S4_RTC_Init(){//  RTC_HandleTypeDef *hrtc


   //"How to know after power start whether the product up from a stand by mode or a power down reset. "
   uint32_t lastRstState = RCC->CSR;
   // code to detect a warm reboot
   if ( RCC_CSR_SFTRSTF & lastRstState )
   {
      //uartPrintf("\n\r Warm boot (reset caused by software)" );
   }
   else
   {
      //uartPrintf("\n\r Cold boot (Powered from Off)" );
   }


   //s4rtc_hrtc = hrtc;
   S4_RTC_t data;

   rtcConfigTime64 = 0;

   hrtc.Instance = RTC;

   //HAL_RTC_DeInit(s4rtc_hrtc);

   hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
   hrtc.Init.AsynchPrediv = 0;
   hrtc.Init.SynchPrediv = 32767;
   hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
   hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
   hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
   HAL_RTC_Init(&hrtc);

   S4_RTC_GetDateTime(&data);
#ifdef RTC_FAST
   rtc64_reg = data.ticks & 0xffffffffffff8000;
#endif

//   /**Enable the Alarm A
//   * fires every second
//   */
////   RTC_AlarmTypeDef sAlarm;
////   sAlarm.AlarmTime.Hours = 0x0;
////   sAlarm.AlarmTime.Minutes = 0x0;
////   sAlarm.AlarmTime.Seconds = 0x0;
////   sAlarm.AlarmTime.SubSeconds = 0x0;
////   sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
////   sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
////   sAlarm.AlarmMask = RTC_ALARMMASK_ALL;
////   sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
////   sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
////   sAlarm.AlarmDateWeekDay = 1;
////   sAlarm.Alarm = RTC_ALARM_A;
////   HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD);
//

   HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
//   if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 15, RTC_WAKEUPCLOCK_RTCCLK_DIV2) != HAL_OK)
//   {
//      Error_Handler();
//   }

//   S4_RTC_Status = RTC_STATUS_INIT_OK;
//   HAL_RTCEx_BKUPWrite(&hrtc, RTC_STATUS_REG, S4_RTC_Status);
}


void S4_RTC_WakeUpOff(){
   HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
//   if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 15, RTC_WAKEUPCLOCK_RTCCLK_DIV2) != HAL_OK)
//   {
//      Error_Handler();
//   }
}
void S4_RTC_WakeUpSet(uint16_t period){
   uint16_t prescalar;

   HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
   if(period>32768) {
      Error_Handler();
   }

   prescalar = period/2 - 1;

   // changing either prescalar or RTC_WAKEUPCLOCK_RTCCLK_DIV wouldn't change the power consumption.
   // only the product matters: prescalar*RTC_WAKEUPCLOCK_RTCCLK_DIV

  /* HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock,
   uint32_t WakeUpAutoClr)
   uint32_t WakeUpAutoClr missing argument in the old Shimmer4*/
#if defined(SHIMMER3R)
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, prescalar, RTC_WAKEUPCLOCK_RTCCLK_DIV2, 0) != HAL_OK)
#else
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, prescalar, RTC_WAKEUPCLOCK_RTCCLK_DIV2) != HAL_OK)
#endif
  {
    Error_Handler();
  }
}
void S4_RTC_WakeUpSetSlow(){
   S4_RTC_WakeUpSet(3276);
}


uint8_t S4_RTC_SetDateTime(S4_RTC_t* data) {
   uint32_t format = RTC_FORMAT_BIN;
  //S4_RTC_t tmp;
   RTC_TimeTypeDef sTime;
   RTC_DateTypeDef sDate;

  /* Check date and time validation */
//  if (format == RTC_FORMAT_BCD) {
//    tmp.date = RTC_BCD2BIN(data->date);
//    tmp.month = RTC_BCD2BIN(data->month);
//    tmp.year = RTC_BCD2BIN(data->year);
//    tmp.weekday = RTC_BCD2BIN(data->weekday);
//    tmp.hours = RTC_BCD2BIN(data->hours);
//    tmp.minutes = RTC_BCD2BIN(data->minutes);
//    tmp.seconds = RTC_BCD2BIN(data->seconds);
//  } else {
//    tmp.date = data->date;
//    tmp.month = data->month;
//    tmp.year = data->year;
//    tmp.weekday = data->weekday;
//    tmp.hours = data->hours;
//    tmp.minutes = data->minutes;
//    tmp.seconds = data->seconds;
//  }

  /* Check year and month */
  if (
    data->year > 99 ||
    data->month == 0 ||
    data->month > 12 ||
    data->date == 0 ||
    data->date > S4_RTC_Months[RTC_LEAP_YEAR(2000 + data->year) ? 1 : 0][data->month - 1] ||
    data->weekday == 0 ||
    data->weekday > 7 ||
    data->hours > 23 ||
    data->minutes > 59 ||
    data->seconds > 59
  ) {
    /* Invalid date */
    return 1;
  }
    /**Initialize RTC and set the Time and Date
    */
  S4_RTC_Status = HAL_RTCEx_BKUPRead(&hrtc, RTC_STATUS_REG);

   if(S4_RTC_Status == RTC_STATUS_ZERO){
      S4_RTC_Init();
   }

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

   hrtc.Instance->SSR = 32768 - (data->ticks%32768);
//    /**Enable the Alarm A
//    */
//  sAlarm.AlarmTime.Hours = 0x0;
//  sAlarm.AlarmTime.Minutes = 0x0;
//  sAlarm.AlarmTime.Seconds = 0x0;
//  sAlarm.AlarmTime.SubSeconds = 0x0;
//  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
//  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
//  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
//  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
//  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
//  sAlarm.AlarmDateWeekDay = 0x1;
//  sAlarm.Alarm = RTC_ALARM_A;
//  HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD);


   /* Write backup registers */
   S4_RTC_Status = RTC_STATUS_TIME_OK;
   HAL_RTCEx_BKUPWrite(&hrtc, RTC_STATUS_REG, RTC_STATUS_TIME_OK);

  /* Return OK */
  return 0;
}

void S4_RTC_GetDateTime(S4_RTC_t* data) {
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
  unix = S4_RTC_RTC2Unix(data);
  data->unix = unix;

   data->ticks = ((uint64_t)data->unix * 32768) + 32768 - data->subseconds;
}

uint32_t S4_RTC_RTC2Unix(S4_RTC_t* data) {
  uint32_t days = 0, seconds = 0;
  uint16_t i;
  uint16_t year = (uint16_t) (data->year + 2000);
  /* Year is below offset year */
  if (year < RTC_OFFSET_YEAR) {
    return 0;
  }
  /* Days in back years */
  for (i = RTC_OFFSET_YEAR; i < year; i++) {
    days += RTC_DAYS_IN_YEAR(i);
  }
  /* Days in current year */
  for (i = 1; i < data->month; i++) {
    days += S4_RTC_Months[RTC_LEAP_YEAR(year)][i - 1];
  }
  /* Day starts with 1 */
  days += data->date - 1;
  seconds = days * RTC_SECONDS_PER_DAY;
  seconds += data->hours * RTC_SECONDS_PER_HOUR;
  seconds += data->minutes * RTC_SECONDS_PER_MINUTE;
  seconds += data->seconds;

  /* seconds = days * 86400; */
  return seconds;
}


void S4_RTC_Unix2RTC(S4_RTC_t* data, uint32_t unix) {
  uint16_t year;

  /* Store unix time to unix in struct */
  data->unix = unix;
  /* Get seconds from unix */
  data->seconds = unix % 60;
  /* Go to minutes */
  unix /= 60;
  /* Get minutes */
  data->minutes = unix % 60;
  /* Go to hours */
  unix /= 60;
  /* Get hours */
  data->hours = unix % 24;
  /* Go to days */
  unix /= 24;

  /* Get week day */
  /* Monday is day one */
  data->weekday = (unix + 3) % 7 + 1;

  /* Get year */
  year = 1970;
  while (1) {
    if (RTC_LEAP_YEAR(year)) {
      if (unix >= 366) {
        unix -= 366;
      } else {
        break;
      }
    } else if (unix >= 365) {
      unix -= 365;
    } else {
      break;
    }
    year++;
  }
  /* Get year in xx format */
  data->year = (uint8_t) (year - 2000);
  /* Get month */
  for (data->month = 0; data->month < 12; data->month++) {
    if (RTC_LEAP_YEAR(year)) {
      if (unix >= (uint32_t)S4_RTC_Months[1][data->month]) {
        unix -= S4_RTC_Months[1][data->month];
      } else {
        break;
      }
    } else if (unix >= (uint32_t)S4_RTC_Months[0][data->month]) {
      unix -= S4_RTC_Months[0][data->month];
    } else {
      break;
    }
  }
  /* Get month */
  /* Month starts with 1 */
  data->month++;
  /* Get date */
  /* Date starts with 1 */
  data->date = unix + 1;
}


void S4_RTC_Ticks2RTC(S4_RTC_t* data, uint64_t ticks) {
  uint16_t year;
   uint32_t unix;

  data->ticks = ticks;
  /* Store unix time to unix in struct */
   unix = ticks/32768;
  data->unix = unix;
  /* Get seconds from unix */
  data->seconds = unix % 60;
  /* Go to minutes */
  unix /= 60;
  /* Get minutes */
  data->minutes = unix % 60;
  /* Go to hours */
  unix /= 60;
  /* Get hours */
  data->hours = unix % 24;
  /* Go to days */
  unix /= 24;

  /* Get week day */
  /* Monday is day one */
  data->weekday = (unix + 3) % 7 + 1;

  /* Get year */
  year = 1970;
  while (1) {
    if (RTC_LEAP_YEAR(year)) {
      if (unix >= 366) {
        unix -= 366;
      } else {
        break;
      }
    } else if (unix >= 365) {
      unix -= 365;
    } else {
      break;
    }
    year++;
  }
  /* Get year in xx format */
  data->year = (uint8_t) (year - 2000);
  /* Get month */
  for (data->month = 0; data->month < 12; data->month++) {
    if (RTC_LEAP_YEAR(year)) {
      if (unix >= (uint32_t)S4_RTC_Months[1][data->month]) {
        unix -= S4_RTC_Months[1][data->month];
      } else {
        break;
      }
    } else if (unix >= (uint32_t)S4_RTC_Months[0][data->month]) {
      unix -= S4_RTC_Months[0][data->month];
    } else {
      break;
    }
  }
  /* Get month */
  /* Month starts with 1 */
  data->month++;
  /* Get date */
  /* Date starts with 1 */
  data->date = unix + 1;
}


void RTC_init(uint64_t ticks){
   S4_RTC_t data;
   S4_RTC_Ticks2RTC(&data, ticks);
   S4_RTC_SetDateTime(&data);
#if RTC_FAST
   rtc64_reg = data.ticks & 0xffffffffffff8000;
#endif
   S4_RTC_setConfigTime(ticks);
}

uint64_t RTC_get64(void){
#if RTC_FAST
   uint64_t t1, t2;
   t1 = rtc64_reg + 0x8000 - (uint32_t)(hrtc.Instance->SSR);
   do{
      t2 = t1;
      t1 = rtc64_reg + 0x8000 - (uint32_t)(hrtc.Instance->SSR);
   }while(t1 != t2);
   return t1;
#else
   S4_RTC_t data;
   S4_RTC_GetDateTime(&data);
   return data.ticks;
#endif
}

uint32_t RTC_get32(void){
#if RTC_FAST
   uint64_t t1, t2;
   t1 = rtc64_reg + 0x8000 - (uint32_t)(hrtc.Instance->SSR);
   do{
      t2 = t1;
      t1 = rtc64_reg + 0x8000 - (uint32_t)(hrtc.Instance->SSR);
   }while(t1 != t2);
   return t1;
#else
   S4_RTC_t data;
   S4_RTC_GetDateTime(&data);
   return data.ticks;
#endif
}


void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
//   static uint8_t green0_cnt = 0;

   if(stat.isSensing && !stat.isConfiguring){
//      if(!green0_cnt++){
//         Board_ledToggle(LED_GREEN0);
//      }
#if !SENS_CLK_RTC0TIM1
      S4Sens_gatherData();
#endif
   }else{
      S4Led_Blink();
   }
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrtc);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_RTC_WakeUpTimerEventCallback could be implemented in the user file
   */
}
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
   //Board_ledToggle(LED_GREEN0);
#if RTC_FAST
   //rtc64_reg += 0x8000; // this is not working well as the interrupt priority is not the highest
#endif
}

/* USER CODE END 1 */
