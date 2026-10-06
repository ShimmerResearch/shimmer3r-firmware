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
 * - STEVAL_MKI109V3 + STEVAL-MKI181V1
 * - NUCLEO_F411RE + X_NUCLEO_IKS01A3
 * - DISCOVERY_SPC584B + STEVAL-MKI181V1
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
#define SENSOR_BUS hi2c1

#endif

/* Includes ------------------------------------------------------------------*/
#include "lis2mdl-pid/lis2mdl_reg.h"
#include "lis2mdl.h"
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
#include "i2c.h"
#include "stm32u5xx_hal.h"
#include "tim.h"
#endif

#include "hal_FactoryTest.h"

/* Private macro -------------------------------------------------------------*/

#define BOOT_TIME                 20 //ms
#define WAIT_TIME_01              20 //ms
#define WAIT_TIME_02              60 //ms

#define SAMPLES                   50 //number of samples

/* Polling timeouts. At 100 Hz a conversion completes every 10 ms, so 50 ms is
 * five missed conversions; the soft reset completes in microseconds. */
#define DATA_READY_TIMEOUT_MS     50
#define RESET_TIMEOUT_MS          20

/* DRDY/INT pin test. Reading the output registers drops DRDY within
 * microseconds, so the pin is checked after a short busy-wait rather than a
 * HAL_Delay(1), which waits 1-2 ms - long enough for the next 100 Hz
 * conversion to land and re-assert the pin on a healthy part. */
#define DRDY_TEST_ATTEMPTS        3
#define DRDY_TEST_RISE_TIMEOUT_MS 100
#define DRDY_TEST_FALL_US         50

/* Self test limits. */
#define MIN_ST_LIMIT_mG           15.0f
#define MAX_ST_LIMIT_mG           500.0f

/* Self test results. */
#define ST_PASS                   1U
#define ST_FAIL                   0U

/* Private variables ---------------------------------------------------------*/
static LIS2MDL_Object_t lis2mdl_obj;

static bool isDrdyIntEnabled = false;

/* Times the last DRDY pin test found the pin re-asserted by a new conversion
 * and checked again. Reported by the factory test for diagnosis. */
static uint8_t drdyRecheckCount = 0;

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
static int32_t
platform_read_raw_data_dma(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void tx_com(uint8_t *tx_buffer, uint16_t len);
static void platform_delay(uint32_t ms);
#if !defined(SHIMMER3R)
static void platform_init(void);
#endif

/* Main Example --------------------------------------------------------------*/

#if defined(SHIMMER3R)
void lis2mdl_driver_init(void)
{
  /* Initialize mems driver interface */
  lis2mdl_obj.Ctx.write_reg = platform_write;
  lis2mdl_obj.Ctx.read_reg = platform_read;
  lis2mdl_obj.Ctx.mdelay = platform_delay;
  lis2mdl_obj.Ctx.handle = &SENSOR_BUS;

  //lis2mdl_obj.IO.BusType = LIS2MDL_I2C_BUS;
}

/* Wait for STATUS_REG ZYXDA. Returns SELF_TEST_PASS once a new sample is
 * ready, SELF_TEST_FAIL_CHIP_DETECTION on an I2C error and
 * SELF_TEST_FAIL_SIGNAL_ISSUE if no conversion completes in time. */
static self_test_result_t lis2mdl_wait_data_ready(void)
{
  uint8_t drdy = 0;
  uint32_t tickStart = HAL_GetTick();

  do
  {
    if (lis2mdl_mag_data_ready_get(&lis2mdl_obj.Ctx, &drdy) != 0)
    {
      return SELF_TEST_FAIL_CHIP_DETECTION;
    }
    if (drdy)
    {
      return SELF_TEST_PASS;
    }
  } while ((HAL_GetTick() - tickStart) < DATA_READY_TIMEOUT_MS);

  return SELF_TEST_FAIL_SIGNAL_ISSUE;
}

/* Average SAMPLES fresh readings per axis, in mG. */
static self_test_result_t lis2mdl_average_mg(float *avg)
{
  int16_t data_raw[3];
  uint8_t i;
  uint8_t j;
  self_test_result_t result;

  memset(avg, 0x00, 3 * sizeof(float));
  for (i = 0; i < SAMPLES; i++)
  {
    result = lis2mdl_wait_data_ready();
    if (result != SELF_TEST_PASS)
    {
      return result;
    }
    if (lis2mdl_magnetic_raw_get(&lis2mdl_obj.Ctx, data_raw) != 0)
    {
      return SELF_TEST_FAIL_CHIP_DETECTION;
    }
    for (j = 0; j < 3; j++)
    {
      avg[j] += lis2mdl_from_lsb_to_mgauss(data_raw[j]);
    }
  }
  for (j = 0; j < 3; j++)
  {
    avg[j] /= SAMPLES;
  }
  return SELF_TEST_PASS;
}

self_test_result_t lis2mdl_self_test(void)
{
  int16_t data_raw[3];
  float val_st_off[3];
  float val_st_on[3];
  float test_val;
  uint8_t whoamI = 0;
  uint8_t i;
  self_test_result_t self_test_result = SELF_TEST_PASS;

  lis2mdl_driver_init();
  drdyRecheckCount = 0;
#if !defined(SHIMMER3R)
  /* Initialize platform specific hardware */
  platform_init();
#endif
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);

#if defined(STEVAL_MKI109V3)
  /* Default SPI mode is 3 wire, so enable 4 wire mode */
  lis2mdl_spi_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_SPI_4_WIRE);
#endif

  /* Check device ID */
  if ((lis2mdl_device_id_get(&lis2mdl_obj.Ctx, &whoamI) != 0) || (whoamI != LIS2MDL_ID))
  {
    return SELF_TEST_FAIL_CHIP_DETECTION;
  }

  if (lis2mdl_set_default_config() != 0)
  {
    return SELF_TEST_FAIL_CHIP_DETECTION;
  }

  /* Set restore magnetic condition policy */
  lis2mdl_set_rst_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_SET_SENS_ODR_DIV_63);
  /* Set power mode */
  lis2mdl_power_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_HIGH_RESOLUTION);
  /* Set Output Data Rate */
  lis2mdl_data_rate_set(&lis2mdl_obj.Ctx, LIS2MDL_ODR_100Hz);
  /* Set Operating mode */
  lis2mdl_operating_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_CONTINUOUS_MODE);
  /* Wait stable output */
  platform_delay(WAIT_TIME_01);

  /* DRDY/INT pin test */
  self_test_result = lis2mdl_drdy_test();

  /* Read dummy data and discard it */
  if (self_test_result == SELF_TEST_PASS)
  {
    self_test_result = lis2mdl_wait_data_ready();
  }
  if (self_test_result == SELF_TEST_PASS
      && lis2mdl_magnetic_raw_get(&lis2mdl_obj.Ctx, data_raw) != 0)
  {
    self_test_result = SELF_TEST_FAIL_CHIP_DETECTION;
  }

  /* Average with self test off, then on */
  if (self_test_result == SELF_TEST_PASS)
  {
    self_test_result = lis2mdl_average_mg(val_st_off);
  }
  if (self_test_result == SELF_TEST_PASS)
  {
    /* Enable Self Test */
    lis2mdl_self_test_set(&lis2mdl_obj.Ctx, PROPERTY_ENABLE);
    /* Wait stable output */
    platform_delay(WAIT_TIME_02);
    self_test_result = lis2mdl_average_mg(val_st_on);
    /* Disable Self Test */
    lis2mdl_self_test_set(&lis2mdl_obj.Ctx, PROPERTY_DISABLE);
  }

  /* Check self test limit on the mG difference */
  if (self_test_result == SELF_TEST_PASS)
  {
    for (i = 0; i < 3; i++)
    {
      test_val = fabs((val_st_on[i] - val_st_off[i]));
      if ((MIN_ST_LIMIT_mG > test_val) || (test_val > MAX_ST_LIMIT_mG))
      {
        self_test_result = SELF_TEST_FAIL_SIGNAL_ISSUE;
      }
    }
  }

  /* Disable sensor. */
  lis2mdl_operating_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_POWER_DOWN);
  return self_test_result;
}

