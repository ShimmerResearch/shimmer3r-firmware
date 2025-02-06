/*
 ******************************************************************************
 * @file    lsm6dsv_self_test.c
 * @author  Sensors Software Solution Team
 * @brief   This file implements the self test procedure.
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
 * - STEVAL_MKI109V3 + STEVAL-MKI196V1
 * - NUCLEO_F411RE + X_NUCLEO_IKS01A3
 * - DISCOVERY_SPC584B + STEVAL-MKI196V1
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

//https://github.com/STMicroelectronics/STMems_Standard_C_drivers/tree/master/lsm6dsv_STdC/examples

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
#define SENSOR_BUS hspi1

#define CS_PORT    CS_LSM6DSV_GPIO_Port
#define CS_PIN     CS_LSM6DSV_Pin

#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#if defined(NUCLEO_F411RE)
#include "lsm6dsv_reg.h"

#include "gpio.h"
#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "usart.h"

#elif defined(STEVAL_MKI109V3)
#include "lsm6dsv_reg.h"

#include "gpio.h"
#include "spi.h"
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "usbd_cdc_if.h"

#elif defined(SPC584B_DIS)
#include "lsm6dsv_reg.h"

#include "components.h"

#elif defined(SHIMMER3R)
#include "lsm6dsv.h"
#include "lsm6dsv_reg.h"

#include "gpio.h"
#include "spi.h"
#include "stm32u5xx.h"
#include "usart.h"
#endif

/* Private macro -------------------------------------------------------------*/
#define BOOT_TIME                 10

/* Self test limits. */
#define SELF_TEST_MIN_ST_LIMIT_mg 50.0f
#define SELF_TEST_MAX_ST_LIMIT_mg 1700.0f
#define MIN_ST_LIMIT_mdps         150000.0f
#define MAX_ST_LIMIT_mdps         700000.0f

/* Self test results. */
#define ST_PASS                   1U
#define ST_FAIL                   0U

uint8_t tx_buffer[1000];

/* Private variables ---------------------------------------------------------*/
static LSM6DSV_Object_t lsm6dsv_obj;

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

#if defined(SHIMMER3R)
void lsm6dsv_driver_init(void)
{
  /* Initialize mems driver interface */
  lsm6dsv_obj.Ctx.write_reg = platform_write;
  lsm6dsv_obj.Ctx.read_reg = platform_read;
  lsm6dsv_obj.Ctx.mdelay = platform_delay;
  lsm6dsv_obj.Ctx.handle = &SENSOR_BUS;
}

void lsm6dsv_selectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void lsm6dsv_unselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

