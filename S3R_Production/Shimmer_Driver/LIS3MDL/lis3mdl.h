/*
 * lis3mdl.h
 *
 *  Created on: Apr 9, 2024
 *      Author: MarkNolan
 */

#ifndef LIS3MDL_LIS3MDL_H_
#define LIS3MDL_LIS3MDL_H_

uint8_t lis3mdl_self_test(void);

void lis3mdl_driver_init(void);
void lis3mdl_SelectDevice(void);
void lis3mdl_UnselectDevice(void);
void lis3mdl_restore_default_config(void);

void lis3mdl_spi_three_wire_set(void);
void lis3mdl_config_mag(uint8_t rate, uint8_t range);
HAL_StatusTypeDef lis3mdl_mag_get(uint8_t *buf);

#endif /* LIS3MDL_LIS3MDL_H_ */
