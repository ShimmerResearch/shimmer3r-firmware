/*
 * adxl371_spi.h
 *
 *  Created on: Apr 19, 2024
 *      Author: MarkNolan
 */

#ifndef ADXL371_ADXL371_SPI_H_
#define ADXL371_ADXL371_SPI_H_

#include <stddef.h>
#include <stdint.h>

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
void adxl371_SelectDevice(void);
void adxl371_UnselectDevice(void);
uint8_t adxl371_self_test(void);

#endif /* ADXL371_ADXL371_SPI_H_ */
