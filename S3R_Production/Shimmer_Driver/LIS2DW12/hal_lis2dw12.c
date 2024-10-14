/*
 ******************************************************************************
 * @file    test_self_test.c
 * @author  Sensors Software Solution Team
 * @brief   This file run selt test procedure
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/*
 * This example was developed using the following STMicroelectronics
 * evaluation boards:
 *
 * - STEVAL_MKI109V3 + STEVAL-MKI179V1
 * - NUCLEO_F411RE + X_NUCLEO_IKS01A3
 * - DISCOVERY_SPC584B + X_NUCLEO_IKS01A3
 *
 * and STM32CubeMX tool with STM32CubeF4 MCU Package
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F411RE - Host side: UART(COM) to USB bridge
 *                    - I2C(Default) / SPI(supported)
 *
 * DISCOVERY_SPC584B  - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
 *
 * If you need to run this example on a different hardware platform a
 * modification of the functions: `platform_write`, `platform_read`,
 * `tx_com` and 'platform_init' is required.
 *
 */

/* STMicroelectronics evaluation boards definition
 *
 * Please uncomment ONLY the evaluation boards in use.
 * If a different hardware is used please comment all
 * following target board and redefine yours.
 */

//#define STEVAL_MKI109V3  /* little endian */
//#define NUCLEO_F411RE    /* little endian */
//#define SPC584B_DIS      /* big endian */

/* ATTENTION: By default the driver is little endian. If you need switch
 *            to big endian please see "Endianness definitions" in the
 *            header file of the driver (_reg.h).
 */

#if defined(STEVAL_MKI109V3)
/* MKI109V3: Define communication interface */
#define SENSOR_BUS hspi2
/* MKI109V3: Vdd and Vddio power supply values */
#define PWM_3V3    915

#elif defined(NUCLEO_F411RE)
/* NUCLEO_F411RE: Define communication interface */
#define SENSOR_BUS hi2c1

#elif defined(SPC584B_DIS)
/* DISCOVERY_SPC584B: Define communication interface */
#define SENSOR_BUS I2CD1

#elif defined(SHIMMER3R)
#define SENSOR_BUS hspi2

#define CS_PORT    CS_LIS2DW12_GPIO_Port
#define CS_PIN     CS_LIS2DW12_Pin

#endif

/* Includes ------------------------------------------------------------------*/
#include "lis2dw12.h"
#include "lis2dw12_reg.h"
#include <stdio.h>
#include <string.h>

#if defined(NUCLEO_F411RE)
#include "gpio.h"
#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "usart.h"

#elif defined(STEVAL_MKI109V3)
#include "gpio.h"
#include "spi.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "usbd_cdc_if.h"

#elif defined(SPC584B_DIS)
#include "components.h"

#elif defined(SHIMMER3R)
#include "gpio.h"
#include "spi.h"
#include "stm32u5xx_hal.h"

#endif

typedef union
{
  int16_t i16bit[3];
} axis3bit16_t;

/* Private macro -------------------------------------------------------------*/
#define BOOT_TIME         20 //ms

/* Self-test recommended samples */
#define SELF_TEST_SAMPLES 5

/* Self-test positive difference */
#define ST_MIN_POS        70.0f
#define ST_MAX_POS        1500.0f

/* Self test results. */
#define ST_PASS           1U
#define ST_FAIL           0U

/* Private variables ---------------------------------------------------------*/
static axis3bit16_t data_raw_acceleration[SELF_TEST_SAMPLES];
static float acceleration_mg[SELF_TEST_SAMPLES][3];
static uint8_t whoamI, rst;
static uint8_t tx_buffer[1000];

static LIS2DW12_Object_t lis2dw12_obj;

static bool isDrdyIntEnabled = false;

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void tx_com(uint8_t *tx_buffer, uint16_t len);
static void platform_delay(uint32_t ms);
#if defined(SHIMMER3R)
static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len);
#else
static void platform_init(void);
#endif

