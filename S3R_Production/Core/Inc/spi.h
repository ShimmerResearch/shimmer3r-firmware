/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.h
  * @brief   This file contains all the function prototypes for
  *          the spi.c file
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
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#include "s4.h"
#include "s4__cfg.h"

/* USER CODE END Includes */

extern SPI_HandleTypeDef hspi1;

extern SPI_HandleTypeDef hspi2;

extern SPI_HandleTypeDef hspi3;

/* USER CODE BEGIN Private defines */

#if defined(SHIMMER3R)
typedef enum
{
  SPI1_CHIP_INDEX_LSM6DSV,
  SPI1_CHIP_INDEX_ADXL371,
  SPI1_CHIP_INDEX_BMP390,
  SPI1_CHIP_QTY
} SPI1_CHIP_INDEX;

typedef enum
{
  SPI2_CHIP_INDEX_LSM303AH,
  SPI2_CHIP_INDEX_LIS3MDL,
  SPI2_CHIP_QTY
} SPI2_CHIP_INDEX;

typedef enum
{
  SPI3_CHIP_INDEX_ADS1292R,
  SPI3_CHIP_QTY
} SPI3_CHIP_INDEX;
#endif

/* USER CODE END Private defines */

void MX_SPI1_Init(void);
void MX_SPI2_Init(void);
void MX_SPI3_Init(void);

/* USER CODE BEGIN Prototypes */

void SPI_init(void);
uint8_t SPI_test(void);
void SPI_configureChannels(void);
void SPI_startSensing(void);
void SPI_pollSensors(void);
void SPI_stopSensing(void);

void SPI_gatherDataCb(void (*done_cb)(void));
void SPI_gatherDataStart(void);
void SpiStep1Start(void);
void SpiStep2Start(void);
void SpiStep3Start(void);
void SpiStepDone(void);

#if defined(SHIMMER3R)
void set_power_spi1_bus(bool state, SPI1_CHIP_INDEX chipIndex);
void set_power_spi2_bus(bool state, SPI2_CHIP_INDEX chipIndex);
#endif

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */

