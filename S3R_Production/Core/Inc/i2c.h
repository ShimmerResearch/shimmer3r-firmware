/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.h
  * @brief   This file contains all the function prototypes for
  *          the i2c.c file
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
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#include "s4.h"
#include "s4__cfg.h"

/* USER CODE END Includes */

extern I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN Private defines */

typedef enum{//i2c
   I2C_STAT_IDLE = 0,
   I2C_STAT_LSM303DLHC_ACCEL_GET_T,
   I2C_STAT_LSM303DLHC_ACCEL_GET_R,
   I2C_STAT_LSM303DLHC_MAG_GET_T,
   I2C_STAT_LSM303DLHC_MAG_GET_R,
   I2C_STAT_MPU9250_GYRO_GET_T,
   I2C_STAT_MPU9250_GYRO_GET_R,
   I2C_STAT_MPU9250_ACCEL_GET_T,
   I2C_STAT_MPU9250_ACCEL_GET_R,
   I2C_STAT_MPU9250_MAG_START,
   I2C_STAT_MPU9250_MAG_GET_T,
   I2C_STAT_MPU9250_MAG_GET_R,
   I2C_STAT_BMP180_TEMP_START,
   I2C_STAT_BMP180_TEMP_GET_T,
   I2C_STAT_BMP180_TEMP_GET_R,
   I2C_STAT_BMP180_PRES_START,
   I2C_STAT_BMP180_PRES_GET_T,
   I2C_STAT_BMP180_PRES_GET_R,
   I2C_STAT_BMP280_START,
   I2C_STAT_BMP280_TEMP_GET_T,
   I2C_STAT_BMP280_TEMP_GET_R,
   I2C_STAT_BMP280_PRES_GET_T,
   I2C_STAT_BMP280_PRES_GET_R,
   I2C_STAT_STC3100_START,
   I2C_STAT_STC3100_ALL_GET,
   I2C_STAT_STC3100_DATA_GET,
}I2C_STATUS;

typedef enum {
   I2C_FIRST_SENSOR = 0,
   I2C_NEXT_SENSOR,
}I2C_SENSING_TYPE;

typedef enum{//i2c
   I2C_LSM303DLHC_ACCEL = 0,
   I2C_LSM303DLHC_MAG,
   I2C_ANALOG_ACCEL,
   I2C_MPU9250_GYRO,
   I2C_MPU9250_ACCEL,
   I2C_MPU9250_MAG,
   I2C_BMP180,
   I2C_BMP280,
   I2C_STC3100,
}I2C_SENSOR;

typedef struct {//i2c_1 - Sensors
   I2C_STATUS status;
   uint8_t rxBuf[130];
   uint8_t sensorList[20];
   uint8_t sensorLen;
   uint8_t sensorCnt;
} I2CTypeDef;
//typedef struct {//i2c_2 - BatteryMonitor
//   I2C_STATUS status;
//   uint8_t rxBuf[20];
//   uint8_t sensorList[20];
//   uint8_t sensorLen;
//   uint8_t sensorCnt;
//} I2CBatteryTypeDef;

typedef struct {//bmp180
   uint8_t en;
   //uint16_t freq;
   uint16_t preSampleClkInterval;
   uint16_t oss;
   //uint16_t preSample;
   uint16_t presMax;
   uint16_t presCnt;
   uint16_t tempMax;
   uint16_t tempCnt;
} BMP180TypeDef;


typedef struct {//bmp280
   uint8_t en;
   uint16_t preSampleClkInterval;
   uint16_t oss;
   uint16_t presMax;
   uint16_t presCnt;
   uint16_t tempMax;
   uint16_t tempCnt;
} BMP280TypeDef;

typedef struct {//mpu9250Mag
   uint8_t en;
   //uint16_t freq;
   uint16_t preSampleClkInterval;
   uint16_t max;
   uint16_t cnt;
} MPU9250MagTypeDef;

