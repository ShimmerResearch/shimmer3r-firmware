/*
 * hal_bmp5.c
 *
 *  Created on: Jul 7, 2026
 *      Author: SuhasVarna
 */

#include "hal_bmp5.h"

#include "gpio.h"
#include "spi.h"
#include "stm32u5xx.h"
#include <string.h>

#include "BMP5_SensorAPI/bmp5.h"
#include "BMP5_SensorAPI/bmp5_selftest.h"

#include "hal_Board.h"
#include "hal_FactoryTest.h"

#define SENSOR_BUS hspi1

/* The BMP581 is a drop-in replacement for the BMP390 and so shares the same
 * chip-select and interrupt lines. */
#define CS_PORT    CS_BMP390_GPIO_Port
#define CS_PIN     CS_BMP390_Pin

struct bmp5_dev bmp5;

static struct bmp5_osr_odr_press_config bmp5OsrOdrPressCfg;

static bool bmp5DrdyIntEnabled = false;

/* Variable to store the device address */
static uint8_t bmp5_dev_addr;

/* Last sample read during the self-test's DRDY check. Used by the factory
 * test to sanity check the temperature. */
static struct bmp5_sensor_data bmp5SelftestData;

/* BMP581 output data rates in normal mode, ordered fastest to slowest
 * (register codes 0x00-0x1F). */
static const struct
{
  uint8_t odr;
  float freq;
} bmp5OdrTable[] = {
  { BMP5_ODR_240_HZ, 240.0f },
  { BMP5_ODR_218_5_HZ, 218.5f },
  { BMP5_ODR_199_1_HZ, 199.1f },
  { BMP5_ODR_179_2_HZ, 179.2f },
  { BMP5_ODR_160_HZ, 160.0f },
  { BMP5_ODR_149_3_HZ, 149.3f },
  { BMP5_ODR_140_HZ, 140.0f },
  { BMP5_ODR_129_8_HZ, 129.8f },
  { BMP5_ODR_120_HZ, 120.0f },
  { BMP5_ODR_110_1_HZ, 110.1f },
  { BMP5_ODR_100_2_HZ, 100.2f },
  { BMP5_ODR_89_6_HZ, 89.6f },
  { BMP5_ODR_80_HZ, 80.0f },
  { BMP5_ODR_70_HZ, 70.0f },
  { BMP5_ODR_60_HZ, 60.0f },
  { BMP5_ODR_50_HZ, 50.0f },
  { BMP5_ODR_45_HZ, 45.0f },
  { BMP5_ODR_40_HZ, 40.0f },
  { BMP5_ODR_35_HZ, 35.0f },
  { BMP5_ODR_30_HZ, 30.0f },
  { BMP5_ODR_25_HZ, 25.0f },
  { BMP5_ODR_20_HZ, 20.0f },
  { BMP5_ODR_15_HZ, 15.0f },
  { BMP5_ODR_10_HZ, 10.0f },
  { BMP5_ODR_05_HZ, 5.0f },
  { BMP5_ODR_04_HZ, 4.0f },
  { BMP5_ODR_03_HZ, 3.0f },
  { BMP5_ODR_02_HZ, 2.0f },
  { BMP5_ODR_01_HZ, 1.0f },
  { BMP5_ODR_0_5_HZ, 0.5f },
  { BMP5_ODR_0_250_HZ, 0.25f },
  { BMP5_ODR_0_125_HZ, 0.125f },
};
#define BMP5_ODR_TABLE_LEN (sizeof(bmp5OdrTable) / sizeof(bmp5OdrTable[0]))

static BMP5_INTF_RET_TYPE
bmp5_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
static BMP5_INTF_RET_TYPE
bmp5_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
static void bmp5_delay_us(uint32_t periodUS, void *intf_ptr);
static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len);
static void platform_delay(uint32_t ms);
static uint8_t bmp5_get_paired_temp_osr(uint8_t pressOversamplingRatio);

