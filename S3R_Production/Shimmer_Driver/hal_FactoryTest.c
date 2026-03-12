/*
 * hal_FactoryTest.c
 *
 *  Created on: Aug 14, 2024
 *      Author: MarkNolan
 */

#include "hal_FactoryTest.h"

#include <inttypes.h>

#include "i2c.h"
#include "spi.h"

#include "BMP3/BMP3_SensorAPI/bmp3_defs.h"
#include "BMP3/BMP3_SensorAPI/self-test/bmp3_selftest.h"
#include "PCM/pcm_config.h"


char *buffer;

uint8_t test_i2c_addr_list[128], test_i2c_addr_list_len;

static uint32_t testGsrResistances[] = { 12000L, 48800L, //GSR Range 0 (8.0kOhm-63.0kOhm)
  76000L, 197000L,                //GSR Range 1 (63.0kOhm-220.0kOhm)
  248000L, 498000L,               //GSR Range 2 (220.0kOhm-680.0kOhm)
  2000000L, 3000000L, 4000000L }; //GSR Range 3 (680.0kOhm-4.7MOhm)

uint32_t gsrResistance[sizeof(testGsrResistances)];

static float gsrFactoryTest_getPassToleranceForTestResistor(uint32_t testResistor);
static uint32_t gsrFactoryTest_getRefResistorForTestResistor(uint32_t testResistor);

void hal_run_factory_test(factory_test_t factoryTestToRun, char *bufPtr)
{
  buffer = bufPtr;

  if (factoryTestToRun == FACTORY_TEST_MAIN || factoryTestToRun == FACTORY_TEST_ICS)
  {
    print_date_and_time();
    ShimFactoryTest_sendReport("\r\n");

    sprintf(buffer, "INFO: Temperature pass range set to %.0f-%.0f\xC2\xB0 C\r\n",
        TEST_THRESHOLD_DEG_IMU_TEMPERATURE_LOWER, TEST_THRESHOLD_DEG_IMU_TEMPERATURE_UPPER);
    ShimFactoryTest_sendReport(buffer);
    ShimFactoryTest_sendReport("\r\n");

    print_shimmer_model();
    ShimFactoryTest_sendReport("\r\n");

    //Sensing power is needed for ADC peripheral and I2C/SPI tests
    Board_enableSensingPower(SENSE_PWR_FACTORY_TEST, 1);

    print_mcu_details();
    ShimFactoryTest_sendReport("\r\n");

    print_battery_details();
    ShimFactoryTest_sendReport("\r\n");

    sd_card_test();
    ShimFactoryTest_sendReport("\r\n");

    bt_module_test();
    ShimFactoryTest_sendReport("\r\n");

    //InfoMem_test();

    SPI_test();
    ShimFactoryTest_sendReport("\r\n");

    I2C_test();
    ShimFactoryTest_sendReport("\r\n");

    Board_enableSensingPower(SENSE_PWR_FACTORY_TEST, 0);

    runMicrophoneTest();
  }

  if (factoryTestToRun == FACTORY_TEST_MAIN || factoryTestToRun == FACTORY_TEST_LEDS)
  {
    if (factoryTestToRun == FACTORY_TEST_MAIN)
    {
      ShimFactoryTest_sendReport("\r\n");
    }
    led_test();
  }
}

void print_date_and_time(void)
{
  uint32_t format = RTC_FORMAT_BIN;
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  /* Get time */
  HAL_RTC_GetTime(&hrtc, &sTime, format);
  /* Get date */
  HAL_RTC_GetDate(&hrtc, &sDate, format);
  sprintf(buffer, "Date (yyyy-mm-dd): %.4u-%.2u-%.2u\r\n",
      ((sDate.Year >= 70 ? 1900 : 2000) + sDate.Year), sDate.Month, sDate.Date);
  ShimFactoryTest_sendReport(buffer);
  sprintf(buffer, "Time (hh:mm:ss): %.2u:%.2u:%.2u (UTC)\r\n", sTime.Hours,
      sTime.Minutes, sTime.Seconds);
  ShimFactoryTest_sendReport(buffer);
}

void print_shimmer_model(void)
{
  ShimFactoryTest_sendReport("Shimmer model:\r\n");
  if (ShimBrd_isDaughterCardIdSet())
  {
    sprintf(buffer, " - S3R_TEST_0003 - PASS: %s", ShimBrd_getDaughtCardIdStrPtr());
    ShimFactoryTest_sendReport(buffer);
    shimmer_expansion_brd *daughterCardId = ShimBrd_getDaughtCardId();
    sprintf(buffer, " (SR%d-%d-%d)\r\n", daughterCardId->exp_brd_id,
        daughterCardId->exp_brd_major, daughterCardId->exp_brd_minor);
    ShimFactoryTest_sendReport(buffer);
  }
  else
  {
    ShimFactoryTest_sendReport(" - S3R_TEST_0003 - FAIL: not set\r\n");
    shimmerStatus.testResult |= S3R_TEST_0003;
  }
}

