/*
 * lis3mdl.h
 *
 *  Created on: Apr 9, 2024
 *      Author: MarkNolan
 */

#ifndef LIS3MDL_LIS3MDL_H_
#define LIS3MDL_LIS3MDL_H_

void lis3mdl_self_test(void);

void lis3mdl_driver_init(void);
void lis3mdl_power_on(void);
void lis3mdl_power_off(void);

#endif /* LIS3MDL_LIS3MDL_H_ */