void bmp5_setup_dev(void)
{
  bmp5.read = bmp5_spi_read;
  bmp5.write = bmp5_spi_write;
  bmp5.intf = BMP5_SPI_INTF;
  bmp5.delay_us = bmp5_delay_us;
  bmp5.intf_ptr = &bmp5_dev_addr;
  /* Unlike the BMP390, the BMP581 does not insert a dummy byte after the
   * register address during SPI reads so no dummy byte handling is needed. */
}

void bmp5_driver_init(void)
{
  bmp5_setup_dev();

  /* Reads and verifies the chip ID and leaves the sensor in standby. The
   * BMP581 outputs pre-compensated pressure and temperature so, unlike the
   * BMP390, there are no calibration coefficients to read here. */
  bmp5_soft_reset(&bmp5);
  bmp5_init(&bmp5);
}

int8_t bmp5_verify_chip_id(void)
{
  uint8_t chip_id = 0;
  int8_t rslt = bmp5_get_regs(BMP5_REG_CHIP_ID, &chip_id, 1, &bmp5);
  if (rslt == BMP5_OK
      && (chip_id == BMP5_CHIP_ID_PRIM || chip_id == BMP5_CHIP_ID_SEC))
  {
    return BMP5_OK;
  }
  return BMP5_E_INVALID_CHIP_ID;
}

void bmp5_selectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void bmp5_unselectDevice(void)
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

uint8_t bmp5_self_test(void)
{
  uint8_t self_test_result = SELF_TEST_PASS;
  int8_t bmp5_result;
  bmp5_result = bmp5_selftest_check(&bmp5);
  if (bmp5_result == BMP5_COMMUNICATION_ERROR_OR_WRONG_DEVICE
      || bmp5_result == BMP5_E_DEV_NOT_FOUND || bmp5_result == BMP5_E_INVALID_CHIP_ID)
  {
    self_test_result = SELF_TEST_FAIL_CHIP_DETECTION;
  }
  else if (bmp5_result == BMP5_OK)
  {
    if (!bmp5_drdy_test())
    {
      self_test_result = SELF_TEST_FAIL_DRDY_ISSUE;
    }
  }
  else
  {
    /* Adding offset to separate our BMP5 API errors/warnings from Shimmer
     * self-test errors */
    self_test_result = bmp5_result + BMP581_API_ERROR_OFFSET;
  }
  return self_test_result;
}

int8_t bmp5_drdy_test(void)
{
  int8_t rslt;
  int8_t res = 0;
  uint8_t i = 0;
  uint8_t int_status = 0;
  struct bmp5_osr_odr_press_config osr_odr_press_cfg = { 0 };
  struct bmp5_int_source_select int_source_select = { 0 };

  /* Reset the sensor */
  rslt = bmp5_soft_reset(&bmp5);
  if (rslt == BMP5_OK)
  {
    rslt = bmp5_init(&bmp5);
    if (rslt == BMP5_OK)
    {
      rslt = bmp5_get_osr_odr_press_config(&osr_odr_press_cfg, &bmp5);

      if (rslt == BMP5_OK)
      {
        /* Enable pressure and select the output data rate and over sampling
         * settings for pressure and temperature */
        osr_odr_press_cfg.press_en = BMP5_ENABLE;
        osr_odr_press_cfg.osr_p = BMP5_OVERSAMPLING_1X;
        osr_odr_press_cfg.osr_t = BMP5_OVERSAMPLING_1X;
        osr_odr_press_cfg.odr = BMP5_ODR_50_HZ;
        rslt = bmp5_set_osr_odr_press_config(&osr_odr_press_cfg, &bmp5);
      }

      if (rslt == BMP5_OK)
      {
        /* Enable data-ready interrupt and latch */
        int_source_select.drdy_en = BMP5_ENABLE;
        rslt = bmp5_int_source_select(&int_source_select, &bmp5);
      }
      if (rslt == BMP5_OK)
      {
        rslt = bmp5_configure_interrupt(BMP5_LATCHED, BMP5_ACTIVE_HIGH,
            BMP5_INTR_PUSH_PULL, BMP5_INTR_ENABLE, &bmp5);
      }

      if (rslt == BMP5_OK)
      {
        rslt = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, &bmp5);
      }

      /* Added in case chip needs time to enable interrupt pin */
      platform_delay(100);

      if (rslt == BMP5_OK)
      {
        /* New sample is every 20ms @ 50Hz. Loop count + delay below allows 100ms for DRDY to toggle */
        for (i = 0; i < 50; i++)
        {
          if (BMP581_INT)
          {
            /* read the sensor data */
            bmp5_get_sensor_data(&bmp5SelftestData, &osr_odr_press_cfg, &bmp5);

            /* NOTE : Read interrupt status register again to clear data ready
             * interrupt status */
            rslt = bmp5_get_interrupt_status(&int_status, &bmp5);
            platform_delay(1);
            //check for pin status, 0 = fail, 1 = pass
            res = ((rslt == BMP5_OK) && (!BMP581_INT)) ? 1 : 0;
            break;
          }
          /* Wait for 1 ms */
          platform_delay(1);
        }
      }
      if (rslt == BMP5_OK)
      {
        rslt = bmp5_set_power_mode(BMP5_POWERMODE_STANDBY, &bmp5);
      }
    }
  }
  return res;
}