/* Utility functions ---------------------------------------------------------*/
static inline float ABSF(float _x)
{
  return (_x < 0.0f) ? -(_x) : _x;
}

static int flush_samples(stmdev_ctx_t *dev_ctx)
{
  lis2dw12_reg_t reg;
  axis3bit16_t dummy;
  int samples = 0;
  /*
   * Discard old samples
   */
  lis2dw12_status_reg_get(dev_ctx, &reg.status);

  if (reg.status.drdy)
  {
    lis2dw12_acceleration_raw_get(dev_ctx, dummy.i16bit);
    samples++;
  }

  return samples;
}

static uint8_t test_self_test_lis2dw12(stmdev_ctx_t *dev_ctx)
{
  lis2dw12_reg_t reg;
  float media[3] = { 0.0f, 0.0f, 0.0f };
  float mediast[3] = { 0.0f, 0.0f, 0.0f };
  uint8_t match[3] = { 0, 0, 0 };
  uint8_t j = 0;
  uint16_t i = 0;
  uint8_t k = 0;
  uint8_t axis;
  uint8_t st_result;

  st_result = ST_PASS;

  /* Restore default configuration */
  lis2dw12_reset_set(dev_ctx, PROPERTY_ENABLE);

  do
  {
    lis2dw12_reset_get(dev_ctx, &rst);
  } while (rst);

  lis2dw12_block_data_update_set(dev_ctx, PROPERTY_ENABLE);
  lis2dw12_full_scale_set(dev_ctx, LIS2DW12_4g);
  lis2dw12_power_mode_set(dev_ctx, LIS2DW12_HIGH_PERFORMANCE);
  lis2dw12_data_rate_set(dev_ctx, LIS2DW12_XL_ODR_50Hz);
  HAL_Delay(100);
  /* Flush old samples */
  flush_samples(dev_ctx);

  do
  {
    lis2dw12_status_reg_get(dev_ctx, &reg.status);

    if (reg.status.drdy)
    {
      /* Read accelerometer data */
      memset(data_raw_acceleration[i].i16bit, 0x00, 3 * sizeof(int16_t));
      lis2dw12_acceleration_raw_get(dev_ctx, data_raw_acceleration[i].i16bit);

      for (axis = 0; axis < 3; axis++)
      {
        acceleration_mg[i][axis]
            = lis2dw12_from_fs4_to_mg(data_raw_acceleration[i].i16bit[axis]);
      }

      i++;
    }
  } while (i < SELF_TEST_SAMPLES);

  for (k = 0; k < 3; k++)
  {
    for (j = 0; j < SELF_TEST_SAMPLES; j++)
    {
      media[k] += acceleration_mg[j][k];
    }

    media[k] = (media[k] / j);
  }

  /* Enable self test mode */
  lis2dw12_self_test_set(dev_ctx, LIS2DW12_XL_ST_POSITIVE);
  HAL_Delay(100);
  i = 0;
  /* Flush old samples */
  flush_samples(dev_ctx);

  do
  {
    lis2dw12_status_reg_get(dev_ctx, &reg.status);

    if (reg.status.drdy)
    {
      /* Read accelerometer data */
      memset(data_raw_acceleration[i].i16bit, 0x00, 3 * sizeof(int16_t));
      lis2dw12_acceleration_raw_get(dev_ctx, data_raw_acceleration[i].i16bit);

      for (axis = 0; axis < 3; axis++)
        acceleration_mg[i][axis]
            = lis2dw12_from_fs4_to_mg(data_raw_acceleration[i].i16bit[axis]);

      i++;
    }
  } while (i < SELF_TEST_SAMPLES);

  for (k = 0; k < 3; k++)
  {
    for (j = 0; j < SELF_TEST_SAMPLES; j++)
    {
      mediast[k] += acceleration_mg[j][k];
    }

    mediast[k] = (mediast[k] / j);
  }

  /* Check for all axis self test value range */
  for (k = 0; k < 3; k++)
  {
    if ((ABSF(mediast[k] - media[k]) >= ST_MIN_POS)
        && (ABSF(mediast[k] - media[k]) <= ST_MAX_POS))
    {
      match[k] = 1;
    }

    //sprintf((char *)tx_buffer, "%d: |%f| <= |%f| <= |%f| %s\r\n", k,
    //        ST_MIN_POS, ABSF(mediast[k] - media[k]), ST_MAX_POS,
    //        match[k] == 1 ? "PASSED" : "FAILED");
    //tx_com(tx_buffer, strlen((char const *)tx_buffer));

    if (match[k] == 0)
    {
      st_result = ST_FAIL;
    }
  }

  /* Disable self test mode */
  lis2dw12_data_rate_set(dev_ctx, LIS2DW12_XL_ODR_OFF);
  lis2dw12_self_test_set(dev_ctx, LIS2DW12_XL_ST_DISABLE);

  return st_result;
}