void print_mcu_details(void)
{
  ShimFactoryTest_sendReport("MCU:\r\n");

  sprintf(buffer, " - Device ID = %lu\r\n", HAL_GetDEVID());
  ShimFactoryTest_sendReport(buffer);
  sprintf(buffer, " - Revision ID = %lu\r\n", HAL_GetREVID());
  ShimFactoryTest_sendReport(buffer);

  /*
   * UID[31:0]: X and Y coordinates on the wafer expressed in BCD format
   * UID[63:40]: LOT_NUM[23:0] Lot number (ASCII encoded)
   * UID[39:32]: WAF_NUM[7:0] Wafer number (8-bit unsigned number)
   * UID[95:64]: LOT_NUM[55:24] Lot number (ASCII encoded)
   * */
  //sprintf(buffer, " - Unique ID w0 = 0x%08X\r\n", HAL_GetUIDw0());
  //ShimFactoryTest_sendReport(buffer);
  //sprintf(buffer, " - Unique ID w1 = 0x%08X\r\n", HAL_GetUIDw1());
  //ShimFactoryTest_sendReport(buffer);
  //sprintf(buffer, " - Unique ID w2 = 0x%08X\r\n", HAL_GetUIDw2());
  //ShimFactoryTest_sendReport(buffer);
  //sprintf(buffer, " - Unique ID = 0x%08X%08X%08X\r\n", HAL_GetUIDw0(),
  //    HAL_GetUIDw1(), HAL_GetUIDw2());
  sprintf(buffer, " - Unique ID = 0x%08" PRIX32 "%08" PRIX32 "%08" PRIX32 "\r\n",
      HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());
  ShimFactoryTest_sendReport(buffer);

  ADCDebugInfo_t adcDebugInfo;
  getherMcuDebugInfo(&adcDebugInfo);

  uint8_t testPass = (adcDebugInfo.vRefMV > TEST_THRESHOLD_VREF_LOWER
      && adcDebugInfo.vRefMV < TEST_THRESHOLD_VREF_UPPER);
  sprintf(buffer, " - S3R_TEST_0007 - %s: VRef = %ldmV (%d-%dmV)\r\n",
      testPass ? "PASS" : "FAIL", adcDebugInfo.vRefMV,
      TEST_THRESHOLD_VREF_LOWER, TEST_THRESHOLD_VREF_UPPER);
  ShimFactoryTest_sendReport(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0007;
  }

  testPass = (adcDebugInfo.vCoreMV > TEST_THRESHOLD_MV_VCORE_LOWER
      && adcDebugInfo.vCoreMV < TEST_THRESHOLD_MV_VCORE_UPPER);
  sprintf(buffer, " - S3R_TEST_0008 - %s: VCore = %ldmV (%d-%dmV)\r\n",
      testPass ? "PASS" : "FAIL", adcDebugInfo.vCoreMV,
      TEST_THRESHOLD_MV_VCORE_LOWER, TEST_THRESHOLD_MV_VCORE_UPPER);
  ShimFactoryTest_sendReport(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0008;
  }

  if (ShimBrd_isBoardSr48_6_0())
  {
    testPass = (adcDebugInfo.vBattPinMV > TEST_THRESHOLD_MV_VBATT_PIN_LOWER_SR48_6_0
        && adcDebugInfo.vBattPinMV < TEST_THRESHOLD_MV_VBATT_PIN_UPPER_SR48_6_0);
    sprintf(buffer, " - S3R_TEST_0009 - %s: VBatt pin = %ldmV (%d-%dmV)\r\n",
        testPass ? "PASS" : "FAIL", adcDebugInfo.vBattPinMV,
        TEST_THRESHOLD_MV_VBATT_PIN_LOWER_SR48_6_0,
        TEST_THRESHOLD_MV_VBATT_PIN_UPPER_SR48_6_0);
  }
  else
  {
    testPass = (adcDebugInfo.vBattPinMV > TEST_THRESHOLD_MV_VBATT_PIN_LOWER
        && adcDebugInfo.vBattPinMV < TEST_THRESHOLD_MV_VBATT_PIN_UPPER);
    sprintf(buffer, " - S3R_TEST_0009 - %s: VBatt pin = %ldmV (%d-%dmV)\r\n",
        testPass ? "PASS" : "FAIL", adcDebugInfo.vBattPinMV,
        TEST_THRESHOLD_MV_VBATT_PIN_LOWER, TEST_THRESHOLD_MV_VBATT_PIN_UPPER);
  }
  ShimFactoryTest_sendReport(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0009;
  }

  testPass = (adcDebugInfo.temperature > TEST_THRESHOLD_MV_MCU_TEMPERATURE_LOWER
      && adcDebugInfo.temperature < TEST_THRESHOLD_MV_MCU_TEMPERATURE_UPPER);
  sprintf(buffer, " - S3R_TEST_0010 - %s: Temperature = %ld\xC2\xB0 C\r\n",
      testPass ? "PASS" : "FAIL", adcDebugInfo.temperature);
  ShimFactoryTest_sendReport(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0010;
  }

  ShimFactoryTest_sendReport(" - I/O status:\r\n");
  sprintf(buffer, "    - Docked: %s\r\n", shimmerStatus.docked ? "Yes" : "No");
  ShimFactoryTest_sendReport(buffer);
  sprintf(buffer, "    - BT connected: %s\r\n", shimmerStatus.btConnected ? "Yes" : "No");
  ShimFactoryTest_sendReport(buffer);
  sprintf(buffer, "    - Button pressed: %s\r\n", shimmerStatus.buttonPressed ? "Yes" : "No");
  ShimFactoryTest_sendReport(buffer);
  sprintf(buffer, "    - USB connected: %s\r\n", shimmerStatus.usbPluggedIn ? "Yes" : "No");
  ShimFactoryTest_sendReport(buffer);
}