int8_t bmp5_configure(float shimmerSamplingFreq, uint8_t overSamplingRatio)
{
  int8_t rslt;
  uint8_t odrIdx = 0;
  uint8_t i;
  struct bmp5_osr_odr_eff osr_odr_eff = { 0 };
  struct bmp5_int_source_select int_source_select = { 0 };

  /* Configuration registers can only be updated while in standby mode */
  rslt = bmp5_set_power_mode(BMP5_POWERMODE_STANDBY, &bmp5);
  if (rslt != BMP5_OK)
  {
    return rslt;
  }

  bmp5OsrOdrPressCfg.press_en = BMP5_ENABLE;
  bmp5OsrOdrPressCfg.osr_p = (overSamplingRatio <= BMP5_OVERSAMPLING_128X) ?
      overSamplingRatio :
      BMP5_OVERSAMPLING_1X;
  bmp5OsrOdrPressCfg.osr_t = bmp5_get_paired_temp_osr(bmp5OsrOdrPressCfg.osr_p);

  /* Select the slowest ODR that is still higher than the Shimmer's sampling
   * frequency (mirroring the approach taken for the BMP390) */
  for (i = 0; i < BMP5_ODR_TABLE_LEN; i++)
  {
    if (bmp5OdrTable[i].freq > shimmerSamplingFreq)
    {
      odrIdx = i;
    }
    else
    {
      break;
    }
  }

  /* Not all OSR/ODR combinations are achievable (BMP581 datasheet Table 9).
   * The sensor reports the feasibility of the configuration via
   * OSR_EFF.odr_is_valid. Step the ODR down until the requested oversampling
   * ratio can be met, thereby preserving the user's OSR selection. */
  while (1)
  {
    bmp5OsrOdrPressCfg.odr = bmp5OdrTable[odrIdx].odr;
    rslt = bmp5_set_osr_odr_press_config(&bmp5OsrOdrPressCfg, &bmp5);
    if (rslt != BMP5_OK)
    {
      return rslt;
    }
    rslt = bmp5_get_osr_odr_eff(&osr_odr_eff, &bmp5);
    if (rslt != BMP5_OK)
    {
      return rslt;
    }
    if (osr_odr_eff.odr_is_valid || (odrIdx >= (BMP5_ODR_TABLE_LEN - 1)))
    {
      break;
    }
    odrIdx++;
  }

  bmp5DrdyIntEnabled = false;
  if (bmp5_is_shimmer_freq_higher(shimmerSamplingFreq, bmp5OsrOdrPressCfg.odr))
  {
    bmp5DrdyIntEnabled = true;

    int_source_select.drdy_en = BMP5_ENABLE;
    rslt = bmp5_int_source_select(&int_source_select, &bmp5);
    if (rslt != BMP5_OK)
    {
      return rslt;
    }
    rslt = bmp5_configure_interrupt(BMP5_LATCHED, BMP5_ACTIVE_HIGH,
        BMP5_INTR_PUSH_PULL, BMP5_INTR_ENABLE, &bmp5);
  }
  else
  {
    rslt = bmp5_configure_interrupt(BMP5_LATCHED, BMP5_ACTIVE_HIGH,
        BMP5_INTR_PUSH_PULL, BMP5_INTR_DISABLE, &bmp5);
  }
  if (rslt != BMP5_OK)
  {
    return rslt;
  }

  rslt = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, &bmp5);

  return rslt;
}

