/*
 ******************************************************************************
 * @file    self_test.c
 * @author  Sensors Software Solution Team
 * @brief   This file implements self test process described by AN5069.
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
 * - STEVAL_MKI109V3 + STEVAL-MKI137V1
 * - NUCLEO_F411RE + X-NUCLEO-IKS01A1
 * - DISCOVERY_SPC584B + STEVAL-MKI137V1
 *
 * and STM32CubeMX tool with STM32CubeF4 MCU Package
 *
 * Used interfaces:
 *
 * STEVAL_MKI109V3    - Host side:   USB (Virtual COM)
 *                    - Sensor side: SPI(Default) / I2C(supported)
 *
 * NUCLEO_STM32F411RE - Host side: UART(COM) to USB bridge
 *                    - Sensor side: I2C(Default) / SPI(supported)
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

//Based on https://github.com/STMicroelectronics/STMems_Standard_C_drivers/tree/master/lis3mdl_STdC/examples

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
/* SHIMMER3R: Define communication interface */
#define SENSOR_BUS hspi2

#define CS_PORT    CS_LIS3MDL_GPIO_Port
#define CS_PIN     CS_LIS3MDL_Pin

#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#if defined(NUCLEO_F411RE)
#include "lis3mdl_reg.h"

#include "gpio.h"
#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "usart.h"

#elif defined(STEVAL_MKI109V3)
#include "lis3mdl_reg.h"

#include "gpio.h"
#include "spi.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "usbd_cdc_if.h"

#elif defined(SPC584B_DIS)
#include "lis3mdl_reg.h"

#include "components.h"

#elif defined(SHIMMER3R)
#include "lis3mdl.h"
#include "lis3mdl_reg.h"

#include "gpio.h"
#include "spi.h"
#include "stm32u5xx_hal.h"
#include "tim.h"
#endif

/* Private macro -------------------------------------------------------------*/
#define WAIT_TIME_00 20 //ms
#define WAIT_TIME_01 20 //ms
#define WAIT_TIME_02 60 //ms

#define SAMPLES      5 //number of samples

/* Self test results. */
#define ST_PASS      1U
#define ST_FAIL      0U

/* Self test limits in gauss*/
static const float min_st_limit[] = { 1.0f, 1.0f, 0.1f };
static const float max_st_limit[] = { 3.0f, 3.0f, 1.0f };

/* Private variables ---------------------------------------------------------*/
static LIS3MDL_Object_t lis3mdl_obj;

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

/* Main Example --------------------------------------------------------------*/

#if defined(SHIMMER3R)
void lis3mdl_driver_init(void)
{
  /* Initialize mems driver interface */
  lis3mdl_obj.Ctx.write_reg = platform_write;
  lis3mdl_obj.Ctx.read_reg = platform_read;
  lis3mdl_obj.Ctx.mdelay = platform_delay;
  lis3mdl_obj.Ctx.handle = &SENSOR_BUS;
}

void lis3mdl_selectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void lis3mdl_unselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

