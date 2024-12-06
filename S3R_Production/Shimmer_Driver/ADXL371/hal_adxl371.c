/*
 * adxl371.c
 *
 *  Created on: Apr 17, 2024
 *      Author: MarkNolan
 */

#define SENSOR_BUS hspi1

#define CS_PORT    CS_HIGH_G_GPIO_Port
#define CS_PIN     CS_HIGH_G_Pin

#include "hal_adxl371.h"
#include "adxl371.h"

#include "gpio.h"
#include "spi.h"
#include "stm32u5xx.h"
#include <stdio.h>

struct adxl371_dev adxl371;

/* Private functions ---------------------------------------------------------*/

/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len);
static void tx_com(uint8_t *tx_buffer, uint16_t len);
static void platform_delay(uint32_t ms);

void adxl371_driver_init(void)
{
  struct adxl371_init_param init_param;

  //adxl371_init((struct adxl371_dev **)&adxl371, init_param);

  adxl371.comm_type = SPI;
  adxl371.reg_read = adxl371_spi_reg_read;
  adxl371.reg_write = adxl371_spi_reg_write;
  adxl371.reg_read_multiple = adxl371_spi_reg_read_multiple;
}

void adxl371_selectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void adxl371_unselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

self_test_result_t adxl371_self_test(void)
{
  /*Needs to contain >=200ms worth of data. @320Hz, 100 samples = 312ms */
  struct adxl371_xyz_accel_data accel_data;
  uint16_t self_test_data[100] = { 0 };
  uint8_t self_test_index = 0;
  uint8_t self_test_reg = 0;
  uint16_t first_set_avg = 0;
  uint16_t second_set_avg = 0;
  int32_t ret;
  uint8_t i;
  self_test_result_t self_test_result = SELF_TEST_PASS;

  if (!isAdxl371Detected())
  {
    self_test_result = SELF_TEST_FAIL_CHIP_DETECTION;
    return self_test_result;
  }

  /*1. Ensure that the low-pass activity filter is enabled. */
  /* Low-pass filter enabled by default. High-pass gets automatically disabled
   * for self-test operation */
  ret = adxl371_reset(&adxl371);

  /*2. Place the device in measurement mode. */
  ret = adxl371_set_odr(&adxl371, ADXL371_ODR_320HZ);
  ret = adxl371_set_bandwidth(&adxl371, ADXL371_BW_640HZ);
  ret = adxl371_set_op_mode(&adxl371, ADXL371_FULL_BW_MEASUREMENT);

  /*3. Wait until the filter settling time passes. */
  /*462.5ms according to datasheet if FILTER_SETTLE = 0 */
  platform_delay(500);

  /*4. Start the self test by setting the ST bit in the SELF_TEST register
   * (Register 0x40). */
  //platform_write(&adxl371, ADXL371_SELF_TEST, ADXL371_ST);
  ret = adxl371_write_mask(&adxl371, ADXL371_SELF_TEST,
      ADXL371_SELF_TEST_START_MSK, ADXL371_SELF_TEST_START(1));

  /*5. Read the acceleration data from the z-axis (Register 0x0C and Register
   * 0x0D) and store the data until the self test completes (ST_DONE goes high). */

  while (1)
  {
    ret = adxl371_get_accel_data(&adxl371, &accel_data);
    self_test_data[self_test_index++] = accel_data.z;

    ret = adxl371_spi_reg_read(&adxl371, ADXL371_SELF_TEST, &self_test_reg);
    if (self_test_reg & ADXL371_ST_DONE)
    {
      break;
    }
  }

  /*6. Average the first 50 ms of data right after ST is set. */
  //50ms = 16 samples at 320 Hz
  for (i = 0; i < 16; i++)
  {
    first_set_avg += self_test_data[i];
  }
  first_set_avg = first_set_avg / 16;

  /*7. Average the last 50 ms of data right before ST_DONE goes high. */
  //50ms = 16 samples at 320 Hz
  for (i = self_test_index - 1; i > self_test_index - 1 - 16; i--)
  {
    second_set_avg += self_test_data[i];
  }
  second_set_avg = second_set_avg / 16;

  /*8. If the absolute value of the difference between the two averaged values
   * is greater than 5 LSB, the self test passes. */
  uint16_t diff = abs((int16_t) second_set_avg - (int16_t) first_set_avg);
  self_test_result = diff > 5 ? SELF_TEST_PASS : SELF_TEST_FAIL_SIGNAL_ISSUE;

  //adxl371_reset(&adxl371);
  ret = adxl371_set_op_mode(&adxl371, ADXL371_STANDBY);

  return self_test_result;
}