HAL_StatusTypeDef bmp5_pressure_temperature_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  /* The BMP581 bursts temperature (0x1D-0x1F) followed by pressure
   * (0x20-0x22). No dummy byte is inserted after the register address. */
  static uint8_t txBuff[] = { BMP5_REG_TEMP_DATA_XLSB | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0 };
  ret = platform_read_raw_data_dma(&SENSOR_BUS, &txBuff[0], buf, sizeof(txBuff));
  return ret;
}

bool bmp5_is_drdy_int_enabled(void)
{
  return bmp5DrdyIntEnabled;
}

bool bmp5_is_shimmer_freq_higher(float shimmerSamplingFreq, uint8_t rate)
{
  return shimmerSamplingFreq > bmp5_get_sensor_freq_from_rate(rate);
}

float bmp5_get_sensor_freq_from_rate(uint8_t rate)
{
  uint8_t i;
  for (i = 0; i < BMP5_ODR_TABLE_LEN; i++)
  {
    if (bmp5OdrTable[i].odr == rate)
    {
      return bmp5OdrTable[i].freq;
    }
  }
  return 0.0f;
}

int8_t bmp5_restore_default_config(void)
{
  int8_t rslt;
  /* Reset the sensor */
  rslt = bmp5_soft_reset(&bmp5);

  return rslt;
}

int8_t bmp5_read_int_status(void)
{
  uint8_t int_status = 0;
  /* Reading the interrupt status register clears the latched interrupt pin */
  int8_t rslt = bmp5_get_interrupt_status(&int_status, &bmp5);
  return rslt;
}

struct bmp5_sensor_data *get_bmp5_selftest_data(void)
{
  return &bmp5SelftestData;
}