void print_battery_details(void)
{
  ShimFactoryTest_sendReport("Battery:\r\n");
  manageReadBatt(1);

  uint8_t testPass = (batteryStatus.battValMV > TEST_THRESHOLD_MV_VBATT_LOWER
      && batteryStatus.battValMV < TEST_THRESHOLD_MV_VBATT_UPPER);
  sprintf(buffer, " - S3R_TEST_0011 - %s: VBatt = %dmV (%d-%dmV)\r\n",
      testPass ? "PASS" : "FAIL", batteryStatus.battValMV,
      TEST_THRESHOLD_MV_VBATT_LOWER, TEST_THRESHOLD_MV_VBATT_UPPER);
  ShimFactoryTest_sendReport(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0011;
  }

  testPass = batteryStatus.battStatusRaw.rawBytes[2] == CHRG_CHIP_STATUS_BAD_BATTERY ? 0 : 1;
  sprintf(buffer, " - S3R_TEST_0012 - %s: Charger chip status = ", testPass ? "PASS" : "FAIL");
  ShimFactoryTest_sendReport(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0012;
  }

  switch (batteryStatus.battStatusRaw.rawBytes[2])
  {
  case CHRG_CHIP_STATUS_SUSPENDED:
    sprintf(buffer, "'Power-Down, or charging is suspended or interrupted'\r\n");
    break;
  case CHRG_CHIP_STATUS_FULLY_CHARGED:
    sprintf(buffer, "Charge is completed\r\n");
    break;
  case CHRG_CHIP_STATUS_PRECONDITIONING:
    sprintf(buffer, "'Pre-qualification mode, CC and CV charging, or Top-off mode'\r\n");
    break;
  case CHRG_CHIP_STATUS_BAD_BATTERY:
    sprintf(buffer, "'Bad battery (Safety timer expired), or LDO mode'\r\n");
    break;
  default:
    //Shouldn't reach here unless fault with FW
    sprintf(buffer, "Unknown\r\n");
    break;
  }
  ShimFactoryTest_sendReport(buffer);

  ShimFactoryTest_sendReport(" - Determined charging status = ");
  switch (batteryStatus.battChargingStatus)
  {
  case CHARGING_STATUS_CHECKING:
    sprintf(buffer, "Checking\r\n");
    break;
  case CHARGING_STATUS_SUSPENDED:
    sprintf(buffer, "Suspended\r\n");
    break;
  case CHARGING_STATUS_FULLY_CHARGED:
    sprintf(buffer, "Fully Charged\r\n");
    break;
  case CHARGING_STATUS_CHARGING:
    sprintf(buffer, "Charging\r\n");
    break;
  case CHARGING_STATUS_BAD_BATTERY:
    sprintf(buffer, "Bad Battery\r\n");
    break;
  case CHARGING_STATUS_ERROR:
    sprintf(buffer, "Error\r\n");
    break;
  default:
    //Shouldn't reach here unless fault with FW
    sprintf(buffer, "Unknown\r\n");
    break;
  }
  ShimFactoryTest_sendReport(buffer);
}

void led_test(void)
{
  ShimFactoryTest_sendReport("LED test (S3R_TEST_0027):\r\n");

#if defined(SHIMMER3R)
  stopLedBlinkTimer();

  Board_ledLwrSetColour(LED_RGB_ALL_OFF);
  Board_ledUprSetColour(LED_RGB_ALL_OFF);
  ShimFactoryTest_sendReport(" - All LEDs off\r\n");
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  ShimFactoryTest_sendReport(" - Lower Red LED on\r\n");
  Board_ledLwrSetColour(LED_RGB_RED);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  ShimFactoryTest_sendReport(" - Lower Green LED on\r\n");
  Board_ledLwrSetColour(LED_RGB_GREEN);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  ShimFactoryTest_sendReport(" - Lower Blue LED on\r\n");
  Board_ledLwrSetColour(LED_RGB_BLUE);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  Board_ledLwrSetColour(LED_RGB_ALL_OFF);

  ShimFactoryTest_sendReport(" - Upper Red LED on\r\n");
  Board_ledUprSetColour(LED_RGB_RED);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  ShimFactoryTest_sendReport(" - Upper Green LED on\r\n");
  Board_ledUprSetColour(LED_RGB_GREEN);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  ShimFactoryTest_sendReport(" - Upper Blue LED on\r\n");
  Board_ledUprSetColour(LED_RGB_BLUE);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  ShimFactoryTest_sendReport(" - All LEDs off\r\n");
  Board_ledUprSetColour(LED_RGB_ALL_OFF);

  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  ShimFactoryTest_sendReport(" - All LEDs on\r\n");
  Board_ledLwrSetColour(LED_RGB_ALL_ON);
  Board_ledUprSetColour(LED_RGB_ALL_ON);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  Board_ledLwrSetColour(LED_RGB_ALL_OFF);
  Board_ledUprSetColour(LED_RGB_ALL_OFF);

  startLedBlinkTimer();

#elif defined(SHIMMER4_SDK)
  S4_RTC_WakeUpOff();

  Board_ledOff(LED_ALL);
  Board_ledOn(LED_BLUE_LWR);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  Board_ledOff(LED_ALL);
  Board_ledOn(LED_GREEN_LWR);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  Board_ledOff(LED_ALL);
  Board_ledOn(LED_RED_LWR);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  Board_ledOff(LED_ALL);
  Board_ledOn(LED_BLUE_UPR);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  Board_ledOff(LED_ALL);
  Board_ledOn(LED_GREEN_UPR);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  Board_ledOff(LED_ALL);
  Board_ledOn(LED_RED_UPR);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  Board_ledOff(LED_ALL);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  Board_ledOn(LED_ALL);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  S4_RTC_WakeUpSetSlow();
#endif
}

