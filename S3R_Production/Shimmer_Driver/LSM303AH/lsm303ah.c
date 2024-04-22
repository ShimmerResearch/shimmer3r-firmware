/******************************************************************************
 * @file    self_test.c
 * @author  Sensors Software Solution Team
 * @brief   This file implement the self test procedure.
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
 * - STEVAL_MKI109V3 + STEVAL-MKI173V1
 * - NUCLEO_F411RE + STEVAL-MKI173V1
 * - DISCOVERY_SPC584B + STEVAL-MKI173V1
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

// Based on https://github.com/STMicroelectronics/STMems_Standard_C_drivers/tree/master/lsm303ah_STdC/examples
#if defined(STEVAL_MKI109V3)
/* MKI109V3: Define communication interface */
#define SENSOR_BUS hspi2
/* MKI109V3: Vdd and Vddio power supply values */
#define PWM_3V3 915

#elif defined(NUCLEO_F411RE)
/* NUCLEO_F411RE: Define communication interface */
#define SENSOR_BUS hi2c1

#elif defined(SPC584B_DIS)
/* DISCOVERY_SPC584B: Define communication interface */
#define SENSOR_BUS I2CD1

#elif defined(SHIMMER3R)
#define SENSOR_BUS hspi2

#define CS_PORT CS_LSM303AH_GPIO_Port
#define CS_PIN CS_LSM303AH_Pin

#endif

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>

#if defined(NUCLEO_F411RE)
#include "lsm303ah_reg.h"

#include "stm32f4xx_hal.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"

#elif defined(STEVAL_MKI109V3)
#include "lsm303ah_reg.h"

#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"

#elif defined(SPC584B_DIS)
#include "lsm303ah_reg.h"

#include "components.h"

#elif defined(SHIMMER3R)
#include "lsm303ah.h"
#include "lsm303ah-pid/lsm303ah_reg.h"

#include "stm32u5xx_hal.h"
#include "usart.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"

#endif

typedef union
{
  int16_t i16bit[3];
  uint8_t u8bit[6];
} axis3bit16_t;

typedef struct
{
  void *hbus;
  uint8_t i2c_address;
  GPIO_TypeDef *cs_port;
  uint16_t cs_pin;
} sensbus_t;

/* Private macro -------------------------------------------------------------*/

#define    BOOT_TIME            20 //ms
#define    WAIT_XL_TIME        200 //ms
#define    WAIT_MAG_TIME_01     20 //ms
#define    WAIT_MAG_TIME_02     60 //ms

#define    SELF_TEST_SAMPLES_XL            5 //number of samples
#define    SELF_TEST_SAMPLES_MAG          50 //number of samples

/* Self test limits. */
#define    SELF_TEST_MIN_ST_LIMIT_mg      70.0f
#define    SELF_TEST_MAX_ST_LIMIT_mg    1500.0f
#define    SELF_TEST_MIN_ST_LIMIT_mG      15.0f
#define    SELF_TEST_MAX_ST_LIMIT_mG     500.0f

/* Self test results. */
#define    ST_PASS     1U
#define    ST_FAIL     0U

/* Private variables ---------------------------------------------------------*/
#if defined(STEVAL_MKI109V3)
static sensbus_t xl_bus  = {&SENSOR_BUS,
                            0,
                            CS_up_GPIO_Port,
                            CS_up_Pin
                           };
static sensbus_t mag_bus = {&SENSOR_BUS,
                            0,
                            CS_up_GPIO_Port,
                            CS_up_Pin
                           };
#elif defined(NUCLEO_F411RE) || defined(SPC584B_DIS)
static sensbus_t xl_bus  = {&SENSOR_BUS,
                            LSM303AH_I2C_ADD_XL,
                            0,
                            0
                           };
static sensbus_t mag_bus = {&SENSOR_BUS,
                            LSM303AH_I2C_ADD_MG,
                            0,
                            0
                           };
#elif defined(SHIMMER3R)
static sensbus_t xl_bus = {&SENSOR_BUS,
                           0,
                           CS_PORT,
                           CS_PIN
                          };
static sensbus_t mag_bus = {&SENSOR_BUS,
                            0,
                            CS_PORT,
                            CS_PIN
                          };
#endif

static stmdev_ctx_t dev_ctx_xl;
static stmdev_ctx_t dev_ctx_mg;

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
    uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
    uint16_t len);