/* Main Example --------------------------------------------------------------*/

void lis2dw12_driver_init(void)
{
  /* Initialize mems driver interface */
  lis2dw12_obj.Ctx.write_reg = platform_write;
  lis2dw12_obj.Ctx.read_reg = platform_read;
  lis2dw12_obj.Ctx.mdelay = platform_delay;
  lis2dw12_obj.Ctx.handle = &SENSOR_BUS;

  //LIS2DW12_RegisterBusIO(lis2dw12_obj, pIO);
}

void lis2dw12_selectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void lis2dw12_unselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

uint8_t lis2dw12_self_test(void)
{
  uint8_t st_result;

  lis2dw12_driver_init();
#if !defined(SHIMMER3R)
  /* Initialize platform specific hardware */
  platform_init();
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);
#endif
  /* Check device ID */
  lis2dw12_device_id_get(&lis2dw12_obj.Ctx, &whoamI);

  if (whoamI != LIS2DW12_ID)
  {
    st_result = ST_FAIL;
  }

  if (st_result == ST_PASS)
  {
    /* Start self test */
    //while (1) {
    st_result = test_self_test_lis2dw12(&lis2dw12_obj.Ctx);
    //}
  }

  //if (st_result == ST_PASS)
  //{
  //  sprintf((char *) tx_buffer, "LIS2DW12 Self Test - PASS\r\n");
  //}
  //else
  //{
  //  sprintf((char *) tx_buffer, "LIS2DW12 Self Test - FAIL\r\n");
  //}
  //
  //tx_com(tx_buffer, strlen((char const *) tx_buffer));
  return st_result;
}

int32_t lis2dw12_configure(float shimmerSamplingFreq,
    lis2dw12_odr_t rate,
    lis2dw12_fs_t range,
    lis2dw12_mode_t mode)
{
  LIS2DW12_Init(&lis2dw12_obj);

  //if (LIS2DW12_ACC_Enable(&lis2dw12_obj) != LIS2DW12_OK)
  //{
  //  return LIS2DW12_ERROR;
  //}

  /* Output data rate selection. */
  if (lis2dw12_data_rate_set(&(lis2dw12_obj.Ctx), rate) != LIS2DW12_OK)
  {
    return LIS2DW12_ERROR;
  }

  /* Full scale selection. */
  if (lis2dw12_full_scale_set(&(lis2dw12_obj.Ctx), range) != LIS2DW12_OK)
  {
    return LIS2DW12_ERROR;
  }

  /* Power mode selection. */
  if (lis2dw12_power_mode_set(&(lis2dw12_obj.Ctx), mode) != LIS2DW12_OK)
  {
    return LIS2DW12_ERROR;
  }

  isDrdyIntEnabled = false;
  //TODO remove IF when fully switched from eval board to BGA variant
#if defined(S3R_BGA_VARIANT)
  if (lis2dw12_is_shimmer_freq_higher(shimmerSamplingFreq, rate))
  {
    lis2dw12_int_notification_set(&(lis2dw12_obj.Ctx), LIS2DW12_INT_LATCHED);

    lis2dw12_ctrl4_int1_pad_ctrl_t int1_pad_ctrl;
    int1_pad_ctrl.int1_drdy = PROPERTY_ENABLE;
    lis2dw12_pin_int1_route_set(&(lis2dw12_obj.Ctx), &int1_pad_ctrl);
    isDrdyIntEnabled = true;
  }
#endif

  lis2dw12_obj.acc_is_enabled = 1U;

  return LIS2DW12_OK;
}

