/*
 * hal_bmp3.c
 *
 *  Created on: Apr 17, 2024
 *      Author: MarkNolan
 */

#include "hal_bmp3.h"

#include "gpio.h"
#include "spi.h"
#include "stm32u5xx.h"
#include <string.h>

#include "BMP3_SensorAPI/bmp3.h"
#include "BMP3_SensorAPI/self-test/bmp3_selftest.h"

#define SENSOR_BUS hspi1

#define CS_PORT    CS_BMP390_GPIO_Port
#define CS_PIN     CS_BMP390_Pin

struct bmp3_dev bmp3;

bool isDrdyIntEnabled = false;

/* Variable to store the device address */
static uint8_t dev_addr;

/* Array to store calibration data */
uint8_t calib_bytes[BMP3_LEN_CALIB_DATA] = { 0 };

static BMP3_INTF_RET_TYPE
bmp3_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BMP3_INTF_RET_TYPE
bmp3_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
static void bmp3_delay_us(uint32_t periodUS, void *intf_ptr);
static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len);

void bmp3_driver_init(void)
{
  bmp3.read = bmp3_spi_read;
  bmp3.write = bmp3_spi_write;
  bmp3.intf = BMP3_SPI_INTF;
  bmp3.delay_us = bmp3_delay_us;
  bmp3.intf_ptr = &dev_addr;
  bmp3.dummy_byte = 1;

  save_calib_data_bytes();
}

void bmp3_selectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void bmp3_unselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

int8_t bmp3_self_test(void)
{
  int8_t result;
  result = bmp3_selftest_check(&bmp3);

  //if (result == BMP3_SENSOR_OK)
  //{
  //  SHIMMER_PRINTF("BMP390 Self Test - PASS\r\n");
  //}
  //else
  //{
  //  SHIMMER_PRINTF("BMP390 Self Test - FAIL\r\n");
  //  bmp3_check_rslt("BMP390", result);
  //}

  return result;
}

void bmp3_configure(float shimmerSamplingFreq, uint8_t rate, uint8_t overSamplingRatio)
{
  int8_t rslt;

  /* Used to select the settings user needs to change */
  uint16_t settings_sel;

  struct bmp3_settings settings;

  /* Select the pressure and temperature sensor to be enabled */
  settings.press_en = BMP3_ENABLE;
  settings.temp_en = BMP3_ENABLE;

  /* Select the output data rate and over sampling settings for pressure and temperature */
  //settings.odr_filter.press_os = BMP3_NO_OVERSAMPLING;
  //settings.odr_filter.temp_os = BMP3_NO_OVERSAMPLING;
  settings.odr_filter.press_os = overSamplingRatio;
  settings.odr_filter.temp_os = overSamplingRatio;
  settings.odr_filter.odr = rate;

  /* Assign the settings which needs to be set in the sensor */
  settings_sel = BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN | BMP3_SEL_PRESS_OS
      | BMP3_SEL_TEMP_OS | BMP3_SEL_ODR;

  isDrdyIntEnabled = false;
  if (bmp3_is_shimmer_freq_higher(shimmerSamplingFreq, rate))
  {
    isDrdyIntEnabled = true;
    settings_sel |= BMP3_SEL_DRDY_EN | BMP3_SEL_LATCH;
  }

  rslt = bmp3_set_sensor_settings(settings_sel, &settings, &bmp3);
  if (rslt == BMP3_SENSOR_OK)
  {
    settings.op_mode = BMP3_MODE_NORMAL;
    rslt = bmp3_set_op_mode(&settings, &bmp3);
  }
}

HAL_StatusTypeDef bmp3_pressure_temperature_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[] = { BMP3_REG_DATA | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(&SENSOR_BUS, &txBuff[0], buf, sizeof(txBuff));
  return ret;
}

bool bmp3_is_drdy_int_enabled(void)
{
  return isDrdyIntEnabled;
}

bool bmp3_is_shimmer_freq_higher(float shimmerSamplingFreq, uint8_t rate)
{
  return shimmerSamplingFreq > bmp3_get_sensor_freq_from_rate(rate);
}

float bmp3_get_sensor_freq_from_rate(uint8_t rate)
{
  float sensorFreq = 0.0;
  switch (rate)
  {
  case BMP3_ODR_200_HZ:
    sensorFreq = 200.0;
    break;
  case BMP3_ODR_100_HZ:
    sensorFreq = 100.0;
    break;
  case BMP3_ODR_50_HZ:
    sensorFreq = 50.0;
    break;
  case BMP3_ODR_25_HZ:
    sensorFreq = 25.0;
    break;
  case BMP3_ODR_12_5_HZ:
    sensorFreq = 12.5;
    break;
  case BMP3_ODR_6_25_HZ:
    sensorFreq = 6.25;
    break;
  case BMP3_ODR_3_1_HZ:
    sensorFreq = 3.1;
    break;
  case BMP3_ODR_1_5_HZ:
    sensorFreq = 1.5;
    break;
  case BMP3_ODR_0_78_HZ:
    sensorFreq = 0.78;
    break;
  case BMP3_ODR_0_39_HZ:
    sensorFreq = 0.39;
    break;
  case BMP3_ODR_0_2_HZ:
    sensorFreq = 0.2;
    break;
  case BMP3_ODR_0_1_HZ:
    sensorFreq = 0.1;
    break;
  case BMP3_ODR_0_05_HZ:
    sensorFreq = 0.05;
    break;
  case BMP3_ODR_0_02_HZ:
    sensorFreq = 0.02;
    break;
  case BMP3_ODR_0_01_HZ:
    sensorFreq = 0.01;
    break;
  case BMP3_ODR_0_006_HZ:
    sensorFreq = 0.006;
    break;
  case BMP3_ODR_0_003_HZ:
    sensorFreq = 0.003;
    break;
  case BMP3_ODR_0_001_HZ:
    sensorFreq = 0.001;
    break;
  default:
    sensorFreq = 0.0;
    break;
  }
  return sensorFreq;
}

int8_t bmp3_restore_default_config(void)
{
  int8_t rslt;
  /* Reset the sensor */
  rslt = bmp3_soft_reset(&bmp3);

  return rslt;
}

uint8_t *get_bmp_calib_data_bytes(void)
{
  return &calib_bytes[0];
}

uint8_t get_bmp_calib_data_bytes_len(void)
{
  return BMP_LEN_CALIB_DATA;
}

int8_t save_calib_data_bytes(void)
{
  int8_t rslt;
  uint8_t reg_addr = BMP3_REG_CALIB_DATA;

  /* Read the calibration data from the sensor */
  rslt = bmp3_get_regs(reg_addr, calib_bytes, BMP3_LEN_CALIB_DATA, &bmp3);

  return rslt;
}

void bmp3_check_rslt(const char api_name[], int8_t rslt, char *outputStr)
{
  switch (rslt)
  {
  case BMP3_OK:
    /* Do nothing */
    break;
  case BMP3_E_NULL_PTR:
    sprintf(outputStr, "API [%s] Error [%d] : Null pointer\r\n", api_name, rslt);
    break;
  case BMP3_E_COMM_FAIL:
    sprintf(outputStr, "API [%s] Error [%d] : Communication failure\r\n", api_name, rslt);
    break;
  case BMP3_E_INVALID_LEN:
    sprintf(outputStr, "API [%s] Error [%d] : Incorrect length parameter\r\n",
        api_name, rslt);
    break;
  case BMP3_E_DEV_NOT_FOUND:
    sprintf(outputStr, "API [%s] Error [%d] : Device not found\r\n", api_name, rslt);
    break;
  case BMP3_E_CONFIGURATION_ERR:
    sprintf(outputStr, "API [%s] Error [%d] : Configuration Error\r\n", api_name, rslt);
    break;
  case BMP3_W_SENSOR_NOT_ENABLED:
    sprintf(outputStr, "API [%s] Error [%d] : Warning when Sensor not enabled\r\n",
        api_name, rslt);
    break;
  case BMP3_W_INVALID_FIFO_REQ_FRAME_CNT:
    sprintf(outputStr,
        "API [%s] Error [%d] : Warning when Fifo watermark level is "
        "not in limit\r\n",
        api_name, rslt);
    break;
  default:
    sprintf(outputStr, "API [%s] Error [%d] : Unknown error code\r\n", api_name, rslt);
    break;
  }
}

/*!
 * SPI write function map to Shimmer platform
 */
static BMP3_INTF_RET_TYPE
bmp3_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  HAL_StatusTypeDef retVal;

  bmp3_selectDevice();
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  if (retVal)
  {
    bmp3_unselectDevice();
    return 1;
  }
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, (uint8_t *) reg_data, (uint16_t) len, 1000);
  if (retVal)
  {
    bmp3_unselectDevice();
    return 1;
  }
  bmp3_unselectDevice();
  return (BMP3_INTF_RET_TYPE) retVal;
}

/*!
 * SPI read function map to Shimmer platform
 */
static BMP3_INTF_RET_TYPE
bmp3_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  HAL_StatusTypeDef retVal;

  bmp3_selectDevice();
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  if (retVal)
  {
    bmp3_unselectDevice();
    return 1;
  }
  retVal = HAL_SPI_Receive(&SENSOR_BUS, (uint8_t *) reg_data, (uint16_t) len, 1000);
  if (retVal)
  {
    bmp3_unselectDevice();
    return 1;
  }
  bmp3_unselectDevice();
  return (BMP3_INTF_RET_TYPE) retVal;
}

/*!
 * Delay function map to Shimmer platform
 */
static void bmp3_delay_us(uint32_t periodUS, void *intf_ptr)
{
  HAL_Delay(periodUS / 1000);
}

static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len)
{
  HAL_StatusTypeDef ret;
  bmp3_selectDevice();
  ret = HAL_SPI_TransmitReceive_DMA(handle, txBufp, rxBufp, len);
  return ret;
}
