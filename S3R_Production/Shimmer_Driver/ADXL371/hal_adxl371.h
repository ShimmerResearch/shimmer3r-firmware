/*
 * adxl371_spi.h
 *
 *  Created on: Apr 19, 2024
 *      Author: MarkNolan
 */

#ifndef ADXL371_HAL_ADXL371_H_
#define ADXL371_HAL_ADXL371_H_

#include <stddef.h>
#include <stdint.h>

/* From https://github.com/analogdevicesinc/arduino/blob/master/Arduino%20Uno%20R3/examples/ADXL372_example/adxl372.h */
/*Acceleremoter configuration*/
#define ADXL371_ACT_VALUE          30     /* Activity threshold value */

#define ADXL371_INACT_VALUE        30     /* Inactivity threshold value */

#define ADXL371_ACT_TIMER          1    /* Activity timer value in multiples of 3.3ms */

#define ADXL371_INACT_TIMER        1     /* Inactivity timer value in multiples of 26ms */

typedef int32_t (*dev_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*dev_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);
typedef void (*dev_mdelay_ptr)(uint32_t millisec);

typedef struct
{
  /** Component mandatory fields **/
  dev_write_ptr write_reg;
  dev_read_ptr read_reg;
  /** Component optional fields **/
  dev_mdelay_ptr mdelay;
  /** Customizable optional pointer **/
  void *handle;
} dev_ctx_t;

void adxl371_driver_init(void);
void adxl371_power_on(void);
void adxl371_power_off(void);
void adxl371_selectDevice(void);
void adxl371_unselectDevice(void);
uint8_t adxl371_self_test(void);
void adxl371_restore_default_config(void);
void adxl371_config_accel(uint8_t rate);

#endif /* ADXL371_HAL_ADXL371_H_ */
