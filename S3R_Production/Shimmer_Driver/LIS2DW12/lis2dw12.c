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
#include "lis2dw12-pid/lis2dw12_reg.h"
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

static stmdev_ctx_t dev_ctx;

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
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.mdelay = platform_delay;
  dev_ctx.handle = &SENSOR_BUS;
}

void lis2dw12_power_on(void)
{
  set_power_spi2_bus(true, SPI2_CHIP_INDEX_LIS2DW12);
}

void lis2dw12_power_off(void)
{
  set_power_spi2_bus(false, SPI2_CHIP_INDEX_LIS2DW12);
}

void lis2dw12_SelectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void lis2dw12_UnselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

void lis2dw12_self_test(void)
{
  uint8_t st_result;

#if !defined(SHIMMER3R)
  lis2dw12_driver_init();
  /* Initialize platform specific hardware */
  platform_init();
#endif
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);
  /* Check device ID */
  lis2dw12_device_id_get(&dev_ctx, &whoamI);

  if (whoamI != LIS2DW12_ID)
  {
    st_result = ST_FAIL;
  }

  if (st_result == ST_PASS)
  {
    /* Start self test */
    //while (1) {
    st_result = test_self_test_lis2dw12(&dev_ctx);
    //}
  }

  if (st_result == ST_PASS)
  {
    sprintf((char *) tx_buffer, "LIS2DW12 Self Test - PASS\r\n");
  }
  else
  {
    sprintf((char *) tx_buffer, "LIS2DW12 Self Test - FAIL\r\n");
  }

  tx_com(tx_buffer, strlen((char const *) tx_buffer));
}

void lis2dw12_config_accel(uint8_t rate, uint8_t range)
{
  //TODO fill in config
  lis2dw12_data_rate_set(&dev_ctx, LIS2DW12_XL_ODR_100Hz);
}

HAL_StatusTypeDef lis2dw12_accel_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[] = { LIS2DW12_OUT_X_L | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(dev_ctx.handle, &txBuff[0], buf, sizeof(txBuff));
  return ret;
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
  lis2dw12_SelectDevice();
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
