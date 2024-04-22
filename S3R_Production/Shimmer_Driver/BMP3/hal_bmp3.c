/*
 * hal_bmp3.c
 *
 *  Created on: Apr 17, 2024
 *      Author: MarkNolan
 */

#include "hal_bmp3.h"

#include <string.h>
#include "stm32u5xx.h"
#include "gpio.h"
#include "spi.h"

#include "BMP3_SensorAPI/bmp3.h"
#include "BMP3_SensorAPI/self-test/bmp3_selftest.h"

#define SENSOR_BUS hspi1

#define CS_PORT CS_BMP390_GPIO_Port
#define CS_PIN CS_BMP390_Pin

struct bmp3_dev bmp3;

/* Variable to store the device address */
static uint8_t dev_addr;

void bmp3_SelectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void bmp3_UnselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

/*!
 * SPI read function map to Shimmer platform
 */
static BMP3_INTF_RET_TYPE bmp3_spi_read(uint8_t reg_addr, uint8_t *reg_data,
    uint32_t len, void *intf_ptr)
{
  HAL_StatusTypeDef retVal;

  bmp3_SelectDevice();
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  if (retVal)
  {
    bmp3_UnselectDevice();
    return 1;
  }
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, (uint8_t*) reg_data, (uint16_t) len,
      1000);
  if (retVal)
  {
    bmp3_UnselectDevice();
    return 1;
  }
  bmp3_UnselectDevice();
  return ( BMP3_INTF_RET_TYPE ) retVal;
}

/*!
 * SPI write function map to Shimmer platform
 */
static BMP3_INTF_RET_TYPE bmp3_spi_write(uint8_t reg_addr,
    const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  HAL_StatusTypeDef retVal;

  bmp3_SelectDevice();
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  if (retVal)
  {
    bmp3_UnselectDevice();
    return 1;
  }
  retVal = HAL_SPI_Receive(&SENSOR_BUS, (uint8_t*) reg_data, (uint16_t) len,
      1000);
  if (retVal)
  {
    bmp3_UnselectDevice();
    return 1;
  }
  bmp3_UnselectDevice();
  return ( BMP3_INTF_RET_TYPE ) retVal;
}

/*!
 * Delay function map to COINES platform
 */
static void bmp3_delay_us(uint32_t period, void *intf_ptr)
{
  HAL_Delay(period / 1000);
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

int8_t bmp390_self_test(void)
{
  int8_t result;
  result = bmp3_selftest_check(&bmp3);

  if (result == BMP3_SENSOR_OK)
  {
    printf("Self Test - PASS\r\n");
  }
  else
  {
    printf("Self Test - FAIL\r\n");
  }

  return result;
}
