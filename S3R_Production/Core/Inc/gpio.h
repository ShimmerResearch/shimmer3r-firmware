/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    gpio.h
 * @brief   This file contains all the function prototypes for
 *          the gpio.c file
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
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* USER CODE BEGIN Includes */

#include "shimmer_definitions.h"
#include <shimmer_include.h>

  /* USER CODE END Includes */

  /* USER CODE BEGIN Private defines */

  /* USER CODE END Private defines */

  void MX_GPIO_Init(void);

  /* USER CODE BEGIN Prototypes */

  void GPIO_usbVbusInputInit(void);
  void GPIO_usbVbusIntInit(uint8_t state);
  void GPIO_userButtonCheck(void);

#if defined(SHIMMER3R)
  void gpioExtiCommon(uint16_t GPIO_Pin, uint8_t isRising);
#endif

  uint8_t SD_insertedCheck(void);
  void SdPowerOn(void);
  void SdPowerOff(void);
  uint8_t isSdPowerOn(void);
  void setMcuHasSdcardControl(uint8_t state);
  void vbusPinStateCheck(void);

  /* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */
