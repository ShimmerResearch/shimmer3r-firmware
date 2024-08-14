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

typedef enum
{
  PRINT_TO_DEBUGGER = 0,
  PRINT_TO_DOCK_UART,
  PRINT_TO_BT_UART
} factory_test_target_t;

typedef enum
{
  FACTORY_TEST_ALL = 0,
  //FACTORY_TEST_LED_START,
  //FACTORY_TEST_STOP,
  FACTORY_TEST_COUNT
} factory_test_t;

uint32_t run_factory_test(void);
void print_date_and_time(void);
void print_shimmer_model(void);
void led_test(void);
void sd_card_test(void);
uint8_t bt_module_test(void);
uint8_t I2C_test(void);
uint8_t SPI_test(void);
void setup_factory_test(factory_test_target_t target, factory_test_t testToRun);
void send_test_report(char *str);

#endif /* HAL_FACTORYTEST_H_ */