self_test_result_t lsm6dsv_self_test(void)
{
  lsm6dsv_all_sources_t all_sources;
  int16_t data_raw[3];
  float val_st_off[3];
  float val_st_on[3];
  float test_val[3];
  uint8_t whoamI;
  lsm6dsv_reset_t rst;
  uint8_t i;
  uint8_t j;
  self_test_result_t self_test_result = SELF_TEST_PASS;

  lsm6dsv_driver_init();

  /* Check device ID */
  lsm6dsv_device_id_get(&lsm6dsv_obj.Ctx, &whoamI);

  if (whoamI != LSM6DSV_ID)
  {
    //st_result = ST_FAIL;
    self_test_result = SELF_TEST_FAIL_CHIP_DETECTION;
  }
  else
  {
    /* Restore default configuration */
    lsm6dsv_reset_set(&lsm6dsv_obj.Ctx, LSM6DSV_RESTORE_CTRL_REGS);
    do
    {
      lsm6dsv_reset_get(&lsm6dsv_obj.Ctx, &rst);
    } while (rst != LSM6DSV_READY);

    /* Enable Block Data Update */
    lsm6dsv_block_data_update_set(&lsm6dsv_obj.Ctx, PROPERTY_ENABLE);
    /*
     * Accelerometer Self Test
     */
    /* Set Output Data Rate */
    lsm6dsv_xl_data_rate_set(&lsm6dsv_obj.Ctx,  LSM6DSV_ODR_AT_60Hz);
    /* Set full scale */
    lsm6dsv_xl_full_scale_set(&lsm6dsv_obj.Ctx, LSM6DSV_4g);
    /* Wait stable output */
    platform_delay(100);

    if(!lsm6dsv_drdy_test())
    {
      self_test_result = SELF_TEST_FAIL_DRDY_ISSUE;
    }
    else
    {
    /* Check if new value available */
      do
      {
        lsm6dsv_all_sources_get(&lsm6dsv_obj.Ctx, &all_sources);
      } while (!all_sources.drdy_xl);

      /* Read dummy data and discard it */
      lsm6dsv_acceleration_raw_get(&lsm6dsv_obj.Ctx, data_raw);
      /* Read 5 sample and get the average vale for each axis */
      memset(val_st_off, 0x00, 3 * sizeof(float));

      for (i = 0; i < 5; i++)
      {
        /* Check if new value available */
        do
        {
          lsm6dsv_all_sources_get(&lsm6dsv_obj.Ctx, &all_sources);
        } while (!all_sources.drdy_xl);

        /* Read data and accumulate the mg value */
        lsm6dsv_acceleration_raw_get(&lsm6dsv_obj.Ctx, data_raw);

        for (j = 0; j < 3; j++)
        {
          val_st_off[j] += lsm6dsv_from_fs4_to_mg(data_raw[j]);
        }
      }

      /* Calculate the mg average values */
      for (i = 0; i < 3; i++)
      {
        val_st_off[i] /= 5.0f;
      }

      /* Enable Self Test positive (or negative) */
      lsm6dsv_xl_self_test_set(&lsm6dsv_obj.Ctx, LSM6DSV_XL_ST_NEGATIVE);
      //lsm6dsv_xl_self_test_set(&lsm6dsv_obj.Ctx, LSM6DSV_XL_ST_POSITIVE);
      /* Wait stable output */
      platform_delay(100);

      /* Check if new value available */
      do
      {
        lsm6dsv_all_sources_get(&lsm6dsv_obj.Ctx, &all_sources);
      } while (!all_sources.drdy_xl);

      /* Read dummy data and discard it */
      lsm6dsv_acceleration_raw_get(&lsm6dsv_obj.Ctx, data_raw);
      /* Read 5 sample and get the average vale for each axis */
      memset(val_st_on, 0x00, 3 * sizeof(float));

      for (i = 0; i < 5; i++)
      {
        /* Check if new value available */
        do
        {
          lsm6dsv_all_sources_get(&lsm6dsv_obj.Ctx, &all_sources);
        } while (!all_sources.drdy_xl);

        /* Read data and accumulate the mg value */
        lsm6dsv_acceleration_raw_get(&lsm6dsv_obj.Ctx, data_raw);

        for (j = 0; j < 3; j++)
        {
          val_st_on[j] += lsm6dsv_from_fs4_to_mg(data_raw[j]);
        }
      }

      /* Calculate the mg average values */
      for (i = 0; i < 3; i++)
      {
        val_st_on[i] /= 5.0f;
      }

      /* Calculate the mg values for self test */
      for (i = 0; i < 3; i++)
      {
        test_val[i] = fabs((val_st_on[i] - val_st_off[i]));
      }

      /* Check self test limit */
      for (i = 0; i < 3; i++)
      {
        if ((SELF_TEST_MIN_ST_LIMIT_mg > test_val[i]) || (test_val[i] > SELF_TEST_MAX_ST_LIMIT_mg))
        {
          //st_result = ST_FAIL;
          self_test_result = SELF_TEST_FAIL_SIGNAL_ISSUE;
        }
      }

      /* Disable Self Test */
      lsm6dsv_xl_self_test_set(&lsm6dsv_obj.Ctx, LSM6DSV_XL_ST_DISABLE);
      /* Disable sensor. */
      lsm6dsv_xl_data_rate_set(&lsm6dsv_obj.Ctx, LSM6DSV_ODR_OFF);
      /*
       * Gyroscope Self Test
       */
      /* Set Output Data Rate */
      lsm6dsv_gy_data_rate_set(&lsm6dsv_obj.Ctx, LSM6DSV_ODR_AT_240Hz);
      /* Set full scale */
      lsm6dsv_gy_full_scale_set(&lsm6dsv_obj.Ctx, LSM6DSV_2000dps);
      /* Wait stable output */
      platform_delay(100);

      /* Check if new value available */
      do
      {
        lsm6dsv_all_sources_get(&lsm6dsv_obj.Ctx, &all_sources);
      } while (!all_sources.drdy_gy);

      /* Read dummy data and discard it */
      lsm6dsv_angular_rate_raw_get(&lsm6dsv_obj.Ctx, data_raw);
      /* Read 5 sample and get the average vale for each axis */
      memset(val_st_off, 0x00, 3 * sizeof(float));

      for (i = 0; i < 5; i++)
      {
        /* Check if new value available */
        do
        {
          lsm6dsv_all_sources_get(&lsm6dsv_obj.Ctx, &all_sources);
        } while (!all_sources.drdy_gy);
        /* Read data and accumulate the mg value */
        lsm6dsv_angular_rate_raw_get(&lsm6dsv_obj.Ctx, data_raw);

        for (j = 0; j < 3; j++)
        {
          val_st_off[j] += lsm6dsv_from_fs2000_to_mdps(data_raw[j]);
        }
      }

      /* Calculate the mg average values */
      for (i = 0; i < 3; i++)
      {
        val_st_off[i] /= 5.0f;
      }

      /* Enable Self Test positive (or negative) */
      lsm6dsv_gy_self_test_set(&lsm6dsv_obj.Ctx, LSM6DSV_GY_ST_POSITIVE);
      //lsm6dsv_gy_self_test_set(&lsm6dsv_obj.Ctx, LIS2DH12_GY_ST_NEGATIVE);
      /* Wait stable output */
      platform_delay(100);
      /* Read 5 sample and get the average vale for each axis */
      memset(val_st_on, 0x00, 3 * sizeof(float));

      for (i = 0; i < 5; i++)
      {
        /* Check if new value available */
        do
        {
          lsm6dsv_all_sources_get(&lsm6dsv_obj.Ctx, &all_sources);
        } while (!all_sources.drdy_gy);

        /* Read data and accumulate the mg value */
        lsm6dsv_angular_rate_raw_get(&lsm6dsv_obj.Ctx, data_raw);

        for (j = 0; j < 3; j++)
        {
          val_st_on[j] += lsm6dsv_from_fs2000_to_mdps(data_raw[j]);
        }
      }

      /* Calculate the mg average values */
      for (i = 0; i < 3; i++)
      {
        val_st_on[i] /= 5.0f;
      }

      /* Calculate the mg values for self test */
      for (i = 0; i < 3; i++)
      {
        test_val[i] = fabs((val_st_on[i] - val_st_off[i]));
      }

      /* Check self test limit */
      for (i = 0; i < 3; i++)
      {
        if ((MIN_ST_LIMIT_mdps > test_val[i]) || (test_val[i] > MAX_ST_LIMIT_mdps))
        {
          self_test_result = SELF_TEST_FAIL_SIGNAL_ISSUE;
        }
      }

      /* Disable Self Test */
      lsm6dsv_gy_self_test_set(&lsm6dsv_obj.Ctx, LSM6DSV_GY_ST_DISABLE);
    }
    /* Disable sensor. */
    lsm6dsv_xl_data_rate_set(&lsm6dsv_obj.Ctx, LSM6DSV_ODR_OFF);
  }
  return self_test_result;
}

