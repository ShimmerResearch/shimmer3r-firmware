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
#include <stdio.h>
#include <string.h>

#include "BMP5_SensorAPI/bmp5.h"
#include "BMP5_SensorAPI/bmp5_selftest.h"

#include "hal_Board.h"
#include "hal_FactoryTest.h"

#define SENSOR_BUS                 hspi1

/* The BMP581 is a drop-in replacement for the BMP390 and so shares the same
 * chip-select and interrupt lines. */
#define CS_PORT                    CS_BMP390_GPIO_Port
#define CS_PIN                     CS_BMP390_Pin

/* The BMP581 loads its factory compensation trim from NVM on soft-reset; a
 * single reset+init can occasionally miss it (see bmp5_driver_init), so retry. */
#define BMP5_INIT_MAX_ATTEMPTS     3U

/* DEV-818 TEMP DIAG: force DSP_CONFIG.comp_pt_en at init to A/B-test the
 * on-chip compensation.  -1 = leave the reset default (0b11 = P&T compensated);
 * 0 = force OFF (raw, both);  3 = force ON.  Set to 0, reflash, and fridge-test
 * to compare against the default (comp=3) run: identical output => comp=3 is a
 * no-op (blank trim); different output => trim present but wrong. */
#define BMP5_DIAG_FORCE_COMP_PT_EN (-1)

/* DEV-818 accuracy tuning: the BMP581 streams noticeably noisier than the
 * host-compensated BMP390 (fridge test: raw noise ~185 Pa vs ~16 Pa). Two
 * datasheet-supported knobs cut that noise, both applied in bmp5_configure():
 *  - BMP5_STREAM_IIR_COEFF : enable the on-chip IIR low-pass. bmp5_configure()
 *    previously set no IIR, so it streamed bypassed (unfiltered). Higher coeff
 *    = more smoothing but more group delay. Comment out to bypass.
 *  - BMP5_STREAM_MIN_OSR_P : floor the pressure oversampling so a low
 * configured OSR still gets adequate resolution (paired temp OSR follows
 * automatically). Comment out to honour the configured OSR verbatim. */
#define BMP5_STREAM_IIR_COEFF      BMP5_IIR_FILTER_COEFF_7
#define BMP5_STREAM_MIN_OSR_P      BMP5_OVERSAMPLING_8X

struct bmp5_dev bmp5;

static struct bmp5_osr_odr_press_config bmp5OsrOdrPressCfg;

static bool bmp5DrdyIntEnabled = false;

/* DEV-818 TEMP DIAG (remove after validation): BMP581 compensation state
 * captured in bmp5_driver_init so an uncompensated boot (NVM trim not loaded)
 * can be confirmed on the bench via a debugger watch or the SWV/ITM console. */
volatile uint8_t bmp5DiagDspConfig = 0xFF; /* 0x30: comp_pt_en = bits[1:0], 0b11 = P&T comp */
volatile uint8_t bmp5DiagStatus = 0xFF; /* 0x28: nvm_rdy = 0x02, nvm_err = 0x04 */
volatile uint8_t bmp5DiagInitAttempts = 0; /* soft-reset+init attempts the loop needed */
volatile int8_t bmp5DiagInitRslt = 0; /* final result of the init retry loop */

/* DEV-818 TEMP DIAG (remove after validation): BMP581 trim/config NVM integrity
 * (datasheet 4.8.1.4). Mismatch / blank trim / zero UID => the part's factory
 * calibration NVM is corrupt or un-programmed. */
volatile uint16_t bmp5DiagNvmCrcCalcBe
    = 0; /* CRC-16-CCITT (poly 0x1021, init 0xFFFF), word MSB-first */
volatile uint16_t bmp5DiagNvmCrcCalcLe
    = 0; /* same, word LSB-first (datasheet doesn't pin byte order) */
volatile uint16_t bmp5DiagNvmCrcStored = 0; /* factory CRC at NVM 0x27 */
volatile uint16_t bmp5DiagNvmBlanks = 0; /* # covered NVM words that are 0x0000 or 0xFFFF */
volatile uint16_t bmp5DiagUidWord[4] = { 0, 0, 0, 0 }; /* NVM 0x23..0x26 (UID source) */
volatile int8_t bmp5DiagNvmReadRslt = 0;               /* NVM read status */

