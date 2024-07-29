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

void lsm6dsv_base_settings_init(void)
{
  LSM6DSV_Init(&lsm6dsv_obj);
}

void lsm6dsv_power_on(void)
{
  set_power_spi1_bus(true, SPI1_CHIP_INDEX_LSM6DSV);
}

void lsm6dsv_power_off(void)
{
  set_power_spi1_bus(false, SPI1_CHIP_INDEX_LSM6DSV);
}

void lsm6dsv_selectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void lsm6dsv_unselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

uint8_t lsm6dsv_self_test(void)
{
  lsm6dsv_all_sources_t all_sources;
  int16_t data_raw[3];
  float val_st_off[3];
  float val_st_on[3];
  float test_val[3];
  uint8_t st_result;
  uint8_t whoamI;
  lsm6dsv_reset_t rst;
  uint8_t i;
  uint8_t j;

  lsm6dsv_driver_init();

  /* Check device ID */
  lsm6dsv_device_id_get(&lsm6dsv_obj.Ctx, &whoamI);

  if (whoamI != LSM6DSV_ID)
  {
    st_result = ST_FAIL;
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
    lsm6dsv_xl_data_rate_set(&lsm6dsv_obj.Ctx, LSM6DSV_ODR_AT_60Hz);
    /* Set full scale */
    lsm6dsv_xl_full_scale_set(&lsm6dsv_obj.Ctx, LSM6DSV_4g);
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
    st_result = ST_PASS;

    for (i = 0; i < 3; i++)
    {
      if ((SELF_TEST_MIN_ST_LIMIT_mg > test_val[i]) || (test_val[i] > SELF_TEST_MAX_ST_LIMIT_mg))
      {
        st_result = ST_FAIL;
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
        st_result = ST_FAIL;
      }
    }

    /* Disable Self Test */
    lsm6dsv_gy_self_test_set(&lsm6dsv_obj.Ctx, LSM6DSV_GY_ST_DISABLE);
    /* Disable sensor. */
    lsm6dsv_xl_data_rate_set(&lsm6dsv_obj.Ctx, LSM6DSV_ODR_OFF);
  }

  if (st_result == ST_PASS)
  {
    sprintf((char *) tx_buffer, "LSM6DSV Self Test - PASS\r\n");
  }

  else
  {
    sprintf((char *) tx_buffer, "LSM6DSV Self Test - FAIL\r\n");
  }

  tx_com(tx_buffer, strlen((char const *) tx_buffer));
  return (st_result == ST_PASS ? 0 : 1);
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

void lsm6dsv_config_accel(uint8_t rate, uint8_t range)
{
  //TODO if chip sampling rate is lower than Shimmer sampling, enable pin
  //interrupt to only read data from chip when it's ready pin_int.drdy_xl =
  //PROPERTY_ENABLE; lsm6dsv_pin_int1_route_set(&lsm6dsv_obj.Ctx, &pin_int);

  /* Set Output Data Rate.
   * Selected data rate have to be equal or greater with respect
   * with MLC data rate.
   */
  lsm6dsv_xl_data_rate_set(&lsm6dsv_obj.Ctx, LSM6DSV_ODR_AT_1920Hz);
  /* Set full scale */
  lsm6dsv_xl_full_scale_set(&lsm6dsv_obj.Ctx, (lsm6dsv_xl_full_scale_t) range);

  ///* Configure filtering chain */
  //filt_settling_mask.drdy = PROPERTY_ENABLE;
  //filt_settling_mask.irq_xl = PROPERTY_ENABLE;
  //filt_settling_mask.irq_g = PROPERTY_ENABLE;
  //lsm6dsv_filt_settling_mask_set(&lsm6dsv_obj.Ctx, filt_settling_mask);
  //lsm6dsv_filt_xl_lp2_set(&lsm6dsv_obj.Ctx, PROPERTY_ENABLE);
  //lsm6dsv_filt_xl_lp2_bandwidth_set(&lsm6dsv_obj.Ctx, LSM6DSV_XL_STRONG);
}

void lsm6dsv_config_gyro(uint8_t rate, uint8_t range)
{
  lsm6dsv_gy_data_rate_set(&lsm6dsv_obj.Ctx, LSM6DSV_ODR_AT_1920Hz);
  lsm6dsv_gy_full_scale_set(&lsm6dsv_obj.Ctx, range);
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

void lsm6dsv_status_get(void)
{
  lsm6dsv_data_ready_t drdy;
  lsm6dsv_flag_data_ready_get(&lsm6dsv_obj.Ctx, &drdy);
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