uint8_t lsm6dsv_drdy_test(void)
{
  int16_t data_raw[3];
  uint8_t i;
  uint8_t res = 0;
  lsm6dsv_interrupt_mode_t mode_int;
  lsm6dsv_pin_int_route_t pin_int;
  pin_int.drdy_xl = PROPERTY_ENABLE;
  mode_int.enable = PROPERTY_ENABLE;
  mode_int.lir = PROPERTY_ENABLE;
  lsm6dsv_interrupt_enable_set(&lsm6dsv_obj.Ctx, mode_int);
  lsm6dsv_pin_int1_route_set(&lsm6dsv_obj.Ctx, &pin_int);
  lsm6dsv_all_sources_t all_sources;
  uint16_t count = 0;
  for(i = 0;i < 15; i++)
  {
    platform_delay(15); // every 16.6 msec new value is seen as per current odr settings
    if(LSM6DSV_DRDY)
    {
      /* Read raw data */
      lsm6dsv_acceleration_raw_get(&lsm6dsv_obj.Ctx, data_raw);
      /* read status again, before checking interrupt pin
       * to make sure pin reads the correct output from register
       * this is to account for delay to see status change in register and pin
       * after data read */
      lsm6dsv_all_sources_get(&lsm6dsv_obj.Ctx, &all_sources);
      res = LSM6DSV_DRDY?0:1;
      if(res == 1) // run test multiple times to get the proper value
      break;
    }
  }
  pin_int.drdy_xl = PROPERTY_DISABLE;
  mode_int.enable = PROPERTY_DISABLE;
  mode_int.lir = PROPERTY_DISABLE;
  lsm6dsv_interrupt_enable_set(&lsm6dsv_obj.Ctx, mode_int);
  lsm6dsv_pin_int1_route_set(&lsm6dsv_obj.Ctx, &pin_int);
  return res;
}

