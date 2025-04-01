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

#include "BMP3/BMP3_SensorAPI/self-test/bmp3_selftest.h"
#include "PCM/pcm_config.h"
#include "bmp3_defs.h"
#include "usbd_cdc_acm_if.h"

factory_test_target_t factoryTestTarget = PRINT_TO_DEBUGGER;
factory_test_t factoryTestToRun;

char buffer[100];

uint8_t test_i2c_addr_list[128], test_i2c_addr_list_len;

static uint32_t testGsrResistances[] = { 12000L, 48800L, //GSR Range 0 (8.0kOhm-63.0kOhm)
  76000L, 197000L,                //GSR Range 1 (63.0kOhm-220.0kOhm)
  248000L, 498000L,               //GSR Range 2 (220.0kOhm-680.0kOhm)
  2000000L, 3000000L, 4000000L }; //GSR Range 3 (680.0kOhm-4.7MOhm)

uint32_t run_factory_test(void)
{
  send_test_report("//**************************** TEST START "
                   "************************************//\r\n");

  shimmerStatus.testResult = 0;

  if (factoryTestToRun == FACTORY_TEST_MAIN || factoryTestToRun == FACTORY_TEST_ICS)
  {
    print_date_and_time();
    send_test_report("\r\n");

    sprintf(buffer, "INFO: Temperature pass range set to %.0f-%.0f\xC2\xB0 C\r\n",
        TEST_THRESHOLD_DEG_IMU_TEMPERATURE_LOWER, TEST_THRESHOLD_DEG_IMU_TEMPERATURE_UPPER);
    send_test_report(buffer);
    send_test_report("\r\n");

    print_shimmer_model();
    send_test_report("\r\n");

    //Sensing power is needed for ADC peripheral and I2C/SPI tests
    Board_enableSensingPower(SENSE_PWR_FACTORY_TEST, 1);

    print_mcu_details();
    send_test_report("\r\n");

    print_battery_details();
    send_test_report("\r\n");

    sd_card_test();
    send_test_report("\r\n");

    bt_module_test();
    send_test_report("\r\n");

    //InfoMem_test();

    I2C_test();
    send_test_report("\r\n");

    SPI_test();
    send_test_report("\r\n");

    Board_enableSensingPower(SENSE_PWR_FACTORY_TEST, 0);

    runMicrophoneTest();
  }

  if (factoryTestToRun == FACTORY_TEST_MAIN || factoryTestToRun == FACTORY_TEST_LEDS)
  {
    if (factoryTestToRun == FACTORY_TEST_MAIN)
    {
      send_test_report("\r\n");
    }
    led_test();
  }

  if (factoryTestToRun == FACTORY_TEST_MAIN || factoryTestToRun == FACTORY_TEST_ICS)
  {
    if (shimmerStatus.testResult)
    {
      sprintf(buffer, "\r\nOverall Result = FAIL (0x%08" PRIX32 ")\r\n",
          shimmerStatus.testResult);
    }
    else
    {
      sprintf(buffer, "\r\nOverall Result = PASS\r\n");
    }
    send_test_report(buffer);
  }

  send_test_report("//***************************** TEST END "
                   "*************************************//\r\n");

  return shimmerStatus.testResult;
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
  send_test_report(buffer);
  sprintf(buffer, "Time (hh:mm:ss): %.2u:%.2u:%.2u (UTC)\r\n", sTime.Hours,
      sTime.Minutes, sTime.Seconds);
  send_test_report(buffer);
}

void print_shimmer_model(void)
{
  send_test_report("Shimmer model:\r\n");
  if (isDaughterCardIdSet())
  {
    sprintf(buffer, " - S3R_TEST_0003 - PASS: %s", getDaughtCardIdStrPtr());
    send_test_report(buffer);
    shimmer_expansion_brd *daughterCardId = getDaughtCardId();
    sprintf(buffer, " (SR%d-%d-%d)\r\n", daughterCardId->exp_brd_id,
        daughterCardId->exp_brd_rev, daughterCardId->exp_brd_special_rev);
    send_test_report(buffer);
  }
  else
  {
    send_test_report(" - S3R_TEST_0003 - FAIL: not set\r\n");
    shimmerStatus.testResult |= S3R_TEST_0003;
  }
}