/* DEV-818 TEMP DIAG: comp_pt_en / STATUS re-read while STREAMING (see bmp5_configure). */
volatile uint8_t bmp5DiagStreamDspConfig = 0xFF;
volatile uint8_t bmp5DiagStreamStatus = 0xFF;

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

/* DEV-818 TEMP DIAG: CRC-16-CCITT (poly 0x1021), one byte at a time. */
static uint16_t bmp5_diag_crc16(uint16_t crc, uint8_t byte)
{
  uint8_t i;
  crc ^= (uint16_t) ((uint16_t) byte << 8);
  for (i = 0; i < 8U; i++)
  {
    crc = (crc & 0x8000U) ? (uint16_t) ((crc << 1) ^ 0x1021U) : (uint16_t) (crc << 1);
  }
  return crc;
}

/* DEV-818 TEMP DIAG: raw NVM read following the datasheet 4.8.1.1 sequence
 * directly (write NVM_ADDR with prog_en=0, USR_READ cmd 0x5D then 0xA5, settle,
 * read NVM_DATA_LSB/MSB). Bypasses the vendor bmp5_nvm_read()'s user-range
 * (0x20-0x22) guard, which otherwise returns BMP5_E_NVM_INVALID_ADDR (-10) for
 * the factory trim rows even though the datasheet permits reading them. Sensor
 * must be in standby. */
static int8_t bmp5_diag_nvm_read_raw(uint8_t addr, uint16_t *out)
{
  int8_t rslt;
  uint8_t reg;
  uint8_t data[2] = { 0, 0 };

  reg = (uint8_t) (addr & BMP5_NVM_ADDR_MSK); /* nvm_row_address, prog_en = 0 */
  rslt = bmp5_set_regs(BMP5_REG_NVM_ADDR, &reg, 1, &bmp5);
  if (rslt == BMP5_OK)
  {
    reg = BMP5_NVM_FIRST_CMND;
    rslt = bmp5_set_regs(BMP5_REG_CMD, &reg, 1, &bmp5);
  }
  if (rslt == BMP5_OK)
  {
    reg = BMP5_NVM_READ_ENABLE_CMND;
    rslt = bmp5_set_regs(BMP5_REG_CMD, &reg, 1, &bmp5);
  }
  if (rslt == BMP5_OK)
  {
    bmp5.delay_us(BMP5_DELAY_US_NVM_READY_READ, bmp5.intf_ptr);
    rslt = bmp5_get_regs(BMP5_REG_NVM_DATA_LSB, data, 2, &bmp5); /* 0x2C=LSB, 0x2D=MSB */
  }
  if (rslt == BMP5_OK)
  {
    *out = (uint16_t) (data[0] | ((uint16_t) data[1] << 8));
  }
  return rslt;
}

/* DEV-818 TEMP DIAG: verify the BMP581 factory trim/config NVM (datasheet
 * 4.8.1.4). Reads the CRC-covered rows (0x00-0x1D, 0x1F, 0x23-0x26 - excludes
 * the user range 0x20-0x22 and the ECC bits at 0x1E), recomputes the CRC-16 two
 * ways (the datasheet cites CCITT init 0xFFFF but also X.25, so byte order is
 * ambiguous - try both), compares to the stored CRC at 0x27, counts blank
 * (0x0000/0xFFFF) rows, and reads the UID rows. Must run in standby (post-init).
 * A MISMATCH / high blank count / zero UID indicates corrupt or un-programmed
 * calibration NVM => a bad/non-genuine part, not a firmware issue. */