static void tx_com(uint8_t *tx_buffer, uint16_t len);
static void platform_delay(uint32_t ms);
#if !defined(SHIMMER3R)
static void platform_init(void);
#endif

/* Main Example --------------------------------------------------------------*/
void lsm303ah_self_test(void)
{
  lsm303ah_reg_t reg;
  uint8_t tx_buffer[1000];
  float meas_st_off[3];
  int16_t data_raw[3];
  float meas_st_on[3];
  float test_val[3];
  uint8_t st_result;
  uint8_t i, j;
  /* Initialize self test results */
  st_result = ST_PASS;

  /* Check device ID */
  lsm303ah_xl_device_id_get(&dev_ctx_xl, &reg.byte);

  if (reg.byte != LSM303AH_ID_XL)
    while (1)
      ; /*manage here device not found */

  lsm303ah_mg_device_id_get(&dev_ctx_mg, &reg.byte);

  if (reg.byte != LSM303AH_ID_MG)
    while (1)
      ; /*manage here device not found */

  /* Restore default configuration */
  lsm303ah_xl_reset_set(&dev_ctx_xl, PROPERTY_ENABLE);

  do
  {
    lsm303ah_xl_reset_get(&dev_ctx_xl, &reg.byte);
  } while (reg.byte);

  lsm303ah_mg_reset_set(&dev_ctx_mg, PROPERTY_ENABLE);

  do
  {
    lsm303ah_mg_reset_get(&dev_ctx_mg, &reg.byte);
  } while (reg.byte);

  /* Enable Block Data Update. */
  lsm303ah_xl_block_data_update_set(&dev_ctx_xl, PROPERTY_ENABLE);
  lsm303ah_mg_block_data_update_set(&dev_ctx_mg, PROPERTY_ENABLE);
  /* Set full scale to 2g. */
  lsm303ah_xl_full_scale_set(&dev_ctx_xl, LSM303AH_XL_2g);
  /* Set Output Data Rate. */
  lsm303ah_xl_data_rate_set(&dev_ctx_xl, LSM303AH_XL_ODR_50Hz_HR);
  /* Wait stable output */
  platform_delay(WAIT_XL_TIME);

  /* Check if new value available */
  do
  {
    lsm303ah_xl_status_reg_get(&dev_ctx_xl, &reg.status_a);
  } while (!reg.status_a.drdy);

  /* Read dummy data and discard it */
  lsm303ah_acceleration_raw_get(&dev_ctx_xl, data_raw);

  /* Read samples and get the average vale for each axis */
  for (i = 0; i < SELF_TEST_SAMPLES_XL; i++)
  {
    /* Check if new value available */
    do
    {
      lsm303ah_xl_status_reg_get(&dev_ctx_xl, &reg.status_a);
    } while (!reg.status_a.drdy);

    /* Read data and accumulate the mg value */
    lsm303ah_acceleration_raw_get(&dev_ctx_xl, data_raw);

    for (j = 0; j < 3; j++)
    {
      meas_st_off[j] += lsm303ah_from_fs2g_to_mg(data_raw[j]);
    }
  }

  /* Calculate the mg average values */
  for (i = 0; i < 3; i++)
  {
    meas_st_off[i] /= SELF_TEST_SAMPLES_XL;
  }

  /* Enable Self Test positive (or negative) */
  lsm303ah_xl_self_test_set(&dev_ctx_xl, LSM303AH_XL_ST_POSITIVE);
  //lsm303ah_xl_self_test_set(&dev_ctx_xl, LSM303AH_XL_ST_NEGATIVE);
  /* Wait stable output */
  platform_delay(WAIT_XL_TIME);

  /* Check if new value available */
  do
  {
    lsm303ah_xl_status_reg_get(&dev_ctx_xl, &reg.status_a);
  } while (!reg.status_a.drdy);

  /* Read dummy data and discard it */
  lsm303ah_acceleration_raw_get(&dev_ctx_xl, data_raw);

  /* Read samples and get the average vale for each axis */
  for (i = 0; i < SELF_TEST_SAMPLES_XL; i++)
  {
    /* Check if new value available */
    do
    {
      lsm303ah_xl_status_reg_get(&dev_ctx_xl, &reg.status_a);
    } while (!reg.status_a.drdy);

    /* Read data and accumulate the mg value */
    lsm303ah_acceleration_raw_get(&dev_ctx_xl, data_raw);

    for (j = 0; j < 3; j++)
    {
      meas_st_on[j] += lsm303ah_from_fs2g_to_mg(data_raw[j]);
    }
  }

  /* Calculate the mg average values */
  for (i = 0; i < 3; i++)
  {
    meas_st_on[i] /= SELF_TEST_SAMPLES_XL;
  }

  /* Calculate the mg values for self test */
  for (i = 0; i < 3; i++)
  {
    test_val[i] = fabs((meas_st_on[i] - meas_st_off[i]));
  }

  /* Check self test limit */
  for (i = 0; i < 3; i++)
  {
    if (( SELF_TEST_MIN_ST_LIMIT_mg > test_val[i])
        || (test_val[i] > SELF_TEST_MAX_ST_LIMIT_mg))
    {
      st_result = ST_FAIL;
    }

    tx_com(tx_buffer, strlen((char const*) tx_buffer));
  }

  /* Disable Self Test */
  lsm303ah_xl_self_test_set(&dev_ctx_xl, LSM303AH_XL_ST_DISABLE);
  /* Disable sensor. */
  lsm303ah_xl_data_rate_set(&dev_ctx_xl, LSM303AH_XL_ODR_OFF);
  /* Temperature compensation enable */
  lsm303ah_mg_offset_temp_comp_set(&dev_ctx_mg, PROPERTY_ENABLE);
  /* Set restore magnetic condition policy */
  lsm303ah_mg_set_rst_mode_set(&dev_ctx_mg, LSM303AH_MG_SET_SENS_ODR_DIV_63);
  /* Set power mode */
  lsm303ah_mg_power_mode_set(&dev_ctx_mg, LSM303AH_MG_HIGH_RESOLUTION);
  /* Set Output Data Rate */
  lsm303ah_mg_data_rate_set(&dev_ctx_mg, LSM303AH_MG_ODR_100Hz);
  /* Set Operating mode */
  lsm303ah_mg_operating_mode_set(&dev_ctx_mg, LSM303AH_MG_CONTINUOUS_MODE);
  /* Wait stable output */
  platform_delay(WAIT_MAG_TIME_01);

  /* Check if new value available */
  do
  {
    lsm303ah_mg_data_ready_get(&dev_ctx_mg, &reg.byte);
  } while (!reg.byte);

  /* Read dummy data and discard it */
  lsm303ah_magnetic_raw_get(&dev_ctx_mg, data_raw);
  /* Read samples and get the average vale for each axis */
  memset(meas_st_off, 0x00, 3 * sizeof(float));

  for (i = 0; i < SELF_TEST_SAMPLES_MAG; i++)
  {
    /* Check if new value available */
    do
    {
      lsm303ah_mg_data_ready_get(&dev_ctx_mg, &reg.byte);
    } while (!reg.byte);

    /* Read data and accumulate the mg value */
    lsm303ah_magnetic_raw_get(&dev_ctx_mg, data_raw);

    for (j = 0; j < 3; j++)
    {
      meas_st_off[j] += lsm303ah_from_lsb_to_mgauss(data_raw[j]);
    }
  }

  /* Calculate the mg average values */
  for (i = 0; i < 3; i++)
  {
    meas_st_off[i] /= SELF_TEST_SAMPLES_MAG;
  }

  /* Enable Self Test */
  lsm303ah_mg_self_test_set(&dev_ctx_mg, PROPERTY_ENABLE);
  /* Wait stable output */
  platform_delay(WAIT_MAG_TIME_02);
  /* Read samples and get the average vale for each axis */
  memset(meas_st_on, 0x00, 3 * sizeof(float));

  for (i = 0; i < SELF_TEST_SAMPLES_MAG; i++)
  {
    /* Check if new value available */
    do
    {
      lsm303ah_mg_data_ready_get(&dev_ctx_mg, &reg.byte);
    } while (!reg.byte);

    /* Read data and accumulate the mg value */
    lsm303ah_magnetic_raw_get(&dev_ctx_mg, data_raw);

    for (j = 0; j < 3; j++)
    {
      meas_st_on[j] += lsm303ah_from_lsb_to_mgauss(data_raw[j]);
    }
  }

  /* Calculate the mg average values */
  for (i = 0; i < 3; i++)
  {
    meas_st_on[i] /= SELF_TEST_SAMPLES_MAG;
  }

  st_result = ST_PASS;

  /* Calculate the mg values for self test */
  for (i = 0; i < 3; i++)
  {
    test_val[i] = fabs((meas_st_on[i] - meas_st_off[i]));
  }

  /* Check self test limit */
  for (i = 0; i < 3; i++)
  {
    if (( SELF_TEST_MIN_ST_LIMIT_mG > test_val[i])
        || (test_val[i] > SELF_TEST_MAX_ST_LIMIT_mG))
    {
      st_result = ST_FAIL;
    }
  }

  /* Disable Self Test */
  lsm303ah_mg_self_test_set(&dev_ctx_mg, PROPERTY_DISABLE);
  /* Disable sensor. */
  lsm303ah_mg_operating_mode_set(&dev_ctx_mg, LSM303AH_MG_POWER_DOWN);

  if (st_result == ST_PASS)
  {
    sprintf((char*) tx_buffer, "Self Test - PASS\r\n");
  }
  else
  {
    sprintf((char*) tx_buffer, "Self Test - FAIL\r\n");
  }

  tx_com(tx_buffer, strlen((char const*) tx_buffer));
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
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
    uint16_t len)
{
  sensbus_t *sensbus = (sensbus_t*) handle;
#if defined(NUCLEO_F411RE)
  /* Write multiple command */
  reg |= 0x80;
  HAL_I2C_Mem_Write(sensbus->hbus, sensbus->i2c_address, reg,
                    I2C_MEMADD_SIZE_8BIT, (uint8_t*) bufp, len, 1000);
#elif defined(STEVAL_MKI109V3) | defined(SHIMMER3R)
  /* Write multiple command */
  reg |= 0x40;
  HAL_GPIO_WritePin(sensbus->cs_port, sensbus->cs_pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(sensbus->hbus, &reg, 1, 1000);
  HAL_SPI_Transmit(sensbus->hbus, (uint8_t*) bufp, len, 1000);
  HAL_GPIO_WritePin(sensbus->cs_port, sensbus->cs_pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  reg |= 0x80;
  i2c_lld_write(sensbus->hbus, sensbus->i2c_address & 0xFE, reg,
               (uint8_t*) bufp, len);
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
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
    uint16_t len)
{
  sensbus_t *sensbus = (sensbus_t*) handle;
#if defined(NUCLEO_F411RE)
  /* Read multiple command */
  reg |= 0x80;
  HAL_I2C_Mem_Read(sensbus->hbus, sensbus->i2c_address, reg,
                   I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
#elif defined(STEVAL_MKI109V3) | defined(SHIMMER3R)
  /* Read multiple command */
  reg |= 0xC0;
  HAL_GPIO_WritePin(sensbus->cs_port, sensbus->cs_pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(sensbus->hbus, &reg, 1, 1000);
  HAL_SPI_Receive(sensbus->hbus, bufp, len, 1000);
  HAL_GPIO_WritePin(sensbus->cs_port, sensbus->cs_pin, GPIO_PIN_SET);
#elif defined(SPC584B_DIS)
  i2c_lld_read(sensbus->hbus, sensbus->i2c_address & 0xFE, reg, bufp,
               len);
#endif
  return 0;
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
  printf((char*) tx_buffer, len);
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

#elif defined(SHIMMER3R)
void lsm303ah_driver_init(void)
{
  /* Initialize mems driver interface */
  dev_ctx_xl.write_reg = platform_write;
  dev_ctx_xl.read_reg = platform_read;
  dev_ctx_xl.handle = (void*) &xl_bus;
  dev_ctx_mg.write_reg = platform_write;
  dev_ctx_mg.read_reg = platform_read;
  dev_ctx_mg.handle = (void*) &mag_bus;
}

void lsm303ah_power_on(void)
{
  set_power_spi2_bus(true, SPI2_CHIP_INDEX_LSM303AH);
}

void lsm303ah_power_off(void)
{
  set_power_spi2_bus(false, SPI2_CHIP_INDEX_LSM303AH);
}

void lsm303ah_accelInit(uint8_t samplingRate, uint8_t range, uint8_t lowPower,
    uint8_t highresolution)
{
#if defined(SHIMMER3)
  uint8_t i2c_buffer[2];
#elif defined(SHIMMER3R)
  uint8_t spi_buffer;
#endif
  uint8_t highFreq;

  if (samplingRate > 7 && !lowPower)
  {
    highFreq = 1;
    switch (samplingRate)
    {
    case 8:
      // 1600 Hz => ODR[3:0] 0101 = 5
      samplingRate = 0x05;
      break;
    case 9:
      // 3200 Hz => ODR[3:0] 0110 = 6
      samplingRate = 0x06;
      break;
    case 10:
      // 6400 Hz => ODR[3:0] 0111 = 7
      samplingRate = 0x07;
      break;
    default:
      break;
    }
  }
  else
  {
    highFreq = 0;
  }

  //Configure Accelerometer
  //write CTRL1_A register
#if defined(SHIMMER3)
  I2C_Set_Slave_Address(LSM303AHTR_ACCEL_ADDR);
  i2c_buffer[0] = LSM303AH_CTRL1_A;
  i2c_buffer[1] = (samplingRate << 4) + (range << 2) + (highFreq << 1);
  I2C_Write_Packet_To_Sensor(i2c_buffer, 2);
#elif defined(SHIMMER3R)
  lsm303ah_reg_t reg;
  reg.ctrl1_a.odr = samplingRate;
  reg.ctrl1_a.fs = range;
  reg.ctrl1_a.hf_odr = highFreq;
  reg.ctrl1_a.bdu = PROPERTY_DISABLE;
  lsm303ah_write_reg(&dev_ctx_xl, LSM303AH_CTRL1_A, &reg.byte, 1);
#endif
}

void lsm303ah_magInit(uint8_t samplingrate)
{
  //Configure Magnetometer
  //write CFG_REG_A_M register
#if defined(SHIMMER3)
  uint8_t i2c_buffer[2];
  I2C_Set_Slave_Address(LSM303AHTR_MAG_ADDR);
  i2c_buffer[0] = LSM303AH_CFG_REG_A_M;
  i2c_buffer[1] = samplingrate << 2;
  I2C_Write_Packet_To_Sensor(i2c_buffer, 2);
#elif defined(SHIMMER3R)
  lsm303ah_reg_t reg;
  reg.cfg_reg_a_m.comp_temp_en = PROPERTY_DISABLE;
  reg.cfg_reg_a_m.reboot = PROPERTY_DISABLE;
  reg.cfg_reg_a_m.soft_rst = PROPERTY_DISABLE;
  reg.cfg_reg_a_m.lp = PROPERTY_DISABLE;
  reg.cfg_reg_a_m.odr = samplingrate;
  reg.cfg_reg_a_m.md = LSM303AH_MG_CONTINUOUS_MODE;
  lsm303ah_write_reg(&dev_ctx_mg, LSM303AH_CFG_REG_A_M, &reg.byte, 1);
#endif
}

void lsm303ah_getAccel(uint8_t *buf)
{
#if defined(SHIMMER3)
  I2C_Set_Slave_Address(LSM303AHTR_ACCEL_ADDR);
  *buf = LSM303AH_OUT_X_L_A;
  I2C_Read_Packet_From_Sensor(buf, 6);
#elif defined(SHIMMER3R)
  lsm303ah_read_reg(&dev_ctx_xl, LSM303AH_OUT_X_L_A, buf, 6);
#endif
}

void lsm303ah_getMag(uint8_t *buf)
{
  static uint8_t last_data[7] = { 0, 0, 0, 0, 0, 0, 0 };
#if defined(SHIMMER3)
  I2C_Set_Slave_Address(LSM303AHTR_MAG_ADDR);
  *buf = STATUS_REG_M;
  I2C_Read_Packet_From_Sensor(buf, 1);
#elif defined(SHIMMER3R)
  lsm303ah_mg_status_get(&dev_ctx_mg, buf);
#endif

  if (buf[0] & 0x08)
  {
#if defined(SHIMMER3)
    *buf = LSM303AH_OUTX_L_REG_M;
    I2C_Read_Packet_From_Sensor(buf, 6);
#elif defined(SHIMMER3R)
    lsm303ah_read_reg(&dev_ctx_mg, LSM303AH_OUTX_L_REG_M, buf, 6);
#endif
    memcpy(last_data, buf, 6);
    last_data[6] = 1;
  }
  else
  {
    if (last_data[6] == 1)
    {
      memcpy(buf, last_data, 6);
    }
  }
}

void lsm303ah_sleep(void)
{
  lsm303ah_xl_data_rate_set(&dev_ctx_xl, LSM303AH_XL_ODR_OFF);
  lsm303ah_mg_operating_mode_set(&dev_ctx_mg, LSM303AH_MG_POWER_DOWN);
}

#endif
