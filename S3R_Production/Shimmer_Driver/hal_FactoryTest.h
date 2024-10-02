/*
 * hal_FactoryTest.h
 *
 *  Created on: Aug 14, 2024
 *      Author: MarkNolan
 */

#ifndef HAL_FACTORYTEST_H_
#define HAL_FACTORYTEST_H_

#include <stdint.h>

#define DELAY_BETWEEN_LED_CHANGES_MS 2000

#define TEST_THRESHOLD_VREF_LOWER (VREF_EXTERNAL_SUPPLY_MV-80)
#define TEST_THRESHOLD_VREF_UPPER (VREF_EXTERNAL_SUPPLY_MV+80)
#define TEST_THRESHOLD_VCORE_LOWER 1120
#define TEST_THRESHOLD_VCORE_UPPER 1280
#define TEST_THRESHOLD_VBATT_PIN_LOWER 1850
#define TEST_THRESHOLD_VBATT_PIN_UPPER 1950
#define TEST_THRESHOLD_MCU_TEMPERATURE_LOWER 10
#define TEST_THRESHOLD_MCU_TEMPERATURE_UPPER 35
#define TEST_THRESHOLD_VBATT_LOWER 2980
#define TEST_THRESHOLD_VBATT_UPPER 4750
#define TEST_THRESHOLD_BMP_TEMPERATURE_LOWER 10
#define TEST_THRESHOLD_BMP_TEMPERATURE_UPPER 35

#define TEST_BT_MODULE_FW "v01.04.16.16"

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
void send_test_report(char *str);

#endif /* HAL_FACTORYTEST_H_ */