static void bmp5_diag_check_nvm(void)
{
  static const uint8_t addrs[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x23, 0x24, 0x25, 0x26 };
  uint16_t crcBe = 0xFFFFU;
  uint16_t crcLe = 0xFFFFU;
  uint16_t blanks = 0U;
  uint16_t word = 0U;
  uint16_t stored = 0U;
  int8_t rslt = BMP5_OK;
  int8_t r;
  uint8_t i;

  /* NVM reads require standby (deep-standby disabled). */
  (void) bmp5_set_power_mode(BMP5_POWERMODE_STANDBY, &bmp5);

  for (i = 0U; i < (uint8_t) sizeof(addrs); i++)
  {
    r = bmp5_diag_nvm_read_raw(addrs[i], &word);
    if (r != BMP5_OK)
    {
      rslt = r;
      continue;
    }
    crcBe = bmp5_diag_crc16(crcBe, (uint8_t) (word >> 8));
    crcBe = bmp5_diag_crc16(crcBe, (uint8_t) (word & 0xFFU));
    crcLe = bmp5_diag_crc16(crcLe, (uint8_t) (word & 0xFFU));
    crcLe = bmp5_diag_crc16(crcLe, (uint8_t) (word >> 8));
    if ((word == 0x0000U) || (word == 0xFFFFU))
    {
      blanks++;
    }
    if (addrs[i] == 0x23U)
    {
      bmp5DiagUidWord[0] = word;
    }
    else if (addrs[i] == 0x24U)
    {
      bmp5DiagUidWord[1] = word;
    }
    else if (addrs[i] == 0x25U)
    {
      bmp5DiagUidWord[2] = word;
    }
    else if (addrs[i] == 0x26U)
    {
      bmp5DiagUidWord[3] = word;
    }
  }
  if (bmp5_diag_nvm_read_raw(0x27U, &stored) != BMP5_OK)
  {
    rslt = (rslt == BMP5_OK) ? BMP5_E_COM_FAIL : rslt;
  }

  bmp5DiagNvmCrcCalcBe = crcBe;
  bmp5DiagNvmCrcCalcLe = crcLe;
  bmp5DiagNvmCrcStored = stored;
  bmp5DiagNvmBlanks = blanks;
  bmp5DiagNvmReadRslt = rslt;

  printf(
      "BMP581 NVM diag: readRslt=%d blanks=%u/%u CRC_be=0x%04X CRC_le=0x%04X "
      "stored[0x27]=0x%04X %s UID=%04X %04X %04X %04X\r\n",
      (int) rslt, (unsigned) blanks, (unsigned) sizeof(addrs), (unsigned) crcBe,
      (unsigned) crcLe, (unsigned) stored,
      ((crcBe == stored) || (crcLe == stored)) ? "MATCH" : "MISMATCH",
      (unsigned) bmp5DiagUidWord[0], (unsigned) bmp5DiagUidWord[1],
      (unsigned) bmp5DiagUidWord[2], (unsigned) bmp5DiagUidWord[3]);
}

