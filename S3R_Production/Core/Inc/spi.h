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
extern "C"
{
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

#define SPI_DMA_TXRX_OFFSET 1
#define SPI_READ_REGISTER   0x80

#if defined(SHIMMER3R)
  typedef enum
  {
    SPI_BUSES_ENABLED_NONE,
    SPI1_BUS_FLAG = 0x01,
    SPI2_BUS_FLAG = 0x02,
    SPI3_BUS_FLAG = 0x04
  } SPI_BUS_INDEX;

  //typedef enum
  //{
  //  SPI1_CHIP_INDEX_LSM6DSV,
  //  SPI1_CHIP_INDEX_ADXL371,
  //  SPI1_CHIP_INDEX_BMP390,
  //  SPI1_CHIP_QTY,
  //  SPI1_CHIP_ALL
  //} SPI1_CHIP_INDEX;
  //
  //typedef enum
  //{
  //  SPI2_CHIP_INDEX_LIS2DW12,
  //  SPI2_CHIP_INDEX_LIS3MDL,
  //  SPI2_CHIP_QTY,
  //  SPI2_CHIP_ALL
  //} SPI2_CHIP_INDEX;
  //
  //typedef enum
  //{
  //  SPI3_CHIP_INDEX_ADS1292R,
  //  SPI3_CHIP_QTY
  //} SPI3_CHIP_INDEX;

  typedef enum
  { //i2c
    SPI1_LSM6DSV_ACCEL = 0,
    SPI1_LSM6DSV_GYRO,
    SPI1_ADXL371_ACCEL,
    SPI1_BMP390_PRESSURE_TEMP,
    SPI2_LIS2DW12_ACCEL,
    SPI2_LIS3MDL_MAG,
    SPI3_ADS1292R_EXG1,
    SPI3_ADS1292R_EXG2
  } SPI_SENSOR;

  typedef enum
  {
    SPI_STAT_IDLE = 0,
    SPI_STAT_LSM6DSV_STATUS_GET,
    SPI_STAT_LSM6DSV_ACCEL_GET,
    SPI_STAT_LSM6DSV_GYRO_GET,
    SPI_STAT_ADXL371_ACCEL_GET,
    SPI_STAT_BMP390_PRESSURE_TEMPERATURE_GET,
    SPI_STAT_LIS2DW12_ACCEL_GET,
    SPI_STAT_LIS3MDL_MAG_GET,
    SPI_STAT_ADS1292R_EXG1_GET,
    SPI_STAT_ADS1292R_EXG2_GET
  } SPI_STATUS;

  typedef enum
  {
    SPI_FIRST_SENSOR = 0,
    SPI_NEXT_SENSOR,
  } SPI_SENSING_TYPE;

  typedef struct
  {
    uint8_t lsm6dsvAccelBuf[SPI_DMA_TXRX_OFFSET + 6];
    uint8_t lsm6dsvGyroBuf[SPI_DMA_TXRX_OFFSET + 6];
    uint8_t adxl371Buf[SPI_DMA_TXRX_OFFSET + 6];
    uint8_t bmp390Buf[SPI_DMA_TXRX_OFFSET + 6 + 1]; //+1 for BMP390 dummy byte
  } spi1ReadBuf;

  typedef struct
  {
    uint8_t lis2dw12AccelBuf[SPI_DMA_TXRX_OFFSET + 6];
    uint8_t lis3mdlMagBuf[SPI_DMA_TXRX_OFFSET + 6];
  } spi2ReadBuf;

  typedef struct
  {
    uint8_t ads1292rExg1Buf[SPI_DMA_TXRX_OFFSET + 7];
    uint8_t ads1292rExg2Buf[SPI_DMA_TXRX_OFFSET + 7];
  } spi3ReadBuf;

  typedef struct
  { //spi - Sensors
    SPI_STATUS status;
    SPI_SENSOR sensorList[20];
    uint8_t sensorLen;
    uint8_t sensorCnt;
    SPI_BUS_INDEX busId;
  } SPITypeDef;

#endif

  /* USER CODE END Private defines */

  void MX_SPI1_Init(void);
  void MX_SPI2_Init(void);
  void MX_SPI3_Init(void);

  /* USER CODE BEGIN Prototypes */

  void SPI_init(void);
  void SPI_configureChannels(void);
  void SPI_startSensing(void);
  void SPI_pollSensors(void);
  void SPI_stopSensing(void);

  void SPI_gatherDataCb(void (*done_cb)(void));
#if defined(SHIMMER3R)
  void SPI_busGatherDataDone_cb(uint8_t flag);
#elif defined(SHIMMER4_SDK)
void SPI_gatherDataStart(void);
void SpiStep1Start(void);
void SpiStep2Start(void);
void SpiStep3Start(void);
void SpiStepDone(void);
#endif

#if defined(SHIMMER3R)
  void SpiSensing(SPITypeDef *spiSensingInfo, SPI_SENSING_TYPE start);
  void SpiSens_sensorNext(SPITypeDef *spiSensingInfo);

  void SPI1_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
  void SPI2_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
  void SPI3_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
  void SPI3_TxCpltCallback(SPI_HandleTypeDef *hspi);
  void SPI3_RxCpltCallback(SPI_HandleTypeDef *hspi);
  void SPI_ErrorCallback(SPI_HandleTypeDef *hspi);

  bool areSpiChannelsEnabled(void);
#endif

  /* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */
