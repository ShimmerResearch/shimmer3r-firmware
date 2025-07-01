/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rtc.h
 * @brief   This file contains all the function prototypes for
 *          the rtc.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* USER CODE BEGIN Includes */

#include <RTC/shimmer_rtc.h>
#include <shimmer_include.h>

  /* USER CODE END Includes */

  extern RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN Private defines */

/* Private macros */
/* Internal status registers for RTC */
#define RTC_STATUS_REG     RTC_BKP_DR19 /* Status Register */
#define RTC_STATUS_INIT_OK 0x1234       /* RTC initialised */
#define RTC_STATUS_TIME_OK 0x4321       /* RTC time OK */
#define RTC_STATUS_ZERO    0x0000

  typedef enum
  {
    RTC_ALARM_CONTEXT_NONE = 0,
    RTC_ALARM_CONTEXT_BATT_READ,
    //RTC_ALARM_CONTEXT_BT_SYNC,
    RTC_ALARM_CONTEXT_AUTO_STOP_RECORDING,
    RTC_ALARM_CONTEXT_REBOOT_TO_BOOTLOADER,
    RTC_NUM_ALARMS
  } RTC_AlarmB_Context_t;

  /* USER CODE END Private defines */

  void MX_RTC_Init(void);

  /* USER CODE BEGIN Prototypes */

  uint8_t RTC_setDateTime(SHIM_RTC_t *data);
  void RTC_getDateTime(SHIM_RTC_t *data);

  void RTC_setTimeFromTicksPtr(uint8_t *ticksPtr);
  void RTC_setTimeFromTicks(uint64_t ticks);
  uint64_t RTC_get64(void);
  uint32_t RTC_get32(void);

  void RTC_wakeUpOff(void);
  void RTC_wakeUpSet(uint16_t period);
  void RTC_wakeUpSetSlow(void);

  void RTC_setAlarmBattRead(void);
  void RTC_setAlarmAutoStopLogging(uint16_t minutesFromNow);
  void RTC_setAlarmRebootToBootloader(uint8_t secondsFromNow);

  void RTC_setNextRtcAlarmA(RTC_HandleTypeDef *hrtc);
  void RTC_setAlarmAFromNow(uint32_t secondsFromNow, RTC_AlarmB_Context_t context);

  void RTC_setupAndStartSdSyncAlarm(void);
  void RTC_stopSdSyncAlarm(void);
  uint8_t RTC_isRwcTimeSet(void);

  /* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H__ */