void bmp5_driver_init(void)
{
  int8_t rslt = BMP5_OK;
  uint8_t attempt;

  bmp5_setup_dev();

  /* Reads and verifies the chip ID and leaves the sensor in standby. The
   * BMP581 outputs pre-compensated pressure and temperature so, unlike the
   * BMP390, there are no calibration coefficients to read here.
   *
   * IMPORTANT: the BMP581 loads its factory compensation trim from NVM during
   * the soft-reset/POR sequence, and the data registers only stream
   * *compensated* pressure/temperature once that trim is loaded (comp_pt_en
   * defaults to enabled). bmp5_init()'s NVM-ready check (STATUS.nvm_rdy) is a
   * single read with no settle, and the BMP581's first SPI access after
   * power-up returns invalid data - so a single reset+init can leave the part
   * streaming UNCOMPENSATED for the whole session. Observed on a fridge sweep:
   * one unit's raw pressure tracked temperature at ~410 Pa/degC (a ~7 kPa swing
   * over ~17 degC) versus the +/-0.5 Pa/degC datasheet spec, while a co-located
   * BMP390 stayed flat. Reset, let POR/NVM settle, then init - and retry - so
   * the trim reliably loads before streaming. Return values are checked (the
   * vendor API otherwise fails silently and the caller ignores it). */
  for (attempt = 0; attempt < BMP5_INIT_MAX_ATTEMPTS; attempt++)
  {
    rslt = bmp5_soft_reset(&bmp5);
    /* Let POR/NVM-load complete before bmp5_init() reads nvm_rdy (matches the
     * settle used in the BMP581 self-test path). */
    platform_delay(3);
    if (rslt == BMP5_OK)
    {
      rslt = bmp5_init(&bmp5);
    }
    if (rslt == BMP5_OK)
    {
      break; /* chip up and NVM trim loaded */
    }
  }
  (void) rslt; /* best-effort: proceed even if all attempts failed (HW fault) */

#if (BMP5_DIAG_FORCE_COMP_PT_EN >= 0)
  /* DEV-818 TEMP DIAG: override comp_pt_en (read-modify-write DSP_CONFIG while
   * the sensor is in standby after init) to A/B-test the on-chip compensation. */
  {
    uint8_t dsp = 0;
    if (bmp5_get_regs(BMP5_REG_DSP_CONFIG, &dsp, 1, &bmp5) == BMP5_OK)
    {
      dsp = (uint8_t) ((dsp & ~0x03U) | ((uint8_t) BMP5_DIAG_FORCE_COMP_PT_EN & 0x03U));
      (void) bmp5_set_regs(BMP5_REG_DSP_CONFIG, &dsp, 1, &bmp5);
    }
  }
#endif

  /* DEV-818 TEMP DIAG (remove after validation): capture the as-initialised
   * compensation state so an uncompensated boot (NVM trim not loaded) can be
   * confirmed on the bench. Expect comp_pt_en (DSP_CONFIG[1:0]) = 3 (0b11) and
   * STATUS.nvm_rdy set / nvm_err clear. attempts > 1 => the trim missed the
   * first load and the retry recovered it. View via a debugger watch on the
   * bmp5Diag* globals, or the SWV/ITM console (printf). */
  {
    uint8_t reg = 0xFF;
    bmp5DiagInitAttempts = (attempt < BMP5_INIT_MAX_ATTEMPTS) ?
        (uint8_t) (attempt + 1U) :
        (uint8_t) BMP5_INIT_MAX_ATTEMPTS;
    bmp5DiagInitRslt = rslt;
    if (bmp5_get_regs(BMP5_REG_DSP_CONFIG, &reg, 1, &bmp5) == BMP5_OK)
    {
      bmp5DiagDspConfig = reg;
    }
    reg = 0xFF;
    if (bmp5_get_regs(BMP5_REG_STATUS, &reg, 1, &bmp5) == BMP5_OK)
    {
      bmp5DiagStatus = reg;
    }
    printf(
        "BMP581 init diag: attempts=%u rslt=%d DSP_CONFIG=0x%02X comp_pt_en=%u "
        "STATUS=0x%02X nvm_rdy=%u nvm_err=%u\r\n",
        (unsigned) bmp5DiagInitAttempts, (int) bmp5DiagInitRslt, (unsigned) bmp5DiagDspConfig,
        (unsigned) (bmp5DiagDspConfig & 0x03U), (unsigned) bmp5DiagStatus,
        (unsigned) ((bmp5DiagStatus & BMP5_INT_NVM_RDY) ? 1U : 0U),
        (unsigned) ((bmp5DiagStatus & BMP5_INT_NVM_ERR) ? 1U : 0U));
  }

  /* DEV-818 TEMP DIAG: verify the factory trim/config NVM integrity (datasheet
   * 4.8.1.4) - the direct check of whether the compensation data is valid. */
  bmp5_diag_check_nvm();
}

