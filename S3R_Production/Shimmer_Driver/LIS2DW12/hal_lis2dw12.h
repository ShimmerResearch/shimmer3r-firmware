/*
 * lis2dw12.h
 *
 *  Created on: May 3, 2024
 *      Author: MarkNolan
 */

#ifndef LIS2DW12_HAL_LIS2DW12_H_
#define LIS2DW12_HAL_LIS2DW12_H_

#include <lis2dw12_reg.h>

void lis2dw12_driver_init(void);
void lis2dw12_power_on(void);
void lis2dw12_power_off(void);
void lis2dw12_selectDevice(void);
void lis2dw12_unselectDevice(void);
uint8_t lis2dw12_self_test(void);
int32_t lis2dw12_configure(float shimmerSamplingFreq, lis2dw12_odr_t rate, lis2dw12_fs_t range);
HAL_StatusTypeDef lis2dw12_accel_get(uint8_t *buf);
bool lis2dw12_is_drdy_int_enabled(void);
bool lis2dw12_is_shimmer_freq_higher(float shimmerSamplingFreq, lis2dw12_odr_t rate);
float lis2dw12_get_sensor_freq_from_rate(lis2dw12_odr_t rate);

int32_t lis2dw12_standby(void);

#endif /* LIS2DW12_HAL_LIS2DW12_H_ */