HAL_StatusTypeDef lis2dw12_accel_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[] = { LIS2DW12_OUT_X_L | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(lis2dw12_obj.Ctx.handle, &txBuff[0], buf, sizeof(txBuff));
  return ret;
}

bool lis2dw12_is_drdy_int_enabled(void)
{
  return isDrdyIntEnabled;
}

bool lis2dw12_is_shimmer_freq_higher(float shimmerSamplingFreq, lis2dw12_odr_t rate)
{
  return shimmerSamplingFreq > lis2dw12_get_sensor_freq_from_rate(rate);
}

float lis2dw12_get_sensor_freq_from_rate(lis2dw12_odr_t rate)
{
  float sensorFreq = 0.0;
  switch (rate)
  {
  case LIS2DW12_XL_ODR_1Hz6_LP_ONLY:
    sensorFreq = 1.6;
    break;
  case LIS2DW12_XL_ODR_12Hz5:
    sensorFreq = 12.5;
    break;
  case LIS2DW12_XL_ODR_25Hz:
    sensorFreq = 25.0;
    break;
  case LIS2DW12_XL_ODR_50Hz:
    sensorFreq = 50.0;
    break;
  case LIS2DW12_XL_ODR_100Hz:
    sensorFreq = 100.0;
    break;
  case LIS2DW12_XL_ODR_200Hz:
    sensorFreq = 200.0;
    break;
  case LIS2DW12_XL_ODR_400Hz:
    sensorFreq = 400.0;
    break;
  case LIS2DW12_XL_ODR_800Hz:
    sensorFreq = 800.0;
    break;
  case LIS2DW12_XL_ODR_1k6Hz:
    sensorFreq = 1600.0;
    break;
  default:
    sensorFreq = 0.0;
    break;
  }
  return sensorFreq;
}

int32_t lis2dw12_standby(void)
{
  return LIS2DW12_ACC_Disable(&lis2dw12_obj);
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_I2C_Mem_Write(handle, LIS2DW12_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT,
      (uint8_t *) bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_write(handle, LIS2DW12_I2C_ADD_H & 0xFE, reg, (uint8_t *) bufp, len);
#elif defined(SHIMMER3R)
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
#endif
  return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_I2C_Mem_Read(handle, LIS2DW12_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_read(handle, LIS2DW12_I2C_ADD_H & 0xFE, reg, bufp, len);
#elif defined(SHIMMER3R)
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
#endif
  return 0;
}

static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len)
{
  HAL_StatusTypeDef ret;
  lis2dw12_selectDevice();
  ret = HAL_SPI_TransmitReceive_DMA(handle, txBufp, rxBufp, len);
  return ret;
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  tx_buffer     buffer to transmit
 * @param  len           number of byte to send
 *
 */
static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
#if defined(NUCLEO_F411RE)
  HAL_UART_Transmit(&huart2, tx_buffer, len, 1000);
#elif defined(STEVAL_MKI109V3)
  CDC_Transmit_FS(tx_buffer, len);
#elif defined(SPC584B_DIS)
  sd_lld_write(&SD2, tx_buffer, len);
#elif defined(SHIMMER3R)
  SHIMMER_PRINTF((char *) tx_buffer, len);
#endif
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
#if defined(NUCLEO_F411RE) | defined(STEVAL_MKI109V3) | defined(SHIMMER3R)
  HAL_Delay(ms);
#elif defined(SPC584B_DIS)
  osalThreadDelayMilliseconds(ms);
#endif
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
#if defined(STEVAL_MKI109V3)
  TIM3->CCR1 = PWM_3V3;
  TIM3->CCR2 = PWM_3V3;
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_Delay(1000);
#endif
}
