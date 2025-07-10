/*
 * hal_FactoryTest.h
 *
 *  Created on: Aug 14, 2024
 *      Author: MarkNolan
 */

#ifndef HAL_FACTORYTEST_H_
#define HAL_FACTORYTEST_H_

#include <math.h>
#include <stdint.h>

#include <stm32u5xx.h>

#define DELAY_BETWEEN_LED_CHANGES_MS               2000

#define TEST_THRESHOLD_VREF_LOWER                  (VREF_EXTERNAL_SUPPLY_MV - 80)
#define TEST_THRESHOLD_VREF_UPPER                  (VREF_EXTERNAL_SUPPLY_MV + 80)
/*
 * Range 1 (VCORE = 1.2 V) with CPU and peripherals running at up to 160 MHz
 * Range 2 (VCORE = 1.1 V) with CPU and peripherals running at up to 110 MHz
 * Range 3 (VCORE = 1.0 V) with CPU and peripherals running at up to 55 MHz
 * Range 4 (VCORE = 0.9 V) with CPU and peripherals running at up to 25 MHz
 * */
/* TODO values we're measuring (1376mV at time of writing) don't match values
given above */
#define TEST_THRESHOLD_MV_VCORE_LOWER              900
#define TEST_THRESHOLD_MV_VCORE_UPPER              1800

/* 1.8V regulator */
#define TEST_THRESHOLD_MV_VBATT_PIN_LOWER          1750
#define TEST_THRESHOLD_MV_VBATT_PIN_UPPER          1850
/* 1.9V regulator */
#define TEST_THRESHOLD_MV_VBATT_PIN_LOWER_SR48_6_0 1850
#define TEST_THRESHOLD_MV_VBATT_PIN_UPPER_SR48_6_0 1950

#define TEST_THRESHOLD_MV_MCU_TEMPERATURE_LOWER    10
#define TEST_THRESHOLD_MV_MCU_TEMPERATURE_UPPER    40
#define TEST_THRESHOLD_MV_VBATT_LOWER              2980
#define TEST_THRESHOLD_MV_VBATT_UPPER              4750
#define TEST_THRESHOLD_DEG_IMU_TEMPERATURE_LOWER   10.0
#define TEST_THRESHOLD_DEG_IMU_TEMPERATURE_UPPER   40.0

#define TEST_THRESHOLD_DEG_IMU_TEMPERATURE_INVALID -1.0

#define TEST_BT_MODULE_FW                          "v01.04.16.16"

#define SELF_TEST_STR_PASS                         "PASS"
#define SELF_TEST_STR_FAIL                         "FAIL"
#define SELF_TEST_STR_EMPTY                        ""
#define SELF_TEST_STR_CHIP_DETECTION               " - Chip not detected"
#define SELF_TEST_STR_SIGNAL_ISSUE                 " - Signal issue"
#define SELF_TEST_STR_TEMPERATURE_ISSUE            " - Temperature issue"
#define SELF_TEST_STR_DRDY_ISSUE                   " - DRDY/INT issue"
#define SELF_TEST_STR_UNKNOWN                      " - Unknown"

#define GSR_TEST_TOLERANCE_5_PERCENT               0.05
#define GSR_TEST_TOLERANCE_7_PERCENT               0.07

typedef enum
{
  PRINT_TO_DEBUGGER = 0,
  PRINT_TO_DOCK_UART,
  PRINT_TO_BT_UART
} factory_test_target_t;

typedef enum
{
  FACTORY_TEST_MAIN = 0,
  FACTORY_TEST_LEDS,
  FACTORY_TEST_ICS,
  FACTORY_TEST_COUNT
} factory_test_t;

typedef enum
{
  SELF_TEST_PASS = 0,
  SELF_TEST_FAIL_CHIP_DETECTION,
  SELF_TEST_FAIL_SIGNAL_ISSUE,
  SELF_TEST_FAIL_TEMPERATURE_ISSUE,
  SELF_TEST_FAIL_DRDY_ISSUE,
  SELF_TEST_FAIL_COUNT,
} self_test_result_t;

enum
{
  //Test index starts at 1
  S3R_TEST_0003 = (1 << (3 - 1)),  //Shimmer model set in EEPROM
  S3R_TEST_0007 = (1 << (7 - 1)),  //MCU Vref
  S3R_TEST_0008 = (1 << (8 - 1)),  //MCU Vcore
  S3R_TEST_0009 = (1 << (9 - 1)),  //MCU VBatt
  S3R_TEST_0010 = (1 << (10 - 1)), //MCU Temperature
  S3R_TEST_0011 = (1 << (11 - 1)), //Battery voltage
  S3R_TEST_0012 = (1 << (12 - 1)), //Battery charger chip
  S3R_TEST_0013 = (1 << (13 - 1)), //SD test
  S3R_TEST_0014 = (1 << (14 - 1)), //BT Module
  S3R_TEST_0015 = (1 << (15 - 1)), //SPI1 - ADS7028
  S3R_TEST_0016 = (1 << (16 - 1)), //SPI1 - LSM6DSV
  S3R_TEST_0017 = (1 << (17 - 1)), //SPI1 - BMP390
  S3R_TEST_0018 = (1 << (18 - 1)), //SPI1 - ADXL371
  S3R_TEST_0019 = (1 << (19 - 1)), //SPI2 - LIS3MDL
  S3R_TEST_0020 = (1 << (20 - 1)), //SPI2 - LIS2DW12
  S3R_TEST_0021 = (1 << (21 - 1)), //SPI3 - ADS1292R (ExG)
  S3R_TEST_0022 = (1 << (22 - 1)), //I2C1 - LIS2MDL
  S3R_TEST_0023 = (1 << (23 - 1)), //I2C1 - CAT24C16
  S3R_TEST_0024 = (1 << (24 - 1)), //I2C4 - CAT24C16 or GSR rig (S3R GSR+ unit)
  S3R_TEST_0025 = (1 << (25 - 1)), //GSR signal test
  S3R_TEST_0026 = (1 << (26 - 1)), //Microphone
};

typedef struct
{
  const char *message;
  uint8_t selfTestResult;
} micTestResult_t;

uint32_t run_factory_test(void);
void print_date_and_time(void);
void print_shimmer_model(void);
void print_mcu_details(void);
void print_battery_details(void);
void led_test(void);
void sd_card_test(void);
void bt_module_test(void);
void I2C_test(void);
void SPI_test(void);
void setup_factory_test(factory_test_target_t target, factory_test_t testToRun);
uint8_t is_temperature_outside_of_range(float_t temperature);
void print_chip_test_result(char *testId, char *chipId, self_test_result_t self_test_result, float_t tempCal);
void send_test_report(const char *str);
uint8_t gsrFactoryTest_run(void);
void gsrFactoryTest_printResults(void);
HAL_StatusTypeDef gsrFactoryTest_getAvgGsr(uint32_t *gsrResistance);
uint8_t runMicrophoneTest(void);
const micTestResult_t *getMicTestResult(uint8_t micResult);

#endif /* HAL_FACTORYTEST_H_ */
