/*
 * hal_FactoryTest.c
 *
 *  Created on: Aug 14, 2024
 *      Author: MarkNolan
 */

#include "hal_FactoryTest.h"

#include "i2c.h"
#include "spi.h"

factory_test_target_t factoryTestTarget = PRINT_TO_DEBUGGER;
factory_test_t factoryTestToRun;

char buffer[100];

uint32_t run_factory_test(void)
{
  send_test_report("//**************************** TEST START "
                   "************************************//\r\n");

  if (factoryTestToRun == FACTORY_TEST_MAIN)
  {
    print_date_and_time();
    send_test_report("\r\n");

    print_shimmer_model();
    send_test_report("\r\n");
  }

  if (factoryTestToRun == FACTORY_TEST_MAIN
      || factoryTestToRun == FACTORY_TEST_LEDS)
  {
    led_test();
  }

  if (factoryTestToRun == FACTORY_TEST_MAIN)
  {
    send_test_report("\r\n");

    sd_card_test();
    send_test_report("\r\n");

    stat.testResult += (!bt_module_test()) << 7;
    send_test_report("\r\n");

    stat.testResult += InfoMem_test() << 8;

    stat.testResult += I2C_test();

    stat.testResult += SPI_test() << 16;
  }

  send_test_report("//***************************** TEST END "
                   "*************************************//\r\n");

  return stat.testResult;
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
  sprintf(buffer, "Time (hh:mm:ss): %.2u:%.2u:%.2u\r\n", sTime.Hours,
      sTime.Minutes, sTime.Seconds);
  send_test_report(buffer);
}

void print_shimmer_model(void)
{
  send_test_report("Shimmer model:\r\n");
  if (isDaughterCardIdSet())
  {
    sprintf(buffer, " - PASS: %s", getDaughtCardIdStrPtr());
    send_test_report(buffer);
    shimmer_expansion_brd *daughterCardId = getDaughtCardId();
    sprintf(buffer, " (SR%d-%d-%d)\r\n", daughterCardId->exp_brd_id,
        daughterCardId->exp_brd_rev, daughterCardId->exp_brd_special_rev);
    send_test_report(buffer);
  }
  else
  {
    send_test_report(" - FAIL: not set\r\n");
  }
}

void led_test(void)
{
  send_test_report("LED test:\r\n");

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
  if (!stat.isSdInserted)
  {
    send_test_report(" - FAIL: not detected\r\n");
  }
  else
  {
    send_test_report(" - ");
    printSdCardInfo(buffer);
    send_test_report(buffer);

    stat.testResult += SD_test() << 6;
    //SD_test_alternative();
    sprintf(buffer, " - %s: read/write test\r\n", stat.badFile ? "FAIL" : "PASS");
    send_test_report(buffer);
  }
}

uint8_t bt_module_test(void)
{
  if (stat.isBtPoweredOn)
  {
    send_test_report("BT Module:\r\n");

    send_test_report(" - MAC ID: ");
    BT_getMacAddressAscii(buffer);
    send_test_report(buffer);
    send_test_report("\r\n");

    sprintf(buffer, " - %s\r\n", getBtVerStrPtr());
    send_test_report(buffer);

    if (strstr(buffer, "v01.04.16.16") != NULL)
    {
      send_test_report(" - PASS\r\n");
    }
    else
    {
      send_test_report(" - FAIL: incorrect BT firmware version\r\n");
    }
  }
  else
  {
    send_test_report(" - FAIL\r\n");
  }
  return stat.isBtPoweredOn;
}

uint8_t I2C_test(void)
{
  uint8_t ret_val = 0;

  set_power_i2c_main_bus(1);
  HAL_Delay(50);

  I2C_scan_busses();

#if defined(SHIMMER4_SDK)
  MPU9250_init(hi2cMainBus);
  if (MPU9250_test())
    ret_val |= 0x01;

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
  send_test_report("I2C:\r\n");

  uint8_t st_result = lis2mdl_self_test();
  sprintf(buffer, " - %s: LIS2MDL\r\n", st_result ? "PASS" : "FAIL");
  send_test_report(buffer);

  uint8_t eeprom_result = CAT24C16_test();
  sprintf(buffer, " - %s: CAT24C16\r\n", eeprom_result ? "FAIL" : "PASS");
  send_test_report(buffer);
#endif

  set_power_i2c_main_bus(0);

#if defined(SHIMMER4_SDK)
  if (bmp280_test(hi2cMainBus))
  {
    ret_val |= 0x10;
  }
#endif

  return ret_val;
}

uint8_t SPI_test(void)
{
  uint8_t ret_val = 0;
#if defined(SHIMMER3R)
  send_test_report("SPI1:\r\n");
  set_power_spi1_bus(1, SPI1_CHIP_ALL);
  HAL_Delay(50);

  uint8_t lsm6dsv_result = lsm6dsv_self_test();
  sprintf(buffer, " - %s: LSM6DSV\r\n", lsm6dsv_result ? "FAIL" : "PASS");
  send_test_report(buffer);

  int8_t bmp390_result = bmp390_self_test();
  sprintf(buffer, " - %s: BMP390\r\n", bmp390_result ? "FAIL" : "PASS");
  send_test_report(buffer);
  if (bmp390_result)
  {
    send_test_report(" - ");
    bmp3_check_rslt("BMP390", bmp390_result, buffer);
    send_test_report(buffer);
  }

  if (isAdxl371Detected())
  {
    uint8_t adxl371_result = adxl371_self_test();
    sprintf(buffer, " - %s: ADXL371\r\n", adxl371_result ? "FAIL" : "PASS");
  }
  else
  {
    sprintf(buffer, " - WARNING: ADXL371 not detected\r\n");
  }
  send_test_report(buffer);

  set_power_spi1_bus(0, SPI1_CHIP_ALL);

  send_test_report("SPI2:\r\n");
  set_power_spi2_bus(1, SPI2_CHIP_ALL);
  HAL_Delay(50);
  uint8_t lis3mdl_result = lis3mdl_self_test();
  sprintf(buffer, " - %s: LIS3MDL\r\n", lis3mdl_result ? "FAIL" : "PASS");
  send_test_report(buffer);

  uint8_t lis2dw12_result = lis2dw12_self_test();
  sprintf(buffer, " - %s: LIS2DW12\r\n", lis2dw12_result ? "PASS" : "FAIL");
  send_test_report(buffer);

  set_power_spi2_bus(0, SPI2_CHIP_ALL);
#endif

  if (isAds1292Present())
  {
    //EXG_init(hspiExg);
    //ret_val |= EXG_test();
  }

  return ret_val;
}

void setup_factory_test(factory_test_target_t target, factory_test_t testToRun)
{
  factoryTestTarget = target;
  factoryTestToRun = testToRun;
}

void send_test_report(char *str)
{
  switch (factoryTestTarget)
  {
  case PRINT_TO_DEBUGGER:
    SHIMMER_PRINTF(str);
    break;
  case PRINT_TO_DOCK_UART:
    DockUart_writeBlocking((uint8_t *) str, strlen(str));
    break;
  case PRINT_TO_BT_UART:
    BT_write((uint8_t *) str, strlen(str));
    //TODO wait for msg to finish transmitting
    break;
  default:
    break;
  }
}