void sd_card_test(void)
{
  ShimFactoryTest_sendReport("SD Card:\r\n");
  if (!shimmerStatus.sdInserted)
  {
    ShimFactoryTest_sendReport(" - S3R_TEST_0013 - FAIL: not detected\r\n");
    shimmerStatus.testResult |= S3R_TEST_0013;
  }
  else
  {
    if (shimmerStatus.docked)
    {
      Board_sd2Mcu();
      HAL_Delay(120); //120ms
    }

    ShimFactoryTest_sendReport(" - ");
    printSdCardInfo(buffer);
    ShimFactoryTest_sendReport(buffer);

    shimmerStatus.testResult += ShimSd_test1() << 6;
    //SD_test_alternative();
    sprintf(buffer, " - S3R_TEST_0013 - %s: MCU read/write test\r\n",
        shimmerStatus.sdBadFile ? "FAIL" : "PASS");
    ShimFactoryTest_sendReport(buffer);

    if (shimmerStatus.sdBadFile)
    {
      shimmerStatus.testResult |= S3R_TEST_0013;
    }

    if (shimmerStatus.docked)
    {
      Board_sd2Pc();
    }
  }
}

void bt_module_test(void)
{
  ShimFactoryTest_sendReport("BT Module:\r\n");
  /* Check MAC and BT version have been read */
  if (*(ShimBt_macIdStrPtrGet()) != 0x00 && *(ShimBt_getBtVerStrPtr()) != 0x00)
  {
    ShimFactoryTest_sendReport(" - MAC ID: ");
    memcpy(&buffer[0], ShimBt_macIdStrPtrGet(), 12);
    sprintf(&buffer[12], "\r\n");
    ShimFactoryTest_sendReport(buffer);

    sprintf(buffer, " - %s\r\n", ShimBt_getBtVerStrPtr());
    ShimFactoryTest_sendReport(buffer);

    sprintf(buffer, " - S3R_TEST_0014 - %s BT firmware version\r\n",
        strstr(buffer, TEST_BT_MODULE_FW) != NULL ? "PASS: Correct" : "FAIL: Incorrect");
    ShimFactoryTest_sendReport(buffer);
  }
  else
  {
    ShimFactoryTest_sendReport(
        " - S3R_TEST_0014 - FAIL - BT hasn't initialised\r\n");
    shimmerStatus.testResult |= S3R_TEST_0014;
  }
}