typedef struct {
   uint8_t lsm303AccelBuf[6];
   uint8_t lsm303MagBuf[6];
   uint8_t mpu9250AccelBuf[6];
   uint8_t mpu9250MagBuf[6];
   uint8_t mpu9250GyroBuf[6];
//   uint8_t tempBuf[6]; // temp and pres don't need buf
//   uint8_t presBuf[6];
} i2cReadBufTypeDef;

/* USER CODE END Private defines */

void MX_I2C2_Init(void);

/* USER CODE BEGIN Prototypes */

void I2C_init(void);
uint8_t I2C_test(void);
void I2C_scan(I2C_HandleTypeDef *hi2c);
I2C_HandleTypeDef * I2C_getHandlerSensor(void);
#if defined(SHIMMER4_SDK)
I2C_HandleTypeDef * I2C_getHandlerBatt(void);

#define I2C_BATT_REPORT_INTERVAL_DEFAULT    0
void I2C_readBatt(void);
void I2C_readBattSetFreq(uint16_t val);
#endif

void I2cSensing(I2C_SENSING_TYPE start);
#if defined(SHIMMER4_SDK)
void I2cBattMonitor(I2C_SENSING_TYPE start);
#endif
//void I2cSensing(uint8_t start);
//void I2cBattMonitor(uint8_t start);

void I2C_gatherDataInit(void (*done_cb)(void));
void I2cSens_gatherDataCb(void (*done_cb)(void));
#if defined(SHIMMER4_SDK)
void I2cBatt_gatherDataCb(void (*done_cb)(void));
#endif
void I2C_gatherDataStart(void);
void I2cSens_gatherDataStart(void);
#if defined(SHIMMER4_SDK)
void I2cBatt_gatherDataStart(void);
#endif

void I2C_configureChannels(void);
void I2cSens_configureChannels(void);
#if defined(SHIMMER4_SDK)
void I2cBatt_configureChannels(void);
#endif

void I2C_startSensing(void);
void I2Sens_startSensing(void);
#if defined(SHIMMER4_SDK)
void I2Batt_startSensing(void);
#endif

void I2C_pollSensors(void);
void I2cSens_pollSensors(void);
#if defined(SHIMMER4_SDK)
void I2cBatt_pollSensors(void);
#endif

void I2C_stopSensing(void);
void I2cSens_stopSensing(void);
#if defined(SHIMMER4_SDK)
void I2cBatt_stopSensing(void);
#endif


void BMP180Setup(void);
void BMP180Sample(void);
void BMP180TxDoneHandler(void);
void BMP180RxDoneHandler(void);
void BMP280Setup(void);
void BMP280Sample(void);
void BMP280TxDoneHandler(void);
void BMP280RxDoneHandler(void);
void MPU9250Init(void);
void MPU9250GyroSample(void);
void MPU9250GyroTxDoneHandler(void);
void MPU9250GyroRxDoneHandler(void);
void MPU9250AccelSample(void);
void MPU9250AccelTxDoneHandler(void);
void MPU9250AccelRxDoneHandler(void);
void MPU9250MagSetup(void);
void MPU9250MagSample(void);
void MPU9250MagTxDoneHandler(void);
void MPU9250MagRxDoneHandler(void);
void Lsm303dlhcSetup(void);
void Lsm303dlhcAccelSample(void);
void Lsm303dlhcAccelTxDoneHandler(void);
void Lsm303dlhcAccelRxDoneHandler(void);
void Lsm303dlhcMagSample(void);
void Lsm303dlhcMagTxDoneHandler(void);
void Lsm303dlhcMagRxDoneHandler(void);
#if defined(SHIMMER4_SDK)
void STC3100Sample(void);
void STC3100BatteryTxDoneHandler(void);
void STC3100BatteryRxDoneHandler(void);
#endif

void I2cSens_sensorNext(void);
#if defined(SHIMMER4_SDK)
void I2cBatt_sensorNext(void);
#endif

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */

