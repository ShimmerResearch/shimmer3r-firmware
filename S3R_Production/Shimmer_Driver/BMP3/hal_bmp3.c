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

/* Variable to store the device address */
static uint8_t dev_addr;

/*!
 * SPI read function map to Shimmer platform
 */
static BMP3_INTF_RET_TYPE
bmp3_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  HAL_StatusTypeDef retVal;

  bmp3_SelectDevice();
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  if (retVal)
  {
    bmp3_UnselectDevice();
    return 1;
  }
  retVal = HAL_SPI_Receive(&SENSOR_BUS, (uint8_t *) reg_data, (uint16_t) len, 1000);
  if (retVal)
  {
    bmp3_UnselectDevice();
    return 1;
  }
  bmp3_UnselectDevice();
  return (BMP3_INTF_RET_TYPE) retVal;
}

/*!
 * SPI write function map to Shimmer platform
 */
static BMP3_INTF_RET_TYPE
bmp3_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  HAL_StatusTypeDef retVal;

  bmp3_SelectDevice();
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  if (retVal)
  {
    bmp3_UnselectDevice();
    return 1;
  }
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, (uint8_t *) reg_data, (uint16_t) len, 1000);
  if (retVal)
  {
    bmp3_UnselectDevice();
    return 1;
  }
  bmp3_UnselectDevice();
  return (BMP3_INTF_RET_TYPE) retVal;
}

static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len)
{
  HAL_StatusTypeDef ret;
  bmp3_SelectDevice();
  ret = HAL_SPI_TransmitReceive_DMA(handle, txBufp, rxBufp, len);
  return ret;
}

/*!
 * Delay function map to COINES platform
 */
static void bmp3_delay_us(uint32_t period, void *intf_ptr)
{
  HAL_Delay(period / 1000);
}

void bmp3_check_rslt(const char api_name[], int8_t rslt)
{
  switch (rslt)
  {
  case BMP3_OK:
    /* Do nothing */
    break;
  case BMP3_E_NULL_PTR:
    SHIMMER_PRINTF("API [%s] Error [%d] : Null pointer\r\n", api_name, rslt);
    break;
  case BMP3_E_COMM_FAIL:
    SHIMMER_PRINTF("API [%s] Error [%d] : Communication failure\r\n", api_name, rslt);
    break;
  case BMP3_E_INVALID_LEN:
    SHIMMER_PRINTF("API [%s] Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
    break;
  case BMP3_E_DEV_NOT_FOUND:
    SHIMMER_PRINTF("API [%s] Error [%d] : Device not found\r\n", api_name, rslt);
    break;
  case BMP3_E_CONFIGURATION_ERR:
    SHIMMER_PRINTF("API [%s] Error [%d] : Configuration Error\r\n", api_name, rslt);
    break;
  case BMP3_W_SENSOR_NOT_ENABLED:
    SHIMMER_PRINTF("API [%s] Error [%d] : Warning when Sensor not enabled\r\n",
        api_name, rslt);
    break;
  case BMP3_W_INVALID_FIFO_REQ_FRAME_CNT:
    SHIMMER_PRINTF("API [%s] Error [%d] : Warning when Fifo watermark level is "
                   "not in limit\r\n",
        api_name, rslt);
    break;
  default:
    SHIMMER_PRINTF("API [%s] Error [%d] : Unknown error code\r\n", api_name, rslt);
    break;
  }
}

void bmp390_driver_init(void)
{
  bmp3.read = bmp3_spi_read;
  bmp3.write = bmp3_spi_write;
  bmp3.intf = BMP3_SPI_INTF;
  bmp3.delay_us = bmp3_delay_us;
  bmp3.intf_ptr = &dev_addr;
  bmp3.dummy_byte = 1;
}

void bmp3_SelectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void bmp3_UnselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

HAL_StatusTypeDef bmp3_pressure_temperature_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[] = { BMP3_REG_DATA | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(&SENSOR_BUS, &txBuff[0], buf, sizeof(txBuff));
  return ret;
}

int8_t bmp390_self_test(void)
{
  int8_t result;
  result = bmp3_selftest_check(&bmp3);

  if (result == BMP3_SENSOR_OK)
  {
    SHIMMER_PRINTF("BMP390 Self Test - PASS\r\n");
  }
  else
  {
    SHIMMER_PRINTF("BMP390 Self Test - FAIL\r\n");
    bmp3_check_rslt("BMP390", result);
  }

  return result;
}

void bmp390_restore_default_config(void)
{
  int8_t rslt;
  /* Reset the sensor */
  rslt = bmp3_soft_reset(&bmp3);
}

//TODO fill in config
void bmp390_config_set(uint8_t precision)
{
  int8_t rslt;

  /* Used to select the settings user needs to change */
  uint16_t settings_sel;

  struct bmp3_settings settings;

  /* Select the pressure and temperature sensor to be enabled */
  settings.press_en = BMP3_ENABLE;
  settings.temp_en = BMP3_ENABLE;

  /* Select the output data rate and over sampling settings for pressure and temperature */
  settings.odr_filter.press_os = BMP3_NO_OVERSAMPLING;
  settings.odr_filter.temp_os = BMP3_NO_OVERSAMPLING;
  settings.odr_filter.odr = BMP3_ODR_25_HZ;

  /* Assign the settings which needs to be set in the sensor */
  settings_sel = BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN | BMP3_SEL_PRESS_OS
      | BMP3_SEL_TEMP_OS | BMP3_SEL_ODR;
  rslt = bmp3_set_sensor_settings(settings_sel, &settings, &bmp3);
  if (rslt == BMP3_SENSOR_OK)
  {
    settings.op_mode = BMP3_MODE_NORMAL;
    rslt = bmp3_set_op_mode(&settings, &bmp3);
  }
}