void I2C_test(void)
{
  self_test_result_t self_test_result = SELF_TEST_PASS;
  float_t tempCal = 0;

  MX_I2C1_Init();

  I2C_scan_busses();

#if defined(SHIMMER4_SDK)
  MPU9250_init(hi2cMainBus);
  if (MPU9250_test())
  {
    ret_val |= 0x01;
  }

  LSM303DLHC_init(hi2cMainBus);
  if (LSM303DLHC_accelTest())
  {
    ret_val |= 0x02;
  }
  if (LSM303DLHC_magTest())
  {
    ret_val |= 0x04;
  }

  Board_SW_EXP(1);

  CAT24C16_init(I2C_getHandlerSensor());
  //HAL_Delay(1000);
  if (CAT24C16_test()) //eeprom
  {
    ret_val |= 0x08;
  }
#elif defined(SHIMMER3R)
  ShimFactoryTest_sendReport("I2C1:\r\n");

  tempCal = TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID;
  self_test_result = lis2mdl_self_test();
  if (self_test_result == SELF_TEST_PASS)
  {
    //Get last temperature value left over from self test
    self_test_result = lis2mdl_temperature_get(&tempCal);
    if (self_test_result || is_temperature_outside_of_range(tempCal))
    {
      self_test_result = SELF_TEST_FAIL_TEMPERATURE_ISSUE;
    }
  }
  if (self_test_result)
  {
    shimmerStatus.testResult |= S3R_TEST_0022;
  }
  print_chip_test_result("S3R_TEST_0022", "LIS2MDL", self_test_result, tempCal);

  uint8_t eeprom_result = eepromTest();
  sprintf(buffer, " - S3R_TEST_0023 - %s: CAT24C16\r\n", eeprom_result ? "FAIL" : "PASS");
  ShimFactoryTest_sendReport(buffer);
  if (eeprom_result)
  {
    shimmerStatus.testResult |= S3R_TEST_0023;
  }
#endif

#if defined(SHIMMER4_SDK)
  if (bmp280_test(hi2cMainBus))
  {
    ret_val |= 0x10;
  }
#endif

  I2C1_DeInit();

#if defined(SHIMMER3R)
  ShimFactoryTest_sendReport("I2C4:\r\n");
  if (ShimBrd_isI2c4Supported())
  {
    uint8_t i2c4_result = 0;

    enableI2cOnSr48OrSr38PpgSocket(1);
    HAL_Delay(2); //2ms as per Shimmer3 code
    I2C_scan_internal_expansion_bus(test_i2c_addr_list, &test_i2c_addr_list_len);

    if (test_i2c_addr_list_len == 0) //Nothing detected on I2C bus
    {
      ShimFactoryTest_sendReport(
          " - S3R_TEST_0024 - FAIL: I2C4 - no test rig detected\r\n");
      ShimFactoryTest_sendReport(
          " - S3R_TEST_0025 - FAIL: GSR - refer to S3R_TEST_0024\r\n");
      i2c4_result = 1;
    }
    else if (test_i2c_addr_list_len == 3) //GSR Test Rig detected
    {
      ShimFactoryTest_sendReport(" - S3R_TEST_0024 - PASS: I2C4\r\n");

      uint8_t gsr_result = gsrFactoryTest_run();
      sprintf(buffer, " - S3R_TEST_0025 - %s: GSR signal test\r\n",
          gsr_result ? "FAIL" : "PASS");
      ShimFactoryTest_sendReport(buffer);

      if (gsr_result)
      {
        gsrFactoryTest_printResults();
        shimmerStatus.testResult |= S3R_TEST_0025;
      }
    }
    else if (test_i2c_addr_list_len == 8) //EEPROM detected
    {
      altEepromInit(&hi2c4);
      i2c4_result = altEepromTest();
      HAL_Delay(5); //5ms to ensure no writes pending

      sprintf(buffer, " - S3R_TEST_0024 - %s: I2C4\r\n", i2c4_result ? "FAIL" : "PASS");
      ShimFactoryTest_sendReport(buffer);
      if (ShimBrd_isHwId(EXP_BRD_GSR_UNIFIED))
      {
        ShimFactoryTest_sendReport(" - S3R_TEST_0025 - WARNING: GSR - Correct "
                                   "test rig not detected\r\n");
      }
      else
      {
        ShimFactoryTest_sendReport(
            " - S3R_TEST_0025 - GSR test not applicable for this model\r\n");
      }
    }
    else
    {
      ShimFactoryTest_sendReport(
          " - S3R_TEST_0024 - FAIL: I2C4 - test rig not recognised\r\n");
      ShimFactoryTest_sendReport(
          " - S3R_TEST_0025 - FAIL: GSR - refer to S3R_TEST_0024\r\n");
      i2c4_result = 1;
    }

    enableI2cOnSr48OrSr38PpgSocket(0);

    if (i2c4_result)
    {
      shimmerStatus.testResult |= S3R_TEST_0024;
    }
  }
  else
  {
    ShimFactoryTest_sendReport(
        " - S3R_TEST_0024 - I2C4 test not applicable for this model\r\n");
    ShimFactoryTest_sendReport(
        " - S3R_TEST_0025 - GSR test not applicable for this model\r\n");
  }
#endif
}