void adxl371_configure(uint8_t rate)
{
  adxl371_set_op_mode(&adxl371, ADXL371_STANDBY);

  adxl371_reset(&adxl371);

  //adxl371_set_autosleep(&adxl371, false);
  //
  //adxl371_set_bandwidth(adxl371, ADXL371_BW_3200HZ);
  //
  //adxl371_set_odr(adxl371, ADXL371_ODR_6400HZ);
  //
  //adxl371_set_wakeup_rate(adxl371, ADXL371_WUR_52ms);
  //
  //adxl371_set_act_proc_mode(adxl371, ADXL371_LOOPED);
  //
  ///* Set Instant On threshold */
  //adxl371_set_instant_on_th(adxl371, ADXL371_INSTANT_ON_LOW_TH);
  //
  ///*Put fifo in Peak Detect and Stream Mode */
  //adxl371_configure_fifo(adxl371, ADXL371_FIFO_STREAMED, ADXL371_XYZ_PEAK_FIFO, 512);
  //
  ///* Set activity/inactivity threshold */
  //adxl371_set_activity_threshold(adxl371, ADXL371_ACTIVITY, ADXL371_ACT_VALUE, true, true);
  //adxl371_set_activity_threshold(adxl371, ADXL371_INACTIVITY, ADXL371_INACT_VALUE, true, true);
  //
  ///* Set activity/inactivity time settings */
  //adxl371_set_activity_time(adxl371, ADXL371_ACT_TIMER);
  //adxl371_set_inactivity_time(adxl371, ADXL371_INACT_TIMER);
  //
  ///* Set instant-on interrupts and activity interrupts */
  //adxl371_interrupt_config(adxl371);
  //
  ///* Set filter settle time */
  //adxl371_set_filter_settle(adxl371, ADXL371_FILTER_SETTLE_16);
  //
  ///* Set operation mode to Instant-On */
  //adxl371_set_op_mode(adxl371, ADXL371_INSTANT_ON);

  adxl371_set_odr(&adxl371, rate);
  adxl371_set_bandwidth(&adxl371, rate);

  /* Set filter settle time */
  adxl371_set_filter_settle(&adxl371, ADXL371_FILTER_SETTLE_4_ODR);

  adxl371_set_filter_lpf_mode(&adxl371, ADXL371_LPF_DISABLE);
  adxl371_set_filter_hpf_mode(&adxl371, ADXL371_HPF_DISABLE);

  /* Set operation mode to Full bandwidth measurement mode */
  adxl371_set_op_mode(&adxl371, ADXL371_FULL_BW_MEASUREMENT);
}

HAL_StatusTypeDef adxl371_accel_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  static uint8_t txBuff[] = { (ADXL371_X_DATA_H << 1) | 0x01, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(&SENSOR_BUS, &txBuff[0], buf, sizeof(txBuff));

  adxl371_spi_reg_read_multiple(&adxl371, ADXL371_DEVID, &buf[0], 4);

  return ret;
}

int32_t adxl371_is_data_rdy(void)
{
  uint8_t status1;
  int32_t ret;

  ret = adxl371_spi_reg_read(&adxl371, ADXL371_STATUS_1, &status1);
  if (ret < 0)
  {
    return ret;
  }

  return ADXL371_STATUS_1_DATA_RDY(status1);
}

void adxl371_reset_chip(void)
{
  int32_t ret = 0;
  ret |= adxl371_reset(&adxl371);
}

uint8_t isAdxl371Detected(void)
{
  uint8_t buf[4];
  adxl371_spi_reg_read_multiple(&adxl371, ADXL371_DEVID, &buf[0], 4);

  if (buf[0] == ADXL371_DEVID_VAL && buf[1] == ADXL371_MST_DEVID_VAL
      && buf[2] == ADXL371_PARTID_VAL && buf[3] == 0xFB)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int32_t adxl371_spi_reg_write(struct adxl371_dev *dev, uint8_t reg_addr, uint8_t reg_data)
{
  reg_addr = reg_addr << 1;
  adxl371_selectDevice();
  HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  HAL_SPI_Transmit(&SENSOR_BUS, &reg_data, 1, 1000);
  adxl371_unselectDevice();
  return 0;
}

int32_t adxl371_spi_reg_read(struct adxl371_dev *dev, uint8_t reg_addr, uint8_t *reg_data)
{
  adxl371_spi_reg_read_multiple(dev, reg_addr, reg_data, 1);
  return 0;
}

int32_t adxl371_spi_reg_read_multiple(struct adxl371_dev *dev,
    uint8_t reg_addr,
    uint8_t *reg_data,
    uint16_t count)
{
  reg_addr = (reg_addr << 1) | 0x01;
  adxl371_selectDevice();
  HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  HAL_SPI_Receive(&SENSOR_BUS, reg_data, count, 1000);
  adxl371_unselectDevice();
  return 0;
}

static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len)
{
  HAL_StatusTypeDef ret;
  adxl371_selectDevice();
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
