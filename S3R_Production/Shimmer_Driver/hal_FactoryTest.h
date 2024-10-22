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

#define DELAY_BETWEEN_LED_CHANGES_MS           2000

#define TEST_THRESHOLD_VREF_LOWER              (VREF_EXTERNAL_SUPPLY_MV - 80)
#define TEST_THRESHOLD_VREF_UPPER              (VREF_EXTERNAL_SUPPLY_MV + 80)
/*
 * Range 1 (VCORE = 1.2 V) with CPU and peripherals running at up to 160 MHz
 * Range 2 (VCORE = 1.1 V) with CPU and peripherals running at up to 110 MHz
 * Range 3 (VCORE = 1.0 V) with CPU and peripherals running at up to 55 MHz
 * Range 4 (VCORE = 0.9 V) with CPU and peripherals running at up to 25 MHz
 * */
/* TODO values we're measuring (1376mV at time of writing) don't match values
given above */
#define TEST_THRESHOLD_VCORE_LOWER             900
#define TEST_THRESHOLD_VCORE_UPPER             1800
#define TEST_THRESHOLD_VBATT_PIN_LOWER         1850
#define TEST_THRESHOLD_VBATT_PIN_UPPER         1950
#define TEST_THRESHOLD_MCU_TEMPERATURE_LOWER   10
#define TEST_THRESHOLD_MCU_TEMPERATURE_UPPER   35
#define TEST_THRESHOLD_VBATT_LOWER             2980
#define TEST_THRESHOLD_VBATT_UPPER             4750
#define TEST_THRESHOLD_IMU_TEMPERATURE_LOWER   10.0
#define TEST_THRESHOLD_IMU_TEMPERATURE_UPPER   35.0

#define TEST_THRESHOLD_IMU_TEMPERATURE_INVALID -1.0

#define TEST_BT_MODULE_FW                      "v01.04.16.16"

#define SELF_TEST_STR_PASS                     "PASS"
#define SELF_TEST_STR_FAIL                     "FAIL"
#define SELF_TEST_STR_EMPTY                    ""
#define SELF_TEST_STR_CHIP_DETECTION           " - Chip not detected"
#define SELF_TEST_STR_SIGNAL_ISSUE             " - Signal issue"
#define SELF_TEST_STR_TEMPERATURE_ISSUE        " - Temperature issue"

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
} self_test_result_t;

uint32_t run_factory_test(void);
void print_date_and_time(void);
void print_shimmer_model(void);
void print_mcu_details(void);
void print_battery_details(void);
void led_test(void);
void sd_card_test(void);
uint8_t bt_module_test(void);
uint8_t I2C_test(void);
uint8_t SPI_test(void);
void setup_factory_test(factory_test_target_t target, factory_test_t testToRun);
uint8_t is_temperature_outside_of_range(float_t temperature);
void print_chip_test_result(char *testId, char *chipId, self_test_result_t self_test_result, float_t tempCal);
void send_test_report(char *str);

#endif /* HAL_FACTORYTEST_H_ */