void print_mcu_details(void)
{
  send_test_report("MCU:\r\n");

  sprintf(buffer, " - Device ID = %lu\r\n", HAL_GetDEVID());
  send_test_report(buffer);
  sprintf(buffer, " - Revision ID = %lu\r\n", HAL_GetREVID());
  send_test_report(buffer);

  /*
   * UID[31:0]: X and Y coordinates on the wafer expressed in BCD format
   * UID[63:40]: LOT_NUM[23:0] Lot number (ASCII encoded)
   * UID[39:32]: WAF_NUM[7:0] Wafer number (8-bit unsigned number)
   * UID[95:64]: LOT_NUM[55:24] Lot number (ASCII encoded)
   * */
  //sprintf(buffer, " - Unique ID w0 = 0x%08X\r\n", HAL_GetUIDw0());
  //send_test_report(buffer);
  //sprintf(buffer, " - Unique ID w1 = 0x%08X\r\n", HAL_GetUIDw1());
  //send_test_report(buffer);
  //sprintf(buffer, " - Unique ID w2 = 0x%08X\r\n", HAL_GetUIDw2());
  //send_test_report(buffer);
  //sprintf(buffer, " - Unique ID = 0x%08X%08X%08X\r\n", HAL_GetUIDw0(),
  //    HAL_GetUIDw1(), HAL_GetUIDw2());
  sprintf(buffer, " - Unique ID = 0x%08" PRIX32 "%08" PRIX32 "%08" PRIX32 "\r\n",
      HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());
  send_test_report(buffer);

  ADCDebugInfo_t adcDebugInfo;
  getherMcuDebugInfo(&adcDebugInfo);

  uint8_t testPass = (adcDebugInfo.vRefMV > TEST_THRESHOLD_VREF_LOWER
      && adcDebugInfo.vRefMV < TEST_THRESHOLD_VREF_UPPER);
  sprintf(buffer, " - S3R_TEST_0007 - %s: VRef = %ldmV (%d-%dmV)\r\n",
      testPass ? "PASS" : "FAIL", adcDebugInfo.vRefMV,
      TEST_THRESHOLD_VREF_LOWER, TEST_THRESHOLD_VREF_UPPER);
  send_test_report(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0007;
  }

  testPass = (adcDebugInfo.vCoreMV > TEST_THRESHOLD_MV_VCORE_LOWER
      && adcDebugInfo.vCoreMV < TEST_THRESHOLD_MV_VCORE_UPPER);
  sprintf(buffer, " - S3R_TEST_0008 - %s: VCore = %ldmV (%d-%dmV)\r\n",
      testPass ? "PASS" : "FAIL", adcDebugInfo.vCoreMV,
      TEST_THRESHOLD_MV_VCORE_LOWER, TEST_THRESHOLD_MV_VCORE_UPPER);
  send_test_report(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0008;
  }

  //Specification = 1.9V from voltage external regulator
  testPass = (adcDebugInfo.vBattPinMV > TEST_THRESHOLD_MV_VBATT_PIN_LOWER
      && adcDebugInfo.vBattPinMV < TEST_THRESHOLD_MV_VBATT_PIN_UPPER);
  sprintf(buffer, " - S3R_TEST_0009 - %s: VBatt pin = %ldmV (%d-%dmV)\r\n",
      testPass ? "PASS" : "FAIL", adcDebugInfo.vBattPinMV,
      TEST_THRESHOLD_MV_VBATT_PIN_LOWER, TEST_THRESHOLD_MV_VBATT_PIN_UPPER);
  send_test_report(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0009;
  }

  testPass = (adcDebugInfo.temperature > TEST_THRESHOLD_MV_MCU_TEMPERATURE_LOWER
      && adcDebugInfo.temperature < TEST_THRESHOLD_MV_MCU_TEMPERATURE_UPPER);
  sprintf(buffer, " - S3R_TEST_0010 - %s: Temperature = %ld\xC2\xB0 C\r\n",
      testPass ? "PASS" : "FAIL", adcDebugInfo.temperature);
  send_test_report(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0010;
  }

  send_test_report(" - I/O status:\r\n");
  sprintf(buffer, "    - Docked: %s\r\n", shimmerStatus.docked ? "Yes" : "No");
  send_test_report(buffer);
  sprintf(buffer, "    - BT connected: %s\r\n", shimmerStatus.btConnected ? "Yes" : "No");
  send_test_report(buffer);
  sprintf(buffer, "    - Button pressed: %s\r\n", shimmerStatus.buttonPressed ? "Yes" : "No");
  send_test_report(buffer);
  GPIO_PinState pin = HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin);
  sprintf(buffer, "    - USB connected: %s\r\n", pin == GPIO_PIN_SET ? "Yes" : "No");
  send_test_report(buffer);
}

