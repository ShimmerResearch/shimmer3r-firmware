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

//static dev_ctx_t dev_ctx;

struct adxl371_dev adxl371_dev;

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
//static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
static int32_t platform_write(struct adxl371_dev *dev, uint8_t reg, const uint8_t bufp)
{
  adxl371_SelectDevice();
  HAL_SPI_Transmit(&SENSOR_BUS, &reg, 1, 1000);
  HAL_SPI_Transmit(&SENSOR_BUS, &bufp, 1, 1000);
  adxl371_UnselectDevice();
  return 0;
}

static int32_t platform_read_multiple(struct adxl371_dev *dev, uint8_t reg, uint8_t *bufp, uint16_t len)
{
  reg = (reg << 1) | 0x01;
  adxl371_SelectDevice();
  HAL_SPI_Transmit(&SENSOR_BUS, &reg, 1, 1000);
  HAL_SPI_Receive(&SENSOR_BUS, bufp, len, 1000);
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
static int32_t platform_read(struct adxl371_dev *dev, uint8_t reg, uint8_t *bufp)
{
  return platform_read_multiple(dev, reg, bufp, 1);
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

void adxl371_driver_init(void)
{
//  /* Initialize mems driver interface */
//  dev_ctx.write_reg = platform_write;
//  dev_ctx.read_reg = platform_read;
//  dev_ctx.mdelay = platform_delay;
//  dev_ctx.handle = &SENSOR_BUS;

  adxl371_dev.reg_read = platform_read;
  adxl371_dev.reg_read_multiple = platform_read_multiple;
  adxl371_dev.reg_write = platform_write;
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
  uint8_t result = 0;
  /*Needs to contain >=200ms worth of data. @320Hz, 100 samples = 312ms */
  uint16_t self_test_data[100] = {0};
  struct adxl371_xyz_accel_data accel_data;
  uint8_t self_test_index = 0;
  uint8_t self_test_reg = 0;

  if (!isAdxl371Detected())
  {
    return 1;
  }

  //TODO implement an ADXL371 signal test feature

  adxl371_init(&adxl371_dev);

  adxl371_reset(&adxl371_dev);

  /*1. Ensure that the low-pass activity filter is enabled. */
  /* Low-pass filter enabled by default.*/

  /*2. Place the device in measurement mode. */
  adxl371_set_bandwidth(&adxl371_dev, ADXL371_BW_160HZ);
  adxl371_set_odr(&adxl371_dev, ADXL371_ODR_320HZ);
  adxl371_set_op_mode(&adxl371_dev, ADXL371_FULL_BW_MEASUREMENT);

  /*3. Wait until the filter settling time passes. */
  /*462.5ms according to datasheet if FILTER_SETTLE = 0 although datasheet also
   * states that first sample doesn't appear until after the settling anyway. */
  platform_delay(500);

  /*4. Start the self test by setting the ST bit in the SELF_TEST register
   * (Register 0x40). */
  platform_write(&adxl371_dev, ADXL371_SELF_TEST, ADXL371_ST);

  /*5. Read the acceleration data from the z-axis (Register 0x0C and Register
   * 0x0D) and store the data until the self test completes (ST_DONE goes high). */

  while(1)
  {
    adxl371_get_accel_data(&adxl371_dev, &accel_data);
    self_test_data[self_test_index++] = accel_data.z;

    platform_read(&adxl371_dev, ADXL371_SELF_TEST, &self_test_reg);
    if (self_test_reg & ADXL371_ST_DONE)
    {
      break;
    }
  }

  /*6. Average the first 50 ms of data right after ST is set. */

  /*7. Average the last 50 ms of data right before ST_DONE goes high. */

  /*8. If the absolute value of the difference between the two averaged values
   * is greater than 5 LSB, the self test passes. */

  adxl371_reset(&adxl371_dev);

  return result;
}

void adxl371_restore_default_config(void)
{
  adxl371_reset(&adxl371_dev);
}

uint8_t isAdxl371Detected(void)
{
  uint8_t buf[4];
  platform_read_multiple(&adxl371_dev, ADXL371_DEVID, &buf[0], 4);

  if (buf[0] == 0xAD && buf[1] == 0x1D && buf[2] == 0xFA && buf[3] == 0xFB)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