void SPI_test(void)
{
  self_test_result_t self_test_result = SELF_TEST_PASS;
  float_t tempCal = 0;

#if defined(SHIMMER3R)
  ShimFactoryTest_sendReport("SPI1:\r\n");
  MX_SPI1_Init();

  if (ShimBrd_isBoardSr48_6_0())
  {
    sprintf(buffer, " - S3R_TEST_0015 - ADS7028 test not applicable for this model\r\n");
    ShimFactoryTest_sendReport(buffer);
  }
  else
  {
    self_test_result = ads7028_factoryTestChipId();
    if (self_test_result)
    {
      shimmerStatus.testResult |= S3R_TEST_0015;
    }
    print_chip_test_result("S3R_TEST_0015", "ADS7028", self_test_result,
        TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID);
  }

  tempCal = TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID;
  self_test_result = lsm6dsv_self_test();
  if (self_test_result == SELF_TEST_PASS)
  {
    //Get last temperature value left over from self test
    self_test_result = lsm6dsv_temperature_get(&tempCal);
    if (self_test_result || is_temperature_outside_of_range(tempCal))
    {
      self_test_result = SELF_TEST_FAIL_TEMPERATURE_ISSUE;
    }
  }
  if (self_test_result)
  {
    shimmerStatus.testResult |= S3R_TEST_0016;
  }
  print_chip_test_result("S3R_TEST_0016", "LSM6DSV", self_test_result, tempCal);

  tempCal = TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID;
  self_test_result = bmp3_self_test();
  if (self_test_result == SELF_TEST_PASS)
  {
    //Self test passed, now check temperature is reasonable
    struct bmp3_data *bmp3_data = (struct bmp3_data *) get_bmp3_selftest_data();
    tempCal = bmp3_data->temperature;

    if (tempCal <= TEST_THRESHOLD_DEG_IMU_TEMPERATURE_LOWER
        || tempCal >= TEST_THRESHOLD_DEG_IMU_TEMPERATURE_UPPER)
    {
      self_test_result = SELF_TEST_FAIL_TEMPERATURE_ISSUE;
    }
  }

  /* If it's a Shimmer self-test result (i.e., <SELF_TEST_FAIL_COUNT), it will
   *  be printed out like all other sensors using "print_chip_test_result".
   *  Else, if it's specific to the BMP3 API, it is printed using
   *  bmp3_check_rslt (subtracting the previously added offset from first so
   *  that the function can recognise it) */
  if (self_test_result < SELF_TEST_FAIL_COUNT)
  {
    print_chip_test_result("S3R_TEST_0017", "BMP390", self_test_result, tempCal);
  }
  else
  {
    ShimFactoryTest_sendReport(" - S3R_TEST_0017 - FAIL: BMP390 - ");

    bmp3_check_rslt("BMP390", ((int8_t) self_test_result) - BMP390_API_ERROR_OFFSET, buffer);
    ShimFactoryTest_sendReport(buffer);
  }
  if (self_test_result != SELF_TEST_PASS)
  {
    shimmerStatus.testResult |= S3R_TEST_0017;
  }

  if (ShimBrd_isAdxl371Present())
  {
    self_test_result = adxl371_self_test();
    print_chip_test_result("S3R_TEST_0018", "ADXL371", self_test_result,
        TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID);
    if (self_test_result)
    {
      shimmerStatus.testResult |= S3R_TEST_0018;
    }
  }
  else
  {
    ShimFactoryTest_sendReport(
        " - S3R_TEST_0018 - ADXL371 test not applicable for this model\r\n");
  }

  SPI1_DeInit();

  ShimFactoryTest_sendReport("SPI2:\r\n");
  MX_SPI2_Init();

  if (ShimBrd_isLis3mdlPresent())
  {
    tempCal = TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID;
    self_test_result = lis3mdl_self_test();
    if (self_test_result == SELF_TEST_PASS)
    {
      //Get new temperature value
      self_test_result = lis3mdl_temperature_get(&tempCal);
      if (self_test_result || is_temperature_outside_of_range(tempCal))
      {
        self_test_result = SELF_TEST_FAIL_TEMPERATURE_ISSUE;
      }
    }
    print_chip_test_result("S3R_TEST_0019", "LIS3MDL", self_test_result, tempCal);
    if (self_test_result)
    {
      shimmerStatus.testResult |= S3R_TEST_0019;
    }
  }
  else
  {
    ShimFactoryTest_sendReport(
        " - S3R_TEST_0019 - LIS3MDL test not applicable for this model\r\n");
  }

  tempCal = TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID;
  self_test_result = lis2dw12_self_test();
  if (self_test_result == SELF_TEST_PASS)
  {
    //Get last temperature value left over from self test
    self_test_result = lis2dw12_temperature_get(&tempCal);
    if (self_test_result || is_temperature_outside_of_range(tempCal))
    {
      self_test_result = SELF_TEST_FAIL_TEMPERATURE_ISSUE;
    }
  }
  print_chip_test_result("S3R_TEST_0020", "LIS2DW12", self_test_result, tempCal);
  if (self_test_result)
  {
    shimmerStatus.testResult |= S3R_TEST_0020;
  }

  SPI2_DeInit();

#endif

  ShimFactoryTest_sendReport("SPI3:\r\n");
  if (ShimBrd_isAds1292Present())
  {
    uint8_t ads1292RInitResult = 0;
    ADS1292_init();
    MX_SPI3_Init();
    ads1292RInitResult = EXG_init(&hspi3);
    if (ads1292RInitResult)
    {
      sprintf(buffer, " - S3R_TEST_0021 - FAIL: ADS1292R Init fail\r\n");
      ShimFactoryTest_sendReport(buffer);
    }
    else
    {
      uint8_t ads1292RTestResult = EXG_self_test();
      sprintf(buffer, " - S3R_TEST_0021 - %s: ADS1292R Chip1 detect\r\n",
          (ads1292RTestResult & 0x01) ? "FAIL" : "PASS");
      ShimFactoryTest_sendReport(buffer);
      sprintf(buffer, " - S3R_TEST_0021 - %s: ADS1292R Chip2 detect\r\n",
          (ads1292RTestResult & 0x02) ? "FAIL" : "PASS");
      ShimFactoryTest_sendReport(buffer);
      if (ads1292RTestResult)
      {
        shimmerStatus.testResult |= S3R_TEST_0021;
      }
    }
    EXG_powerOff();
    SPI3_DeInit();
  }
  else
  {
    ShimFactoryTest_sendReport(
        " - S3R_TEST_0021 - ADS1292R test not applicable for this model\r\n");
  }
}

uint8_t is_temperature_outside_of_range(float_t temperature)
{
  return (temperature < TEST_THRESHOLD_DEG_IMU_TEMPERATURE_LOWER
      || temperature > TEST_THRESHOLD_DEG_IMU_TEMPERATURE_UPPER);
}

