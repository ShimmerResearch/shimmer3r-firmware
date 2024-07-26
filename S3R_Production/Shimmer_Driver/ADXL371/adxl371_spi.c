/*
 * adxl371.c
 *
 *  Created on: Apr 17, 2024
 *      Author: MarkNolan
 */

#define SENSOR_BUS hspi1

#define CS_PORT    CS_HIGH_G_GPIO_Port
#define CS_PIN     CS_HIGH_G_Pin

#include "adxl371.h"

#include "gpio.h"
#include "spi.h"
#include "stm32u5xx.h"
#include <stdio.h>

static dev_ctx_t dev_ctx;

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

void adxl371_driver_init(void)
{
  /* Initialize mems driver interface */
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.mdelay = platform_delay;
  dev_ctx.handle = &SENSOR_BUS;
}

void adxl371_power_on(void)
{
  set_power_spi1_bus(true, SPI1_CHIP_INDEX_ADXL371);
}

void adxl371_power_off(void)
{
  set_power_spi1_bus(false, SPI1_CHIP_INDEX_ADXL371);
}

void adxl371_SelectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void adxl371_UnselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

uint8_t adxl371_self_test(void)
{
  uint8_t buf[4];
  platform_read(dev_ctx.handle, 0x00, &buf[0], 4);

  if (buf[0] == 0xAD && buf[1] == 0x1D && buf[2] == 0xFA && buf[3] == 0xFB)
  {
    SHIMMER_PRINTF("ADXL371 Self Test - PASS\r\n");
    return 0;
  }
  else
  {
    SHIMMER_PRINTF("ADXL371 Self Test - FAIL\r\n");
    return 1;
  }
}

void adxl371_restore_default_config(void)
{
  //TODO
  //adxl371_reset(dev);
}

void adxl371_config_accel(uint8_t rate, uint8_t range)
{
  //TODO
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
  adxl371_SelectDevice();
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Transmit(handle, (uint8_t *) bufp, len, 1000);
  adxl371_UnselectDevice();
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
  reg = (reg << 1) | 0x01;
  adxl371_SelectDevice();
  HAL_SPI_Transmit(handle, &reg, 1, 1000);
  HAL_SPI_Receive(handle, bufp, len, 1000);
  adxl371_UnselectDevice();
  return 0;
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
  SHIMMER_PRINTF((char *) tx_buffer, len);
}

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 *
 */
static void platform_delay(uint32_t ms)
{
  HAL_Delay(ms);
}