void bmp5_check_rslt(const char api_name[], int8_t rslt, char *outputStr)
{
  switch (rslt)
  {
  case BMP5_OK:
    /* Do nothing */
    break;
  case BMP5_E_NULL_PTR:
    sprintf(outputStr, "API [%s] Error [%d] : Null pointer\r\n", api_name, rslt);
    break;
  case BMP5_E_COM_FAIL:
    sprintf(outputStr, "API [%s] Error [%d] : Communication failure\r\n", api_name, rslt);
    break;
  case BMP5_E_DEV_NOT_FOUND:
    sprintf(outputStr, "API [%s] Error [%d] : Device not found\r\n", api_name, rslt);
    break;
  case BMP5_E_INVALID_CHIP_ID:
    sprintf(outputStr, "API [%s] Error [%d] : Invalid chip ID\r\n", api_name, rslt);
    break;
  case BMP5_E_NVM_NOT_READY:
    sprintf(outputStr, "API [%s] Error [%d] : NVM not ready\r\n", api_name, rslt);
    break;
  case BMP5_E_POR_SOFTRESET:
    sprintf(outputStr, "API [%s] Error [%d] : Power-on reset/softreset failure\r\n",
        api_name, rslt);
    break;
  case BMP5_E_INVALID_POWERMODE:
    sprintf(outputStr, "API [%s] Error [%d] : Invalid powermode\r\n", api_name, rslt);
    break;

  case BMP5_TRIMMING_DATA_OUT_OF_BOUND:
    sprintf(outputStr, "Trimming data out of bound\r\n");
    break;
  case BMP5_TEMPERATURE_BOUND_WIRE_FAILURE_OR_MEMS_DEFECT:
    sprintf(outputStr, "Temperature bound wire failure or MEMs defect\r\n");
    break;
  case BMP5_PRESSURE_BOUND_WIRE_FAILURE_OR_MEMS_DEFECT:
    sprintf(outputStr, "Pressure bound wire failure or MEMs defect\r\n");
    break;
  case BMP5_IMPLAUSIBLE_TEMPERATURE:
    sprintf(outputStr, "Implausible Temperature\r\n");
    break;
  case BMP5_IMPLAUSIBLE_PRESSURE:
    sprintf(outputStr, "Implausible Pressure\r\n");
    break;
  case BMP5_E_SELFTEST_TIMEOUT:
    sprintf(outputStr, "Self-test timeout\r\n");
    break;

  default:
    sprintf(outputStr, "API [%s] Error [%d] : Unknown error code\r\n", api_name, rslt);
    break;
  }
}

/* Recommended pairing of temperature oversampling with pressure oversampling
 * as per BMP581 datasheet Table 9 */
static uint8_t bmp5_get_paired_temp_osr(uint8_t pressOversamplingRatio)
{
  switch (pressOversamplingRatio)
  {
  case BMP5_OVERSAMPLING_128X:
    return BMP5_OVERSAMPLING_8X;
  case BMP5_OVERSAMPLING_64X:
    return BMP5_OVERSAMPLING_4X;
  case BMP5_OVERSAMPLING_32X:
    return BMP5_OVERSAMPLING_2X;
  default:
    return BMP5_OVERSAMPLING_1X;
  }
}

/*!
 * SPI write function map to Shimmer platform
 */
static BMP5_INTF_RET_TYPE
bmp5_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  HAL_StatusTypeDef retVal;

  bmp5_selectDevice();
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  if (retVal)
  {
    bmp5_unselectDevice();
    return 1;
  }
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, (uint8_t *) reg_data, (uint16_t) len, 1000);
  if (retVal)
  {
    bmp5_unselectDevice();
    return 1;
  }
  bmp5_unselectDevice();
  return (BMP5_INTF_RET_TYPE) retVal;
}

/*!
 * SPI read function map to Shimmer platform
 */
static BMP5_INTF_RET_TYPE
bmp5_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  HAL_StatusTypeDef retVal;

  bmp5_selectDevice();
  retVal = HAL_SPI_Transmit(&SENSOR_BUS, &reg_addr, 1, 1000);
  if (retVal)
  {
    bmp5_unselectDevice();
    return 1;
  }
  retVal = HAL_SPI_Receive(&SENSOR_BUS, (uint8_t *) reg_data, (uint16_t) len, 1000);
  if (retVal)
  {
    bmp5_unselectDevice();
    return 1;
  }
  bmp5_unselectDevice();
  return (BMP5_INTF_RET_TYPE) retVal;
}

/*!
 * Delay function map to Shimmer platform
 */
static void bmp5_delay_us(uint32_t periodUS, void *intf_ptr)
{
  /* Round up so that sub-millisecond delays (e.g. NVM ready reads) are not
   * skipped entirely */
  HAL_Delay((periodUS + 999) / 1000);
}

static int32_t
platform_read_raw_data_dma(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t len)
{
  HAL_StatusTypeDef ret;
  bmp5_selectDevice();
  ret = HAL_SPI_TransmitReceive_DMA(handle, txBufp, rxBufp, len);
  if (ret != HAL_OK)
  {
    bmp5_unselectDevice();
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