int8_t bmp5_verify_chip_id(void)
{
  uint8_t chip_id = 0;
  int8_t rslt;

  /* The BMP581 returns invalid data on the very first SPI read after power-up
   * (the Bosch driver performs the same throwaway CHIP_ID read inside
   * bmp5_soft_reset() and bmp5_init() for exactly this reason). This function
   * is the first access to the freshly-powered sensor in
   * PressureSensor_detect(), so discard one read first - otherwise the chip ID
   * reads as garbage and detection silently falls back to the SR number instead
   * of confirming the part. */
  (void) bmp5_get_regs(BMP5_REG_CHIP_ID, &chip_id, 1, &bmp5);

  rslt = bmp5_get_regs(BMP5_REG_CHIP_ID, &chip_id, 1, &bmp5);
  if (rslt == BMP5_OK && (chip_id == BMP5_CHIP_ID_PRIM || chip_id == BMP5_CHIP_ID_SEC))
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

  /* Soft-reset before the self-test. Unlike bmp3_selftest_check() (which
   * soft-resets before bmp3_init()), the vendor's bmp5_selftest_check() calls
   * bmp5_init() with no preceding reset, and bmp5_init()'s NVM-ready check
   * reads STATUS.nvm_rdy just once with no settle/retry. The BMP581's first
   * SPI read after power-up returns invalid data, so the single soft-reset in
   * bmp5_driver_init() cannot reliably confirm POR/NVM-ready. Resetting here -
   * with SPI already awake - reloads the NVM and makes the ready bit valid,
   * avoiding a spurious BMP5_E_NVM_NOT_READY (-5) self-test failure. */
  bmp5_soft_reset(&bmp5);
  platform_delay(3);

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
        /* Configure the interrupt (latched, active-high) BEFORE routing the
         * data-ready source to the pin. The BMP581 requires the latch/polarity
         * to be set while the INT source is still deselected (Bosch bmp5
         * driver note + bmp5_selftest example: "Select INT_SOURCE after
         * configuring interrupt"). Doing int_source_select first can leave
         * DRDY not actually driven on the INT pin. */
        rslt = bmp5_configure_interrupt(BMP5_LATCHED, BMP5_ACTIVE_HIGH,
            BMP5_INTR_PUSH_PULL, BMP5_INTR_ENABLE, &bmp5);
      }
      if (rslt == BMP5_OK)
      {
        /* Now route data-ready to the INT pin */
        int_source_select.drdy_en = BMP5_ENABLE;
        rslt = bmp5_int_source_select(&int_source_select, &bmp5);
      }

      if (rslt == BMP5_OK)
      {
        rslt = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, &bmp5);
      }

      /* Give the sensor time to enter normal mode and produce the first
       * conversion before polling the interrupt pin. */
      platform_delay(100);

      if (rslt == BMP5_OK)
      {
        /* New sample is every 20ms @ 50Hz. Loop count + delay below allows
         * ~50ms for DRDY to toggle. */
        for (i = 0; i < 50; i++)
        {
          if (BMP581_INT)
          {
            /* read the sensor data */
            bmp5_get_sensor_data(&bmp5SelftestData, &osr_odr_press_cfg, &bmp5);

            /* Read interrupt status again to clear the data-ready interrupt */
            rslt = bmp5_get_interrupt_status(&int_status, &bmp5);

            /* In latched mode the pin de-asserts the instant INT_STATUS is read,
             * then re-asserts on the next sample. Verify de-assertion with a tight
             * busy-poll immediately after the clear (no ms delay) so we don't race
             * the next conversion and false-fail a good sensor. 0 = fail, 1 = pass */
            uint8_t deasserted = 0;
            for (uint16_t p = 0; p < 2000; p++)
            {
              if (!BMP581_INT)
              {
                deasserted = 1;
                break;
              }
            }
            res = ((rslt == BMP5_OK) && deasserted) ? 1 : 0;
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
#ifdef BMP5_STREAM_MIN_OSR_P
  /* Ensure a minimum pressure oversampling for acceptable noise, even if the
   * configured OSR is low. The ODR-feasibility loop below steps the ODR down as
   * needed so the higher OSR is actually achievable. */
  if (bmp5OsrOdrPressCfg.osr_p < BMP5_STREAM_MIN_OSR_P)
  {
    bmp5OsrOdrPressCfg.osr_p = BMP5_STREAM_MIN_OSR_P;
  }
#endif
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

#ifdef BMP5_STREAM_IIR_COEFF
  /* Enable the on-chip IIR low-pass to cut streaming noise. Config registers
   * are writable here because we are still in standby. shdw_set_iir_* routes
   * the filtered value to the data registers we read. bmp5_set_iir_config does
   * a read-modify-write of DSP_CONFIG, so comp_pt_en is preserved. */
  {
    struct bmp5_iir_config iir_cfg = { 0 };
    iir_cfg.set_iir_t = BMP5_STREAM_IIR_COEFF;
    iir_cfg.set_iir_p = BMP5_STREAM_IIR_COEFF;
    iir_cfg.shdw_set_iir_t = BMP5_ENABLE;
    iir_cfg.shdw_set_iir_p = BMP5_ENABLE;
    rslt = bmp5_set_iir_config(&iir_cfg, &bmp5);
    if (rslt != BMP5_OK)
    {
      return rslt;
    }
  }
#endif

  /* Enable the data-ready interrupt (latched, active-high) - routed to both the
   * INT pin and the STATUS register - and enter NORMAL mode. Configure the
   * interrupt BEFORE selecting the source (Bosch note: "Select INT_SOURCE after
   * configuring interrupt"). */
  rslt = bmp5_configure_interrupt(BMP5_LATCHED, BMP5_ACTIVE_HIGH,
      BMP5_INTR_PUSH_PULL, BMP5_INTR_ENABLE, &bmp5);
  if (rslt != BMP5_OK)
  {
    return rslt;
  }
  int_source_select.drdy_en = BMP5_ENABLE;
  rslt = bmp5_int_source_select(&int_source_select, &bmp5);
  if (rslt != BMP5_OK)
  {
    return rslt;
  }
  rslt = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, &bmp5);
  if (rslt != BMP5_OK)
  {
    return rslt;
  }

  /* Auto-detect the DRDY interrupt line. If the physical INT pin toggles within
   * the window, gate sensing reads on it (efficient). If it does not - e.g. the
   * shared BMP390_INT line is unreliable on this board - fall back to polling
   * the data-ready status over SPI. The data-ready source stays enabled either
   * way, so both the INT pin and the STATUS register track new samples and the
   * data registers update consistently (a blind read otherwise returns
   * transitional/invalid values). Up to 100 x 2ms = 200ms (~10 samples @50Hz). */
  bmp5DrdyIntEnabled = false;
  for (i = 0; i < 100; i++)
  {
    if (BMP581_INT)
    {
      bmp5DrdyIntEnabled = true;
      bmp5_read_int_status(); /* clear the latched interrupt */
      break;
    }
    platform_delay(2);
  }

  /* DEV-818 TEMP DIAG: confirm comp_pt_en is still set while STREAMING (NORMAL
   * mode) - not just at init. Nothing writes DSP_CONFIG after init, so this
   * should read comp_pt_en=3; a change here would explain uncompensated output.
   * (A register read is safe during an ongoing conversion; only certain DSP
   * field WRITES are restricted.) */
  {
    uint8_t reg = 0xFF;
    if (bmp5_get_regs(BMP5_REG_DSP_CONFIG, &reg, 1, &bmp5) == BMP5_OK)
    {
      bmp5DiagStreamDspConfig = reg;
    }
    reg = 0xFF;
    if (bmp5_get_regs(BMP5_REG_STATUS, &reg, 1, &bmp5) == BMP5_OK)
    {
      bmp5DiagStreamStatus = reg;
    }
    printf("BMP581 stream diag: DSP_CONFIG=0x%02X comp_pt_en=%u STATUS=0x%02X "
           "nvm_rdy=%u nvm_err=%u\r\n",
        (unsigned) bmp5DiagStreamDspConfig,
        (unsigned) (bmp5DiagStreamDspConfig & 0x03U), (unsigned) bmp5DiagStreamStatus,
        (unsigned) ((bmp5DiagStreamStatus & BMP5_INT_NVM_RDY) ? 1U : 0U),
        (unsigned) ((bmp5DiagStreamStatus & BMP5_INT_NVM_ERR) ? 1U : 0U));
  }

  return rslt;
}

HAL_StatusTypeDef bmp5_pressure_temperature_get(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  /* The BMP581 bursts temperature (0x1D-0x1F) followed by pressure
   * (0x20-0x22). No dummy byte is inserted after the register address. */
  static uint8_t txBuff[]
      = { BMP5_REG_TEMP_DATA_XLSB | SPI_READ_REGISTER, 0, 0, 0, 0, 0, 0 };
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