/* Checks the DRDY/INT pin follows the data-ready state: it must rise while a
 * conversion is pending and fall once the outputs are read. At 100 Hz a new
 * conversion can land just after the read and legitimately re-assert the pin;
 * STATUS_REG tells that apart from a pin stuck high, and the check is retried. */
self_test_result_t lis2mdl_drdy_test(void)
{
  int16_t data_raw[3];
  uint8_t drdy = 0;
  uint8_t attempt;
  uint32_t tickStart;
  self_test_result_t result = SELF_TEST_FAIL_DRDY_ISSUE;

  drdyRecheckCount = 0;

  /* Set DRDY pin */
  if (lis2mdl_drdy_on_pin_set(&lis2mdl_obj.Ctx, 1) != 0)
  {
    return SELF_TEST_FAIL_CHIP_DETECTION;
  }

  for (attempt = 0; attempt < DRDY_TEST_ATTEMPTS; attempt++)
  {
    /* Running continuously, a conversion is pending within 10 ms. A pin that
     * never rises is stuck low or open. */
    tickStart = HAL_GetTick();
    while (!LIS2MDL_DRDY && ((HAL_GetTick() - tickStart) < DRDY_TEST_RISE_TIMEOUT_MS))
    {
    }
    if (!LIS2MDL_DRDY)
    {
      result = SELF_TEST_FAIL_DRDY_ISSUE;
      break;
    }

    /* Reading the outputs clears data-ready, so the pin must fall */
    if (lis2mdl_magnetic_raw_get(&lis2mdl_obj.Ctx, data_raw) != 0)
    {
      result = SELF_TEST_FAIL_CHIP_DETECTION;
      break;
    }
    delay_us(DRDY_TEST_FALL_US);
    if (!LIS2MDL_DRDY)
    {
      result = SELF_TEST_PASS;
      break;
    }

    /* Still high: correct only if a new conversion completed since the read */
    if (lis2mdl_mag_data_ready_get(&lis2mdl_obj.Ctx, &drdy) != 0)
    {
      result = SELF_TEST_FAIL_CHIP_DETECTION;
      break;
    }
    if (!drdy)
    {
      /* No new data, yet the pin is high: stuck high */
      result = SELF_TEST_FAIL_DRDY_ISSUE;
      break;
    }
    drdyRecheckCount++;
  }

  lis2mdl_drdy_on_pin_set(&lis2mdl_obj.Ctx, 0);

  return result;
}

uint8_t lis2mdl_get_drdy_recheck_count(void)
{
  return drdyRecheckCount;
}

void lis2mdl_configure(float shimmerSamplingFreq, lis2mdl_odr_t rate)
{
  LIS2MDL_Init(&lis2mdl_obj);

  /* Set restore magnetic condition policy */
  lis2mdl_set_rst_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_SET_SENS_ODR_DIV_63);
  /* Set power mode */
  lis2mdl_power_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_HIGH_RESOLUTION);
  /* Set Output Data Rate */
  lis2mdl_data_rate_set(&lis2mdl_obj.Ctx, rate);
  /* Set Operating mode */
  lis2mdl_operating_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_CONTINUOUS_MODE);

  isDrdyIntEnabled = false;
  if (lis2mdl_is_shimmer_freq_higher(shimmerSamplingFreq, rate))
  {
    /* Set DRDY pin */
    lis2mdl_drdy_on_pin_set(&lis2mdl_obj.Ctx, 1);
    isDrdyIntEnabled = true;
  }

  /* Wait stable output */
  platform_delay(WAIT_TIME_01);
}

HAL_StatusTypeDef lis2mdl_mag_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  ret = platform_read_raw_data_dma(lis2mdl_obj.Ctx.handle, LIS2MDL_OUTX_L_REG, buf, 6);
  return ret;
}

bool lis2mdl_is_drdy_int_enabled(void)
{
  return isDrdyIntEnabled;
}

bool lis2mdl_is_shimmer_freq_higher(float shimmerSamplingFreq, lis2mdl_odr_t rate)
{
  return shimmerSamplingFreq > lis2mdl_get_sensor_freq_from_rate(rate);
}

