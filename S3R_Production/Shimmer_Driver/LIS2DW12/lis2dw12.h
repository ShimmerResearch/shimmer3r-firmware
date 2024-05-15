/*
 * lis2dw12.h
 *
 *  Created on: May 3, 2024
 *      Author: MarkNolan
 */

#ifndef LIS2DW12_LIS2DW12_H_
#define LIS2DW12_LIS2DW12_H_

void lis2dw12_self_test(void);
void lis2dw12_driver_init(void);
void lis2dw12_SelectDevice(void);
void lis2dw12_UnselectDevice(void);

void lis2dw12_config_accel(uint8_t rate, uint8_t range);
HAL_StatusTypeDef lis2dw12_accel_get(uint8_t *buf);

#endif /* LIS2DW12_LIS2DW12_H_ */