void print_battery_details(void)
{
  send_test_report("Battery:\r\n");
  manageReadBatt(1);

  uint8_t testPass = (batteryStatus.battValMV > TEST_THRESHOLD_MV_VBATT_LOWER
      && batteryStatus.battValMV < TEST_THRESHOLD_MV_VBATT_UPPER);
  sprintf(buffer, " - S3R_TEST_0011 - %s: VBatt = %dmV (%d-%dmV)\r\n",
      testPass ? "PASS" : "FAIL", batteryStatus.battValMV,
      TEST_THRESHOLD_MV_VBATT_LOWER, TEST_THRESHOLD_MV_VBATT_UPPER);
  send_test_report(buffer);
  if (!testPass)
  {
    shimmerStatus.testResult |= S3R_TEST_0011;
  }

  testPass = batteryStatus.battStatusRaw.rawBytes[2] == CHRG_CHIP_STATUS_BAD_BATTERY ? 0 : 1;
  sprintf(buffer, " - S3R_TEST_0012 - %s: Charger chip status = ", testPass ? "PASS" : "FAIL");
  send_test_report(buffer);
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
  send_test_report(buffer);

  send_test_report(" - Determined charging status = ");
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
  send_test_report(buffer);
}

void led_test(void)
{
  send_test_report("LED test (S3R_TEST_0027):\r\n");

#if defined(SHIMMER3R)
  stopLedBlinkTimer();

  Board_ledLwrSetColour(LED_RGB_ALL_OFF);
  Board_ledUprSetColour(LED_RGB_ALL_OFF);
  send_test_report(" - All LEDs off\r\n");
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);

  send_test_report(" - Lower Red LED on\r\n");
  Board_ledLwrSetColour(LED_RGB_RED);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  send_test_report(" - Lower Green LED on\r\n");
  Board_ledLwrSetColour(LED_RGB_GREEN);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  send_test_report(" - Lower Blue LED on\r\n");
  Board_ledLwrSetColour(LED_RGB_BLUE);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  Board_ledLwrSetColour(LED_RGB_ALL_OFF);

  send_test_report(" - Upper Red LED on\r\n");
  Board_ledUprSetColour(LED_RGB_RED);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  send_test_report(" - Upper Green LED on\r\n");
  Board_ledUprSetColour(LED_RGB_GREEN);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  send_test_report(" - Upper Blue LED on\r\n");
  Board_ledUprSetColour(LED_RGB_BLUE);
  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  send_test_report(" - All LEDs off\r\n");
  Board_ledUprSetColour(LED_RGB_ALL_OFF);

  HAL_Delay(DELAY_BETWEEN_LED_CHANGES_MS);
  send_test_report(" - All LEDs on\r\n");
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
  send_test_report("SD Card:\r\n");
  if (!shimmerStatus.sdInserted)
  {
    send_test_report(" - S3R_TEST_0013 - FAIL: not detected\r\n");
    shimmerStatus.testResult |= S3R_TEST_0013;
  }
  else
  {
    if (shimmerStatus.docked)
    {
      Board_sd2Arm();
      HAL_Delay(120); //120ms
    }

    send_test_report(" - ");
    printSdCardInfo(buffer);
    send_test_report(buffer);

    shimmerStatus.testResult += SD_test() << 6;
    //SD_test_alternative();
    sprintf(buffer, " - S3R_TEST_0013 - %s: MCU read/write test\r\n",
        shimmerStatus.sdBadFile ? "FAIL" : "PASS");
    send_test_report(buffer);

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
  send_test_report("BT Module:\r\n");
  if (isBtIsInitialised())
  {
    send_test_report(" - MAC ID: ");
    BT_getMacAddressAscii(buffer);
    send_test_report(buffer);
    send_test_report("\r\n");

    sprintf(buffer, " - %s\r\n", getBtVerStrPtr());
    send_test_report(buffer);

    sprintf(buffer, " - S3R_TEST_0014 - %s BT firmware version\r\n",
        strstr(buffer, TEST_BT_MODULE_FW) != NULL ? "PASS: Correct" : "FAIL: Incorrect");
    send_test_report(buffer);
  }
  else
  {
    send_test_report(" - S3R_TEST_0014 - FAIL - BT hasn't initialised\r\n");
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
  send_test_report("I2C1:\r\n");

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
    shimmerStatus.testResult |= S3R_TEST_0015;
  }
  print_chip_test_result("S3R_TEST_0015", "LIS2MDL", self_test_result, tempCal);

  uint8_t eeprom_result = eepromTest();
  sprintf(buffer, " - S3R_TEST_0016 - %s: CAT24C16\r\n", eeprom_result ? "FAIL" : "PASS");
  send_test_report(buffer);
  if (eeprom_result)
  {
    shimmerStatus.testResult |= S3R_TEST_0016;
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
  send_test_report("I2C4:\r\n");
  if (isI2c4Supported())
  {
    uint8_t i2c4_result = 0;

    enableI2cOnInternalExpansionBrd(1);
    HAL_Delay(2); //2ms as per Shimmer3 code
    I2C_scan_internal_expansion_bus(test_i2c_addr_list, &test_i2c_addr_list_len);

    if (test_i2c_addr_list_len == 0)
    {
      send_test_report(
          " - S3R_TEST_0017 - FAIL: I2C4 - no test rig detected\r\n");
      send_test_report(
          " - S3R_TEST_0018 - FAIL: GSR - no test rig detected\r\n");
      i2c4_result = 1;
    }
    else if (test_i2c_addr_list_len == 3)
    {
      send_test_report(" - S3R_TEST_0017 - PASS: I2C4\r\n");

      uint8_t gsr_result = runGsrFactoryTest();
      sprintf(buffer, " - S3R_TEST_0018 - %s: GSR signal test\r\n",
          gsr_result ? "FAIL" : "PASS");
      send_test_report(buffer);

      if (gsr_result)
      {
        shimmerStatus.testResult |= S3R_TEST_0018;
      }
    }
    else if (test_i2c_addr_list_len == 8)
    {
      altEepromInit(&hi2c4);
      i2c4_result = altEepromTest();
      HAL_Delay(5); //5ms to ensure no writes pending

      sprintf(buffer, " - S3R_TEST_0017 - %s: I2C4\r\n", i2c4_result ? "FAIL" : "PASS");
      send_test_report(buffer);
      send_test_report(
          " - S3R_TEST_0018 - WARNING: GSR - no test rig detected\r\n");
    }
    else
    {
      send_test_report(
          " - S3R_TEST_0017 - FAIL: I2C4 - test rig not recognised\r\n");
      send_test_report(
          " - S3R_TEST_0018 - FAIL: GSR - test rig not recognised\r\n");
      i2c4_result = 1;
    }

    enableI2cOnInternalExpansionBrd(0);

    if (i2c4_result)
    {
      shimmerStatus.testResult |= S3R_TEST_0017;
    }
  }
  else
  {
    send_test_report(
        " - S3R_TEST_0017 - I2C4 test not applicable for this model\r\n");
  }
#endif
}

void SPI_test(void)
{
  self_test_result_t self_test_result = SELF_TEST_PASS;
  float_t tempCal = 0;

#if defined(SHIMMER3R)
  send_test_report("SPI1:\r\n");
  MX_SPI1_Init();

  if (isBoardSr48_6_0())
  {
    sprintf(buffer, " - S3R_TEST_0019 - ADS7028 test not applicable for this model\r\n");
    send_test_report(buffer);
  }
  else
  {
    self_test_result = SELF_TEST_FAIL_CHIP_DETECTION;
    if (self_test_result)
    {
      shimmerStatus.testResult |= S3R_TEST_0019;
    }
    print_chip_test_result("S3R_TEST_0019", "ADS7028", self_test_result,
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
    shimmerStatus.testResult |= S3R_TEST_0020;
  }
  print_chip_test_result("S3R_TEST_0020", "LSM6DSV", self_test_result, tempCal);

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
    print_chip_test_result("S3R_TEST_0021", "BMP390", self_test_result, tempCal);
  }
  else
  {
    send_test_report(" - S3R_TEST_0021 - FAIL: BMP390 - ");

    bmp3_check_rslt("BMP390", ((int8_t) self_test_result) - BMP390_API_ERROR_OFFSET, buffer);
    send_test_report(buffer);
  }
  if (self_test_result != SELF_TEST_PASS)
  {
    shimmerStatus.testResult |= S3R_TEST_0021;
  }

  if (isAdxl371Present())
  {
    self_test_result = adxl371_self_test();
    print_chip_test_result("S3R_TEST_0022", "ADXL371", self_test_result,
        TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID);
    if (self_test_result)
    {
      shimmerStatus.testResult |= S3R_TEST_0022;
    }
  }
  else
  {
    sprintf(buffer, " - S3R_TEST_0022 - ADXL371 test not applicable for this model\r\n");
    send_test_report(buffer);
  }

  SPI1_DeInit();

  send_test_report("SPI2:\r\n");
  MX_SPI2_Init();

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
  print_chip_test_result("S3R_TEST_0023", "LIS3MDL", self_test_result, tempCal);
  if (self_test_result)
  {
    shimmerStatus.testResult |= S3R_TEST_0023;
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
  print_chip_test_result("S3R_TEST_0024", "LIS2DW12", self_test_result, tempCal);
  if (self_test_result)
  {
    shimmerStatus.testResult |= S3R_TEST_0024;
  }

  SPI2_DeInit();

#endif

  send_test_report("SPI3:\r\n");
  if (isAds1292Present())
  {
    MX_SPI3_Init();

    //EXG_init(hspiExg);
    //ret_val |= EXG_test();

    send_test_report(
        " - S3R_TEST_0025 - WARNING: ADS1292R test not implemented yet\r\n");
    //if (self_test_result)
    //{
    //  shimmerStatus.testResult |= S3R_TEST_0025;
    //}

    SPI3_DeInit();
  }
  else
  {
    send_test_report(
        " - S3R_TEST_0025 - ADS1292R test not applicable for this model\r\n");
  }
}

void setup_factory_test(factory_test_target_t target, factory_test_t testToRun)
{
  factoryTestTarget = target;
  factoryTestToRun = testToRun;
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
  send_test_report(buffer);
}

void send_test_report(char *str)
{
  switch (factoryTestTarget)
  {
  case PRINT_TO_DEBUGGER:
    SHIMMER_PRINTF(str);
    break;
  case PRINT_TO_DOCK_UART:
    if (shimmerStatus.usbPluggedIn)
    {
      CDC_Transmit(0, (uint8_t *) str, strlen(str));
    }
    else if (shimmerStatus.docked)
    {
      DockUart_writeBlocking((uint8_t *) str, strlen(str));
    }
    break;
  case PRINT_TO_BT_UART:
    BT_write((uint8_t *) str, strlen(str));
    //wait for msg to finish transmitting
    while (getUsedSpaceInBtTxBuf() > 0)
    {
      HAL_Delay(100);
    }
    break;
  default:
    break;
  }
}

uint8_t runGsrFactoryTest(void)
{
  uint32_t gsrResistance = 0;
  uint8_t returnVal = 0;
  uint8_t i = 0;
  HAL_StatusTypeDef status;

  Board_SW_GSR(1);
  gsrTestRigInit(&hi2c4);

  GSR_setRange(HW_RES_40K);
  initGsrAdc();
  ADC_HandleTypeDef *hadcFactoryTestPtr = getHadc2();

  for (i = 0; i < sizeof(testGsrResistances) / sizeof(testGsrResistances[0]); i++)
  {
    setGsrTestRigResistance(testGsrResistances[i]);
    HAL_Delay(100);

    status = getFactoryTestGsrAvg(hadcFactoryTestPtr, &gsrResistance);

    uint32_t buffer = gsrResistance * GSR_TEST_TOLERANCE;
    if (status != HAL_OK || (gsrResistance < (testGsrResistances[i] - buffer))
        || (gsrResistance > (testGsrResistances[i] + buffer)))
    {
      returnVal = 1;
      break;
    }
  }

  //Stop ADC
  HAL_ADC_Stop(hadcFactoryTestPtr);
  HAL_ADC_DeInit(hadcFactoryTestPtr);

  Board_SW_GSR(0);

  return returnVal;
}

uint8_t runMicrophoneTest(void)
{
  uint8_t self_test_result = SELF_TEST_PASS;

  send_test_report("Microphone:\r\n");

  self_test_result = micTest() ? SELF_TEST_FAIL_SIGNAL_ISSUE : SELF_TEST_PASS;

  sprintf(buffer, " - S3R_TEST_0026 - %s\r\n",
      self_test_result == SELF_TEST_PASS ? "PASS" : "FAIL: Test buffer is empty");
  send_test_report(buffer);

  if (self_test_result != SELF_TEST_PASS)
  {
    shimmerStatus.testResult |= S3R_TEST_0026;
  }

  return self_test_result;
}