float lis2mdl_get_sensor_freq_from_rate(lis2mdl_odr_t rate)
{
  float sensorFreq = 0.0;
  switch (rate)
  {
  case LIS2MDL_ODR_10Hz:
    sensorFreq = 10.0;
    break;
  case LIS2MDL_ODR_20Hz:
    sensorFreq = 20.0;
    break;
  case LIS2MDL_ODR_50Hz:
    sensorFreq = 50.0;
    break;
  case LIS2MDL_ODR_100Hz:
    sensorFreq = 100.0;
    break;
  default:
    sensorFreq = 0.0;
    break;
  }
  return sensorFreq;
}

int32_t lis2mdl_set_default_config(void)
{
  uint8_t rst = 1;
  uint32_t tickStart;
  int32_t ret;

  /* Restore default configuration */
  ret = lis2mdl_reset_set(&lis2mdl_obj.Ctx, PROPERTY_ENABLE);

  tickStart = HAL_GetTick();
  while ((ret == 0) && rst)
  {
    if ((HAL_GetTick() - tickStart) >= RESET_TIMEOUT_MS)
    {
      ret = -1;
      break;
    }
    ret = lis2mdl_reset_get(&lis2mdl_obj.Ctx, &rst);
  }

  /* Enable Block Data Update */
  if (ret == 0)
  {
    ret = lis2mdl_block_data_update_set(&lis2mdl_obj.Ctx, PROPERTY_ENABLE);
  }
  /* Temperature compensation enable */
  if (ret == 0)
  {
    ret = lis2mdl_offset_temp_comp_set(&lis2mdl_obj.Ctx, PROPERTY_ENABLE);
  }
  return ret;
}

void lis2mdl_sleep(void)
{
  lis2mdl_operating_mode_set(&lis2mdl_obj.Ctx, LIS2MDL_POWER_DOWN);
}
#endif

int32_t lis2mdl_temperature_get(float_t *tempCal)
{
  int16_t tempUncal = 0;
  int32_t res = lis2mdl_temperature_raw_get(&lis2mdl_obj.Ctx, &tempUncal);
  *tempCal = lis2mdl_from_lsb_to_celsius(tempUncal);
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
  int32_t ret = 0;
#if defined(NUCLEO_F411RE)
  /* Write multiple command */
  reg |= 0x80;
  HAL_I2C_Mem_Write(handle, LIS2MDL_I2C_ADD, reg, I2C_MEMADD_SIZE_8BIT,
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
  i2c_lld_write(handle, LIS2MDL_I2C_ADD & 0xFE, reg, (uint8_t *) bufp, len);
#elif defined(SHIMMER3R)
  /* Write multiple command */
  reg |= 0x80;
  ret = (int32_t) HAL_I2C_Mem_Write(handle, LIS2MDL_I2C_ADD, reg,
      I2C_MEMADD_SIZE_8BIT, (uint8_t *) bufp, len, 1000);
#endif
  return ret;
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
  int32_t ret = 0;
#if defined(NUCLEO_F411RE)
  /* Read multiple command */
  reg |= 0x80;
  HAL_I2C_Mem_Read(handle, LIS2MDL_I2C_ADD, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
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
  i2c_lld_read(handle, LIS2MDL_I2C_ADD & 0xFE, reg, bufp, len);
#elif defined(SHIMMER3R)
  /* Read multiple command */
  reg |= 0x80;
  ret = (int32_t) HAL_I2C_Mem_Read(
      handle, LIS2MDL_I2C_ADD, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#endif
  return ret;
}

static int32_t platform_read_raw_data_dma(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
  HAL_StatusTypeDef ret;
  ret = HAL_I2C_Mem_Read_DMA(handle, LIS2MDL_I2C_ADD, reg, I2C_MEMADD_SIZE_8BIT, bufp, len);
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
#elif defined(SHIMMER3R)
#endif
}

#if !defined(SHIMMER3R)
/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
#ifdef STEVAL_MKI109V3
  TIM3->CCR1 = PWM_3V3;
  TIM3->CCR2 = PWM_3V3;
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_Delay(1000);
#endif
}
#endif
