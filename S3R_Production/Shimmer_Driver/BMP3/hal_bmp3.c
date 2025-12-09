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

#include "hal_FactoryTest.h"

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
static void platform_delay(uint32_t ms);

void bmp3_driver_init(void)
{
  bmp3.read = bmp3_spi_read;
  bmp3.write = bmp3_spi_write;
  bmp3.intf = BMP3_SPI_INTF;
  bmp3.delay_us = bmp3_delay_us;
  bmp3.intf_ptr = &dev_addr;
  bmp3.dummy_byte = 1; /* SPI interface, read extra byte */

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

uint8_t bmp3_self_test(void)
{
  uint8_t self_test_result = SELF_TEST_PASS;
  int8_t bmp3_result;
  bmp3_result = bmp3_selftest_check(&bmp3);
  if (bmp3_result == BMP3_COMMUNICATION_ERROR_OR_WRONG_DEVICE)
  {
    self_test_result = SELF_TEST_FAIL_CHIP_DETECTION;
  }
  else if (bmp3_result == BMP3_SENSOR_OK)
  {
    if (!bmp3_drdy_test())
    {
      self_test_result = SELF_TEST_FAIL_DRDY_ISSUE;
    }
  }
  else
  {
    /* Adding offset to separate our BMP3 API errors/warnings from Shimmer
     * self-test errors */
    self_test_result = bmp3_result + BMP390_API_ERROR_OFFSET;
  }
  return self_test_result;
}

int8_t bmp3_drdy_test(void)
{
  int8_t rslt;
  int8_t res = 0;
  uint8_t i = 0;
  struct bmp3_status status = { { 0 } };
  struct bmp3_data data = { 0 };

  /* Used to select the settings user needs to change */
  uint16_t settings_sel;
  struct bmp3_settings settings = { 0 };

  /* Reset the sensor */
  rslt = bmp3_soft_reset(&bmp3);
  if (rslt == BMP3_SENSOR_OK)
  {
    rslt = bmp3_init(&bmp3);
    if (rslt == BMP3_SENSOR_OK)
    {
      /* Select the pressure and temperature sensor to be enabled */
      settings.press_en = BMP3_ENABLE;
      settings.temp_en = BMP3_ENABLE;

      /* Select the output data rate and over sampling settings for pressure and temperature */
      settings.odr_filter.press_os = BMP3_NO_OVERSAMPLING;
      settings.odr_filter.temp_os = BMP3_NO_OVERSAMPLING;
      settings.odr_filter.odr = BMP3_ODR_50_HZ;

      /* enable interrupts and latch */
      settings.int_settings.drdy_en = 1;
      settings.int_settings.latch = 1;

      /* Assign the settings which needs to be set in the sensor */
      settings_sel = BMP3_SEL_DRDY_EN | BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN
          | BMP3_SEL_LATCH | BMP3_SEL_PRESS_OS | BMP3_SEL_TEMP_OS | BMP3_SEL_ODR;
      rslt = bmp3_set_sensor_settings(settings_sel, &settings, &bmp3);

      if (rslt == BMP3_SENSOR_OK)
      {
        settings.op_mode = BMP3_MODE_NORMAL;
        rslt = bmp3_set_op_mode(&settings, &bmp3);
      }

      /* Added in case chip needs time to enable interrupt pin */
      platform_delay(100);

      if (rslt == BMP3_SENSOR_OK)
      {
        /* New sample is every 20ms @ 50Hz. Loop count + delay below allows 100ms for DRDY to toggle */
        for (i = 0; i < 50; i++)
        {
          if (BMP390_INT)
          {
            /* read the sensor data */
            bmp3_get_sensor_data(BMP3_PRESS_TEMP, &data, &bmp3);

            /* NOTE : Read status register again to clear data ready interrupt status */
            rslt = bmp3_get_status(&status, &bmp3);
            platform_delay(1);
            //check for pin status, 0 = fail, 1 = pass
            res = ((rslt == BMP3_SENSOR_OK) && (!BMP390_INT)) ? 1 : 0;
            break;
          }
          /* Wait for 1 ms */
          platform_delay(1);
        }
      }
      if (rslt == BMP3_SENSOR_OK)
      {
        settings.op_mode = BMP3_MODE_SLEEP;
        rslt = bmp3_set_op_mode(&settings, &bmp3);
      }
    }
  }
  return res;
}

int8_t bmp3_configure(float shimmerSamplingFreq, uint8_t overSamplingRatio)
{
  int8_t rslt;
  uint8_t rate = BMP3_ODR_200_HZ;
  uint8_t next_odr = BMP3_ODR_200_HZ;

  /* Used to select the settings user needs to change */
  uint16_t settings_sel;

  struct bmp3_settings settings;

  /* Select the pressure and temperature sensor to be enabled */
  settings.press_en = BMP3_ENABLE;
  settings.temp_en = BMP3_ENABLE;
  /* Select the output data rate and over sampling settings for pressure and temperature */
  settings.odr_filter.press_os = overSamplingRatio;

  if ((settings.odr_filter.press_os == BMP3_OVERSAMPLING_16X)
      || (settings.odr_filter.press_os == BMP3_OVERSAMPLING_32X))
  {
    settings.odr_filter.temp_os = BMP3_OVERSAMPLING_2X;
  }
  else
  {
    settings.odr_filter.temp_os = BMP3_NO_OVERSAMPLING;
  }

  if (shimmerSamplingFreq >= (float) 100.0)
  {
    rate = BMP3_ODR_200_HZ;
  }
  else if ((shimmerSamplingFreq >= (float) 50.0))
  {
    rate = BMP3_ODR_100_HZ;
  }
  else if ((shimmerSamplingFreq >= (float) 25.0))
  {
    rate = BMP3_ODR_50_HZ;
  }
  else if ((shimmerSamplingFreq >= (float) 12.5))
  {
    rate = BMP3_ODR_25_HZ;
  }
  else if ((shimmerSamplingFreq >= (float) 6.25))
  {
    rate = BMP3_ODR_12_5_HZ;
  }
  else if ((shimmerSamplingFreq >= (float) 3.1))
  {
    rate = BMP3_ODR_6_25_HZ;
  }
  else if ((shimmerSamplingFreq >= (float) 1.5))
  {
    rate = BMP3_ODR_3_1_HZ;
  }
  else
  {
    rate = BMP3_ODR_1_5_HZ;
  }

  rslt = find_next_supported_odr(rate, &next_odr, &settings);

  settings.odr_filter.odr = next_odr;

  /* Assign the settings which needs to be set in the sensor */
  settings_sel = BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN | BMP3_SEL_PRESS_OS
      | BMP3_SEL_TEMP_OS | BMP3_SEL_ODR;

  isDrdyIntEnabled = false;
  if (bmp3_is_shimmer_freq_higher(shimmerSamplingFreq, rate))
  {
    isDrdyIntEnabled = true;
    settings_sel |= BMP3_SEL_DRDY_EN | BMP3_SEL_LATCH;

    settings.int_settings.drdy_en = BMP3_ENABLE;
    settings.int_settings.latch = BMP3_ENABLE;
  }

  rslt = bmp3_set_sensor_settings(settings_sel, &settings, &bmp3);
  if (rslt != BMP3_SENSOR_OK)
  {
    return rslt;
  }

  settings.op_mode = BMP3_MODE_NORMAL;
  rslt = bmp3_set_op_mode(&settings, &bmp3);

  return rslt;
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

  case BMP3_TRIMMING_DATA_OUT_OF_BOUND:
    ShimFactoryTest_sendReport("Trimming data out of bound\r\n");
    break;
  case BMP3_TEMPERATURE_BOUND_WIRE_FAILURE_OR_MEMS_DEFECT:
    ShimFactoryTest_sendReport(
        "Temperature bound wire failure or MEMs defect\r\n");
    break;
  case BMP3_PRESSURE_BOUND_WIRE_FAILURE_OR_MEMS_DEFECT:
    ShimFactoryTest_sendReport(
        "Pressure bound wire failure or MEMs defect\r\n");
    break;
  case BMP3_IMPLAUSIBLE_TEMPERATURE:
    ShimFactoryTest_sendReport("Implausible Temperature\r\n");
    break;
  case BMP3_IMPLAUSIBLE_PRESSURE:
    ShimFactoryTest_sendReport("Implausible Pressure\r\n");
    break;

  default:
    sprintf(outputStr, "API [%s] Error [%d] : Unknown error code\r\n", api_name, rslt);
    break;
  }
}

int8_t bmp3_read_sensor_status(void)
{
  struct bmp3_status status = { { 0 } };
  int8_t rslt = bmp3_get_status(&status, &bmp3);
  return rslt;
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
  if (ret != HAL_OK)
  {
    bmp3_unselectDevice();
  }
  return ret;
}

static void platform_delay(uint32_t ms)
{
#if defined(NUCLEO_F411RE) | defined(STEVAL_MKI109V3) | defined(SHIMMER3R)
  HAL_Delay(ms);
#elif defined(SPC584B_DIS)
  osalThreadDelayMilliseconds(ms);
#endif
}
