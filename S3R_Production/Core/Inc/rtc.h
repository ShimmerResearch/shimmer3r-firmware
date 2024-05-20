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
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#include "s4__cfg.h"

/* USER CODE END Includes */

extern RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN Private defines */

/* Private macros */
/* Internal status registers for RTC */
#define RTC_STATUS_REG                  RTC_BKP_DR19 /* Status Register */
#define RTC_STATUS_INIT_OK              0x1234       /* RTC initialised */
#define RTC_STATUS_TIME_OK              0x4321       /* RTC time OK */
#define RTC_STATUS_ZERO                 0x0000

/* Internal RTC defines */
#define RTC_LEAP_YEAR(year)             ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))
#define RTC_DAYS_IN_YEAR(x)             RTC_LEAP_YEAR(x) ? 366 : 365
#define RTC_OFFSET_YEAR                 1970
#define RTC_SECONDS_PER_DAY             86400
#define RTC_SECONDS_PER_HOUR            3600
#define RTC_SECONDS_PER_MINUTE          60
#define RTC_BCD2BIN(x)                  ((((x) >> 4) & 0x0F) * 10 + ((x) & 0x0F))
#define RTC_CHAR2NUM(x)                 ((x) - '0')
#define RTC_CHARISNUM(x)                ((x) >= '0' && (x) <= '9')


typedef struct {
  uint8_t seconds;     /*!< Seconds parameter, from 00 to 59 */
  uint16_t subseconds; /*!< Subsecond downcounter. When it reaches zero, it's reload value is the same as
                                 @ref RTC_SYNC_PREDIV, so in our case 0x3FF = 1023, 1024 steps in one second */
  uint8_t minutes;     /*!< Minutes parameter, from 00 to 59 */
  uint8_t hours;       /*!< Hours parameter, 24Hour mode, 00 to 23 */
  uint8_t weekday;         /*!< Day in a week, from 1 to 7 */
  uint8_t date;        /*!< Date in a month, 1 to 31 */
  uint8_t month;       /*!< Month in a year, 1 to 12 */
  uint8_t year;        /*!< Year parameter, 00 to 99, 00 is 2000 and 99 is 2099 */
  uint32_t unix;       /*!< Seconds from 01.01.1970 00:00:00 */
  uint64_t ticks;      /*!< ticks from 01.01.1970 00:00:00 */
} S4_RTC_t;

/* USER CODE END Private defines */

void MX_RTC_Init(void);

/* USER CODE BEGIN Prototypes */
void S4_RTC_Init(void);//RTC_HandleTypeDef *hrtc
uint8_t S4_RTC_SetDateTime(S4_RTC_t* data);
void S4_RTC_GetDateTime(S4_RTC_t* data) ;
uint32_t S4_RTC_RTC2Unix(S4_RTC_t* data) ;
void S4_RTC_Unix2RTC(S4_RTC_t* data, uint32_t unix);

void RTC_init(uint64_t ticks);
uint32_t RTC_get32(void);
uint64_t RTC_get64(void);
uint8_t isRwcTimeSet(void);

void S4_RWC_setConfigTime(uint64_t val);
uint64_t S4_RWC_getConfigTime(void);

void S4_RTC_WakeUpOff(void);
void S4_RTC_WakeUpSet(uint16_t period);
void S4_RTC_WakeUpSetSlow(void);
void enableRTCAlarm(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H__ */