void lsm6dsv_configure(float shimmerSamplingFreq,
    uint8_t isGyroEn,
    uint8_t isAccelEn,
    uint8_t rate,
    uint8_t rangeGyro,
    uint8_t rangeAccel)
{
  LSM6DSV_Init(&lsm6dsv_obj);

  if (isGyroEn)
  {
    lsm6dsv_gy_data_rate_set(&lsm6dsv_obj.Ctx, (lsm6dsv_data_rate_t) rate);
    lsm6dsv_gy_full_scale_set(&lsm6dsv_obj.Ctx, (lsm6dsv_gy_full_scale_t) rangeGyro);
  }

  if (isAccelEn)
  {
    /* Set Output Data Rate.
     * Selected data rate have to be equal or greater with respect
     * with MLC data rate.
     */
    lsm6dsv_xl_data_rate_set(&lsm6dsv_obj.Ctx, (lsm6dsv_data_rate_t) rate);
    /* Set full scale */
    lsm6dsv_xl_full_scale_set(&lsm6dsv_obj.Ctx, (lsm6dsv_xl_full_scale_t) rangeAccel);
  }

  //if chip sampling rate is lower than Shimmer sampling, enable pin
  //interrupt to only read data from chip when it's ready
  lsm6dsv_interrupt_mode_t mode_int;
  lsm6dsv_pin_int_route_t pin_int;
  pin_int.drdy_g_eis = 0;
  pin_int.drdy_temp = 0;
  pin_int.fifo_th = 0;
  pin_int.fifo_ovr = 0;
  pin_int.fifo_full = 0;
  pin_int.cnt_bdr = 0;
  pin_int.emb_func_endop = 0;
  pin_int.timestamp = 0;
  pin_int.shub = 0;
  pin_int.emb_func = 0;
  pin_int.sixd = 0;
  pin_int.single_tap = 0;
  pin_int.double_tap = 0;
  pin_int.wakeup = 0;
  pin_int.freefall = 0;
  pin_int.sleep_change = 0;

  isDrdyIntEnabled = false;
  if (lsm6dsv_is_shimmer_freq_higher(shimmerSamplingFreq, rate))
  {
    if ((isGyroEn && isAccelEn) || isGyroEn)
    {
      pin_int.drdy_g = PROPERTY_ENABLE;
      mode_int.enable = PROPERTY_ENABLE;
      mode_int.lir = PROPERTY_ENABLE;
      isDrdyIntEnabled = true;
    }
    else
    {
      pin_int.drdy_xl = PROPERTY_ENABLE;
      mode_int.enable = PROPERTY_ENABLE;
      mode_int.lir = PROPERTY_ENABLE;
      isDrdyIntEnabled = true;
    }
    lsm6dsv_interrupt_enable_set(&lsm6dsv_obj.Ctx, mode_int);
    lsm6dsv_pin_int1_route_set(&lsm6dsv_obj.Ctx, &pin_int);
  }

  ///* Configure filtering chain */
  //filt_settling_mask.drdy = PROPERTY_ENABLE;
  //filt_settling_mask.irq_xl = PROPERTY_ENABLE;
  //filt_settling_mask.irq_g = PROPERTY_ENABLE;
  //lsm6dsv_filt_settling_mask_set(&lsm6dsv_obj.Ctx, filt_settling_mask);
  //lsm6dsv_filt_xl_lp2_set(&lsm6dsv_obj.Ctx, PROPERTY_ENABLE);
  //lsm6dsv_filt_xl_lp2_bandwidth_set(&lsm6dsv_obj.Ctx, LSM6DSV_XL_STRONG);
}