self_test_result_t lis3mdl_self_test(void)
{
  uint8_t tx_buffer[1000];
  int16_t data_raw[3];
  float val_st_off[3];
  float val_st_on[3];
  float test_val[3];
  uint8_t whoamI;
  uint8_t drdy;
  uint8_t i;
  uint8_t j;
  self_test_result_t self_test_result = SELF_TEST_PASS;

  lis3mdl_driver_init();

  /* Check device ID */
  lis3mdl_device_id_get(&lis3mdl_obj.Ctx, &whoamI);

  if (whoamI != LIS3MDL_ID)
  {
    self_test_result = SELF_TEST_FAIL_CHIP_DETECTION;
  }
  else
  {
    lis3mdl_restore_default_config();

    /* Set Full Scale */
    lis3mdl_full_scale_set(&lis3mdl_obj.Ctx, LIS3MDL_12_GAUSS);
    /* Set Output Data Rate */
    lis3mdl_data_rate_set(&lis3mdl_obj.Ctx, LIS3MDL_LP_80Hz);
    /* Wait */
    platform_delay(WAIT_TIME_00);
    /* Set Operating mode */
    lis3mdl_operating_mode_set(&lis3mdl_obj.Ctx, LIS3MDL_CONTINUOUS_MODE);
    /* Wait stable output */
    platform_delay(WAIT_TIME_01);

    /* Adding DRDY pin test */
    if (!lis3mdl_drdy_test())
    {
      self_test_result = SELF_TEST_FAIL_DRDY_ISSUE;
    }
    else
    {
      /* Check if new value available */
      do
      {
        lis3mdl_mag_data_ready_get(&lis3mdl_obj.Ctx, &drdy);
      } while (!drdy);

      /* Read dummy data and discard it */
      lis3mdl_magnetic_raw_get(&lis3mdl_obj.Ctx, data_raw);

      /* Read samples and get the average vale for each axis */
      memset(val_st_off, 0x00, 3 * sizeof(float));
      for (i = 0; i < SAMPLES; i++)
      {
        /* Check if new value available */
        do
        {
          lis3mdl_mag_data_ready_get(&lis3mdl_obj.Ctx, &drdy);
        } while (!drdy);

        /* Read data and accumulate the mg value */
        lis3mdl_magnetic_raw_get(&lis3mdl_obj.Ctx, data_raw);

        for (j = 0; j < 3; j++)
        {
          val_st_off[j] += lis3mdl_from_fs12_to_gauss(data_raw[j]);
        }
      }

      /* Calculate the mg average values */
      for (i = 0; i < 3; i++)
      {
        val_st_off[i] /= SAMPLES;
      }

      /* Enable Self Test */
      lis3mdl_self_test_set(&lis3mdl_obj.Ctx, PROPERTY_ENABLE);
      /* Wait stable output */
      platform_delay(WAIT_TIME_02);

      /* Check if new value available */
      do
      {
        lis3mdl_mag_data_ready_get(&lis3mdl_obj.Ctx, &drdy);
      } while (!drdy);

      /* Read dummy data and discard it */
      lis3mdl_magnetic_raw_get(&lis3mdl_obj.Ctx, data_raw);
      /* Read samples and get the average vale for each axis */
      memset(val_st_on, 0x00, 3 * sizeof(float));

      for (i = 0; i < SAMPLES; i++)
      {
        /* Check if new value available */
        do
        {
          lis3mdl_mag_data_ready_get(&lis3mdl_obj.Ctx, &drdy);
        } while (!drdy);

        /* Read data and accumulate the mg value */
        lis3mdl_magnetic_raw_get(&lis3mdl_obj.Ctx, data_raw);

        for (j = 0; j < 3; j++)
        {
          val_st_on[j] += lis3mdl_from_fs12_to_gauss(data_raw[j]);
        }
      }

      /* Calculate the mg average values */
      for (i = 0; i < 3; i++)
      {
        val_st_on[i] /= SAMPLES;
      }

      /* Calculate the mg values for self test */
      for (i = 0; i < 3; i++)
      {
        test_val[i] = fabs((val_st_on[i] - val_st_off[i]));
      }

      /* Check self test limit */
      for (i = 0; i < 3; i++)
      {
        if ((min_st_limit[i] > test_val[i]) || (test_val[i] > max_st_limit[i]))
        {
          self_test_result = SELF_TEST_FAIL_SIGNAL_ISSUE;
        }
      }
    }
    float_t tempCal;
    lis3mdl_temperature_get(&tempCal);

    /* Disable Self Test */
    lis3mdl_self_test_set(&lis3mdl_obj.Ctx, PROPERTY_DISABLE);
    /* Disable sensor. */
    lis3mdl_operating_mode_set(&lis3mdl_obj.Ctx, LIS3MDL_POWER_DOWN);
  }
  return self_test_result;
}

uint8_t lis3mdl_drdy_test(void)
{
  int16_t data_raw[3];
  uint8_t i;
  for(i = 0; i < 5; i++)
  {
	platform_delay(WAIT_TIME_01);
    if(LIS3MDL_DRDY)
    {
      /* Read dummy data and discard it */
      lis3mdl_magnetic_raw_get(&lis3mdl_obj.Ctx, data_raw);
      if(LIS3MDL_DRDY)
      {
        return 0; //Test fails
      }
      else
      {
        return 1; //Test pass
      }
    }
  }
  return 0; //Test fails due to incorrect pin settings after timeout
}

void lis3mdl_configure(float shimmerSamplingFreq, lis3mdl_om_t rate, lis3mdl_fs_t range)
{
  LIS3MDL_Init(&lis3mdl_obj);
  /* Note: DRDY pin is always enabled for LIS3MDL nothing needs to be configured */

  /* Set Full Scale */
  lis3mdl_full_scale_set(&lis3mdl_obj.Ctx, range);
  /* Set Output Data Rate */
  lis3mdl_data_rate_set(&lis3mdl_obj.Ctx, rate);
  /* Wait */
  platform_delay(WAIT_TIME_00);
  /* Set Operating mode */
  lis3mdl_operating_mode_set(&lis3mdl_obj.Ctx, LIS3MDL_CONTINUOUS_MODE);
  /* Wait stable output */
  platform_delay(WAIT_TIME_01);
}

HAL_StatusTypeDef lis3mdl_mag_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[] = { LIS3MDL_OUT_X_L | 0xC0, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(lis3mdl_obj.Ctx.handle, &txBuff[0], buf, sizeof(txBuff));
  return ret;
}

