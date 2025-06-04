/*
 * adxl371_spi.h
 *
 *  Created on: Apr 19, 2024
 *      Author: MarkNolan
 */

#ifndef ADXL371_HAL_ADXL371_H_
#define ADXL371_HAL_ADXL371_H_

#include "hal_FactoryTest.h"
#include "stm32u5xx.h"
#include <stddef.h>
#include <stdint.h>

/* From https://github.com/analogdevicesinc/arduino/blob/master/Arduino%20Uno%20R3/examples/ADXL372_example/adxl372.h */
/*Acceleremoter configuration*/
#define ADXL371_ACT_VALUE           30 /* Activity threshold value */

#define ADXL371_INACT_VALUE         30 /* Inactivity threshold value */

#define ADXL371_ACT_TIMER           1 /* Activity timer value in multiples of 3.3ms */

#define ADXL371_INACT_TIMER         1 /* Inactivity timer value in multiples of 26ms */

#define ADXL371_ST                  0x01
#define ADXL371_ST_DONE             0x02

#define ADXL371_SELF_TEST_START_MSK NO_OS_BIT(0)
#define ADXL371_SELF_TEST_START(x)  (((x) & 0x1) << 0)

typedef int32_t (*dev_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*dev_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);
typedef void (*dev_mdelay_ptr)(uint32_t millisec);

//typedef struct
//{
//  /** Component mandatory fields **/
//  dev_write_ptr write_reg;
//  dev_read_ptr read_reg;
//  /** Component optional fields **/
//  dev_mdelay_ptr mdelay;
//  /** Customizable optional pointer **/
//  void *handle;
//} dev_ctx_t;

void adxl371_driver_init(void);
void adxl371_selectDevice(void);
void adxl371_unselectDevice(void);
self_test_result_t adxl371_self_test(void);
void adxl371_configure(uint8_t rate);
HAL_StatusTypeDef adxl371_accel_get(uint8_t *buf);
int32_t adxl371_is_data_rdy(void);

void adxl371_reset_chip(void);
uint8_t isAdxl371Detected(void);

#endif /* ADXL371_HAL_ADXL371_H_ */