HAL_StatusTypeDef lsm6dsv_accel_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[] = { LSM6DSV_OUTX_L_A | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(lsm6dsv_obj.Ctx.handle, &txBuff[0], buf, sizeof(txBuff));
  return ret;
}

HAL_StatusTypeDef lsm6dsv_gyro_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[] = { LSM6DSV_OUTX_L_G | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(lsm6dsv_obj.Ctx.handle, &txBuff[0], buf, sizeof(txBuff));
  return ret;
}

HAL_StatusTypeDef lsm6dsv_gyro_accel_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[]
      = { LSM6DSV_OUTX_L_G | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(lsm6dsv_obj.Ctx.handle, &txBuff[0], buf, sizeof(txBuff));
  return ret;
}

bool lsm6dsv_is_drdy_int_enabled(void)
{
  return isDrdyIntEnabled;
}

bool lsm6dsv_is_shimmer_freq_higher(float shimmerSamplingFreq, lsm6dsv_data_rate_t rate)
{
  return shimmerSamplingFreq > lsm6dsv_get_sensor_freq_from_rate(rate);
}

float lsm6dsv_get_sensor_freq_from_rate(lsm6dsv_data_rate_t rate)
{
  float sensorFreq = 0.0;
  switch (rate)
  {
  case LSM6DSV_ODR_AT_1Hz875:
    sensorFreq = 1.875;
    break;
  case LSM6DSV_ODR_AT_7Hz5:
    sensorFreq = 7.5;
    break;
  case LSM6DSV_ODR_AT_15Hz:
    sensorFreq = 15.0;
    break;
  case LSM6DSV_ODR_AT_30Hz:
    sensorFreq = 30.0;
    break;
  case LSM6DSV_ODR_AT_60Hz:
    sensorFreq = 60.0;
    break;
  case LSM6DSV_ODR_AT_120Hz:
    sensorFreq = 120.0;
    break;
  case LSM6DSV_ODR_AT_240Hz:
    sensorFreq = 240.0;
    break;
  case LSM6DSV_ODR_AT_480Hz:
    sensorFreq = 480.0;
    break;
  case LSM6DSV_ODR_AT_960Hz:
    sensorFreq = 960.0;
    break;
  case LSM6DSV_ODR_AT_1920Hz:
    sensorFreq = 1920.0;
    break;
  case LSM6DSV_ODR_AT_3840Hz:
    sensorFreq = 3840.0;
    break;
  case LSM6DSV_ODR_AT_7680Hz:
    sensorFreq = 7680.0;
    break;
  case LSM6DSV_ODR_HA01_AT_15Hz625:
    sensorFreq = 15.625;
    break;
  case LSM6DSV_ODR_HA01_AT_31Hz25:
    sensorFreq = 31.25;
    break;
  case LSM6DSV_ODR_HA01_AT_62Hz5:
    sensorFreq = 62.5;
    break;
  case LSM6DSV_ODR_HA01_AT_125Hz:
    sensorFreq = 125.0;
    break;
  case LSM6DSV_ODR_HA01_AT_250Hz:
    sensorFreq = 250.0;
    break;
  case LSM6DSV_ODR_HA01_AT_500Hz:
    sensorFreq = 500.0;
    break;
  case LSM6DSV_ODR_HA01_AT_1000Hz:
    sensorFreq = 1000.0;
    break;
  case LSM6DSV_ODR_HA01_AT_2000Hz:
    sensorFreq = 2000.0;
    break;
  case LSM6DSV_ODR_HA01_AT_4000Hz:
    sensorFreq = 4000.0;
    break;
  case LSM6DSV_ODR_HA01_AT_8000Hz:
    sensorFreq = 8000.0;
    break;
  case LSM6DSV_ODR_HA02_AT_12Hz5:
    sensorFreq = 12.5;
    break;
  case LSM6DSV_ODR_HA02_AT_25Hz:
    sensorFreq = 25.0;
    break;
  case LSM6DSV_ODR_HA02_AT_50Hz:
    sensorFreq = 50.0;
    break;
  case LSM6DSV_ODR_HA02_AT_100Hz:
    sensorFreq = 100.0;
    break;
  case LSM6DSV_ODR_HA02_AT_200Hz:
    sensorFreq = 200.0;
    break;
  case LSM6DSV_ODR_HA02_AT_400Hz:
    sensorFreq = 400.0;
    break;
  case LSM6DSV_ODR_HA02_AT_800Hz:
    sensorFreq = 800.0;
    break;
  case LSM6DSV_ODR_HA02_AT_1600Hz:
    sensorFreq = 1600.0;
    break;
  case LSM6DSV_ODR_HA02_AT_3200Hz:
    sensorFreq = 3200.0;
    break;
  case LSM6DSV_ODR_HA02_AT_6400Hz:
    sensorFreq = 6400.0;
    break;
  default:
    sensorFreq = 0.0;
    break;
  }
  return sensorFreq;
}