void print_chip_test_result(char *testId, char *chipId, self_test_result_t self_test_result, float_t tempCal)
{
  char *selfTestResultStr;
  char *selfTestDetailsStr;
  if (self_test_result == SELF_TEST_PASS)
  {
    selfTestResultStr = &SELF_TEST_STR_PASS[0];
    selfTestDetailsStr = &SELF_TEST_STR_EMPTY[0];
  }
  else
  {
    selfTestResultStr = &SELF_TEST_STR_FAIL[0];
    if (self_test_result == SELF_TEST_FAIL_CHIP_DETECTION)
    {
      selfTestDetailsStr = &SELF_TEST_STR_CHIP_DETECTION[0];
    }
    else if (self_test_result == SELF_TEST_FAIL_SIGNAL_ISSUE)
    {
      selfTestDetailsStr = &SELF_TEST_STR_SIGNAL_ISSUE[0];
    }
    else if (self_test_result == SELF_TEST_FAIL_TEMPERATURE_ISSUE)
    {
      selfTestDetailsStr = &SELF_TEST_STR_TEMPERATURE_ISSUE[0];
    }
    else if (self_test_result == SELF_TEST_FAIL_DRDY_ISSUE)
    {
      selfTestDetailsStr = &SELF_TEST_STR_DRDY_ISSUE[0];
    }
    else
    {
      selfTestDetailsStr = &SELF_TEST_STR_UNKNOWN[0];
    }
  }

  if (tempCal == TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID)
  {
    sprintf(buffer, " - %s - %s: %s%s\r\n", testId, selfTestResultStr, chipId, selfTestDetailsStr);
  }
  else
  {
    sprintf(buffer, " - %s - %s: %s%s (%.2f\xC2\xB0 C)\r\n", testId,
        selfTestResultStr, chipId, selfTestDetailsStr, tempCal);
  }
  ShimFactoryTest_sendReport(buffer);
}

void ShimFactoryTest_sendReportImpl(const char *str, factory_test_target_t factoryTestTarget)
{
  switch (factoryTestTarget)
  {
  case PRINT_TO_DEBUGGER:
    SHIMMER_PRINTF(str);
    break;
  case PRINT_TO_DOCK_UART:
    if (shimmerStatus.usbPluggedIn)
    {

    }
    else if (shimmerStatus.docked)
    {
      DockUart_writeBlocking((uint8_t *) str, strlen(str));
    }
    break;
  case PRINT_TO_BT_UART:
    ShimBt_writeToTxBufAndSend((uint8_t *) str, strlen(str), SHIMMER_CMD);
    //wait for msg to finish transmitting
    while (ShimBt_getUsedSpaceInBtTxBuf() > 0)
    {
      HAL_Delay(100);
    }
    break;
  default:
    break;
  }
}

uint8_t gsrFactoryTest_run(void)
{
  uint8_t returnVal = 0;
  uint8_t i = 0;
  HAL_StatusTypeDef status;

#if SUPPORT_SR48_6_0
  if (ShimBrd_isBoardSr48_6_0())
  {
    Board_SR48_6_0_SW_GSR(1);
  }
#endif
  status = gsrTestRigInit(&hi2c4);
  if (status != HAL_OK)
  {
    //ShimFactoryTest_sendReport("Failed to initialise GSR test rig\r\n");
    return returnVal;
  }

  GSR_setActiveResistor(HW_RES_40K);
  //Setup ADC for GSR readings
#if SUPPORT_SR48_6_0
  if (ShimBrd_isBoardSr48_6_0())
  {
    initGsrMcuAdc();
  }
  else
  {
    ads7028_factoryTestGsrInit();
  }
#else
  ads7028_factoryTestGsrInit();
#endif

  gsrResistance[0] = 0xFF;

  float passTolerance;
  for (i = 0; i < sizeof(testGsrResistances) / sizeof(testGsrResistances[0]); i++)
  {
    status = setGsrTestRigResistance(testGsrResistances[i]);
    if (status != HAL_OK)
    {
      //sprintf(buffer, "Failed to set GSR test rig resistance %lu\r\n",
      //    testGsrResistances[i]);
      //ShimFactoryTest_sendReport(buffer);
      return returnVal;
    }
    HAL_Delay(100);

    status = gsrFactoryTest_getAvgGsr(&gsrResistance[i]);

    passTolerance
        = gsrFactoryTest_getPassToleranceForTestResistor(testGsrResistances[i]);
    uint32_t gsrBuffer = testGsrResistances[i] * passTolerance;
    if (status != HAL_OK || (gsrResistance[i] < (testGsrResistances[i] - gsrBuffer))
        || (gsrResistance[i] > (testGsrResistances[i] + gsrBuffer)))
    {
      returnVal = 1;
    }
  }

  GSR_resetGsrRange();

  //Deinit ADC for GSR readings
#if SUPPORT_SR48_6_0
  if (ShimBrd_isBoardSr48_6_0())
  {
    //Stop ADC
    deinitGsrMcuAdc();
    Board_SR48_6_0_SW_GSR(0);
  }
  else
  {
    /*Only deinit SPI1 if not using I2C controlled by ADC chip. Otherwise SPI1 will be deinitialised later in the test flow.*/
    if (!ShimBrd_isI2cOnPPGControlledByAdcChip())
    {
      SPI1_DeInit();
    }
  }
#else
  /*Only deinit SPI1 if not using I2C controlled by ADC chip. Otherwise SPI1 will be deinitialised later in the test flow.*/
  if (!ShimBrd_isI2cOnPPGControlledByAdcChip())
  {
    SPI1_DeInit();
  }
#endif

  return returnVal;
}