float lis3mdl_get_sensor_freq_from_rate(lis3mdl_om_t rate)
{
  float sensorFreq = 0.0;
  switch (rate)
  {
  case LIS3MDL_LP_1kHz:
    sensorFreq = 1000.0;
    break;
  case LIS3MDL_MP_560Hz:
    sensorFreq = 560.0;
    break;
  case LIS3MDL_HP_300Hz:
    sensorFreq = 300.0;
    break;
  case LIS3MDL_UHP_155Hz:
    sensorFreq = 155.0;
    break;
  case LIS3MDL_LP_80Hz:
  case LIS3MDL_MP_80Hz:
  case LIS3MDL_HP_80Hz:
  case LIS3MDL_UHP_80Hz:
    sensorFreq = 80.0;
    break;
  case LIS3MDL_LP_40Hz:
  case LIS3MDL_MP_40Hz:
  case LIS3MDL_HP_40Hz:
  case LIS3MDL_UHP_40Hz:
    sensorFreq = 40.0;
    break;
  case LIS3MDL_LP_20Hz:
  case LIS3MDL_MP_20Hz:
  case LIS3MDL_HP_20Hz:
  case LIS3MDL_UHP_20Hz:
    sensorFreq = 20.0;
    break;
  case LIS3MDL_LP_10Hz:
  case LIS3MDL_MP_10Hz:
  case LIS3MDL_HP_10Hz:
  case LIS3MDL_UHP_10Hz:
    sensorFreq = 10.0;
    break;
  case LIS3MDL_LP_5Hz:
  case LIS3MDL_MP_5Hz:
  case LIS3MDL_HP_5Hz:
  case LIS3MDL_UHP_5Hz:
    sensorFreq = 5.0;
    break;
  case LIS3MDL_LP_2Hz5:
  case LIS3MDL_MP_2Hz5:
  case LIS3MDL_HP_2Hz5:
  case LIS3MDL_UHP_2Hz5:
    sensorFreq = 2.5;
    break;
  case LIS3MDL_LP_1Hz25:
  case LIS3MDL_MP_1Hz25:
  case LIS3MDL_HP_1Hz25:
  case LIS3MDL_UHP_1Hz25:
    sensorFreq = 1.25;
    break;
  case LIS3MDL_LP_Hz625:
    sensorFreq = 0.625;
    break;
  default:
    sensorFreq = 0.0;
    break;
  }
  return sensorFreq;
}

#endif

void lis3mdl_restore_default_config(void)
{
  uint8_t rst;

  /* Restore default configuration */
  lis3mdl_reset_set(&lis3mdl_obj.Ctx, PROPERTY_ENABLE);

  do
  {
    lis3mdl_reset_get(&lis3mdl_obj.Ctx, &rst);
  } while (rst);

  /* Enable Block Data Update */
  lis3mdl_block_data_update_set(&lis3mdl_obj.Ctx, PROPERTY_ENABLE);
}

void lis3mdl_spi_three_wire_set(void)
{
  int32_t ret;

  lis3mdl_ctrl_reg3_t ctrl_reg3;
  ctrl_reg3.not_used_02 = 0; //Default = 0
  ctrl_reg3.lp = 0;          //Default = 0
  ctrl_reg3.not_used_01 = 0; //Default = 0
  ctrl_reg3.sim = LIS3MDL_SPI_3_WIRE;
  ctrl_reg3.md = 3; //Default = 3

  ret = lis3mdl_write_reg(&lis3mdl_obj.Ctx, LIS3MDL_CTRL_REG3, (uint8_t *) &ctrl_reg3, 1);
}

int32_t lis3mdl_temperature_get(float_t *tempCal)
{
  int16_t tempUncal = 0;

  lis3mdl_temperature_meas_set(&lis3mdl_obj.Ctx, 1);
  /* Wait stable output - just picking an existing delay here */
  platform_delay(WAIT_TIME_02);

  int32_t res = lis3mdl_temperature_raw_get(&lis3mdl_obj.Ctx, &tempUncal);

  lis3mdl_temperature_meas_set(&lis3mdl_obj.Ctx, 0);

  *tempCal = lis3mdl_from_lsb_to_celsius(tempUncal);
  return res;
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
  /* Write multiple command */
  reg |= 0x80;
  HAL_I2C_Mem_Write(handle, LIS3MDL_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT,
      (uint8_t *) bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  /* Write multiple command */
  reg |= 0x40;
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  /* Write multiple command */
  reg |= 0x80;
  i2c_lld_write(handle, LIS3MDL_I2C_ADD_L & 0xFE, reg, (uint8_t *) bufp, len);
#elif defined(SHIMMER3R)
  /* Write multiple command */
  reg |= 0x40;
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);

  //uint8_t txBuf[2] = { 0 };
  //txBuf[0] = reg;
  //txBuf[1] = *bufp;
  //HAL_SPI_Transmit(handle, &txBuf[0], 2, 1000);
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
  /* Read multiple command */
  reg |= 0x80;
  HAL_I2C_Mem_Read(handle, LIS3MDL_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  /* Read multiple command */
  reg |= 0xC0;
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  /* Read multiple command */
  reg |= 0x80;
  i2c_lld_read(handle, LIS3MDL_I2C_ADD_L & 0xFE, reg, bufp, len);
#elif defined(SHIMMER3R)
  ///* Read multiple command */
  reg |= 0xC0;
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
  lis3mdl_selectDevice();
  ret = HAL_SPI_TransmitReceive_DMA(handle, txBufp, rxBufp, len);
  return ret;
}

/*
 * @brief  Send buffer to console (platform dependent)
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

#if !defined(SHIMMER3R)
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
#endif