void lsm6dsv_restore_default_config(void)
{
  lsm6dsv_reset_t rst;

  /* Restore default configuration */
  lsm6dsv_reset_set(&lsm6dsv_obj.Ctx, LSM6DSV_RESTORE_CTRL_REGS);
  do
  {
    lsm6dsv_reset_get(&lsm6dsv_obj.Ctx, &rst);
  } while (rst != LSM6DSV_READY);

  /* Enable Block Data Update */
  lsm6dsv_block_data_update_set(&lsm6dsv_obj.Ctx, PROPERTY_ENABLE);
}

void lsm6dsv_status_get(void)
{
  lsm6dsv_data_ready_t drdy;
  lsm6dsv_flag_data_ready_get(&lsm6dsv_obj.Ctx, &drdy);
}

int32_t lsm6dsv_temperature_get(float_t *tempCal)
{
  int16_t tempUncal = 0;
  int32_t res = lsm6dsv_temperature_raw_get(&lsm6dsv_obj.Ctx, &tempUncal);
  *tempCal = lsm6dsv_from_lsb_to_celsius(tempUncal);
  return res;
}

#endif

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
  HAL_I2C_Mem_Write(handle, LSM6DSV_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT,
      (uint8_t *) bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_write(handle, LSM6DSV_I2C_ADD_H & 0xFE, reg, (uint8_t *) bufp, len);
#elif defined(SHIMMER3R)
  lsm6dsv_selectDevice();
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);
  lsm6dsv_unselectDevice();
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
  HAL_I2C_Mem_Read(handle, LSM6DSV_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3)
  reg |= 0x80;
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  HAL_GPIO_WritePin(CS_up_GPIO_Port, CS_up_Pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_read(handle, LSM6DSV_I2C_ADD_H & 0xFE, reg, bufp, len);
#elif defined(SHIMMER3R)
  reg |= SPI_READ_REGISTER;
  lsm6dsv_selectDevice();
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  lsm6dsv_unselectDevice();
#endif
  return 0;
}

static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len)
{
  HAL_StatusTypeDef ret;
  lsm6dsv_selectDevice();
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