void gsrFactoryTest_printResults(void)
{
  uint8_t i = 0;
  uint8_t returnVal = 0;
  uint32_t referenceResistor = 0;
  float passTolerance = 0.0;

  if (gsrResistance[0] != 0xFF)
  {
    ShimFactoryTest_sendReport("\r\n    - GSR Test Results:\r\n");
    ShimFactoryTest_sendReport(
        "      - Source, Measured, Pass Tolerance, Measured "
        "Tolerance, Ref Resistor, Result\r\n");
    for (i = 0; i < sizeof(testGsrResistances) / sizeof(testGsrResistances[0]); i++)
    {
      returnVal = 0;

      referenceResistor
          = gsrFactoryTest_getRefResistorForTestResistor(testGsrResistances[i]);
      passTolerance
          = gsrFactoryTest_getPassToleranceForTestResistor(testGsrResistances[i]);

      uint32_t gsrBuffer = testGsrResistances[i] * passTolerance;
      if ((gsrResistance[i] < (testGsrResistances[i] - gsrBuffer))
          || (gsrResistance[i] > (testGsrResistances[i] + gsrBuffer)))
      {
        returnVal = 1;
      }

      float measured_tolerance
          = (((float) gsrResistance[i] - (float) testGsrResistances[i]) * 100.0f)
          / (float) testGsrResistances[i];

      sprintf(buffer, "      - %lu ohms, %lu ohms, +-%.0f%%, %+.02f%%, %lu ohms, %s\r\n",
          testGsrResistances[i], gsrResistance[i], passTolerance * 100.0f,
          measured_tolerance, referenceResistor, returnVal ? "FAIL" : "PASS");
      ShimFactoryTest_sendReport(buffer);
    }
  }
}

static float gsrFactoryTest_getPassToleranceForTestResistor(uint32_t testResistor)
{
  if (testResistor > 8000L && testResistor < 63000L)
  {
    return GSR_TEST_TOLERANCE_8_PERCENT;
  }
  else if (testResistor >= 63000L && testResistor < 220000L)
  {
    return GSR_TEST_TOLERANCE_7_PERCENT;
  }
  else if (testResistor >= 220000L && testResistor < 680000L)
  {
    return GSR_TEST_TOLERANCE_7_PERCENT;
  }
  else
  {
    return GSR_TEST_TOLERANCE_7_PERCENT;
  }
}

static uint32_t gsrFactoryTest_getRefResistorForTestResistor(uint32_t testResistor)
{
  if (testResistor > 8000L && testResistor < 63000L)
  {
    return 40000;
  }
  else if (testResistor >= 63000L && testResistor < 220000L)
  {
    return 287000;
  }
  else if (testResistor >= 220000L && testResistor < 680000L)
  {
    return 1000000;
  }
  else
  {
    return 3300000;
  }
}

HAL_StatusTypeDef gsrFactoryTest_getAvgGsr(uint32_t *gsrResistance)
{
  HAL_StatusTypeDef status;
  uint32_t gsrResistanceAvg = 0;

  for (uint8_t i = 0; i < 13; i++)
  {
#if SUPPORT_SR48_6_0
    if (ShimBrd_isBoardSr48_6_0())
    {
      status = getFactoryTestGsrResistanceMcuAdc(gsrResistance);
    }
    else
    {
      status = ads7028_factoryTestGetGsrResistance(gsrResistance);
    }
#else
    status = ads7028_factoryTestGetGsrResistance(gsrResistance);
#endif

    //Skip first 3 measurements to account for range changing
    if (i > 2)
    {
      gsrResistanceAvg += *gsrResistance;
    }

    /* TODO Figure out why delay is needed at all */
    HAL_Delay(15);
  }

  *gsrResistance = gsrResistanceAvg / 10;
  return status;
}

uint8_t runMicrophoneTest(void)
{
  uint8_t self_test_result = SELF_TEST_PASS;
  uint8_t micResult;

  ShimFactoryTest_sendReport("Microphone:\r\n");

  micResult = micTest();

  ShimFactoryTest_sendReport(" - S3R_TEST_0026 - ");
  const micTestResult_t *result = getMicTestResult(micResult);
  ShimFactoryTest_sendReport(result->message);
  self_test_result = result->selfTestResult;
  ShimFactoryTest_sendReport("\r\n");

  if (self_test_result != SELF_TEST_PASS)
  {
    shimmerStatus.testResult |= S3R_TEST_0026;
  }

  return self_test_result;
}

const micTestResult_t *getMicTestResult(uint8_t micResult)
{
  static const micTestResult_t results[] = { [FACTORY_TEST_MIC_PASS] = { "PASS", SELF_TEST_PASS },
    [FACTORY_TEST_MIC_FAIL_NO_DATA_IN_BUFFER]
    = { "FAIL: Test buffer is empty", SELF_TEST_FAIL_SIGNAL_ISSUE },
    [MDF_ERROR_ACQUISITION_OVERFLOW]
    = { "FAIL: Acquisition Overflow Error", SELF_TEST_FAIL_CHIP_DETECTION },
    [MDF_ERROR_RSF_OVERRUN] = { "FAIL: RSF Overrun Error", SELF_TEST_FAIL_CHIP_DETECTION },
    [MDF_ERROR_CLOCK_ABSENCE] = { "FAIL: Clock Absence Error", SELF_TEST_FAIL_CHIP_DETECTION },
    [MDF_ERROR_SATURATION] = { "FAIL: Saturation Error", SELF_TEST_FAIL_CHIP_DETECTION },
    [MDF_ERROR_DMA] = { "FAIL: DMA", SELF_TEST_FAIL_CHIP_DETECTION } };
  static const micTestResult_t unknown
      = { "FAIL: Unknown Error Code", SELF_TEST_FAIL_CHIP_DETECTION };

  if (micResult < sizeof(results) / sizeof(results[0]) && results[micResult].message != NULL)
  {
    return &results[micResult];
  }
  return &unknown;
}
