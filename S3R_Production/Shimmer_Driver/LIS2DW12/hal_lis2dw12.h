/*
 * lis2dw12.h
 *
 *  Created on: May 3, 2024
 *      Author: MarkNolan
 */

#ifndef LIS2DW12_HAL_LIS2DW12_H_
#define LIS2DW12_HAL_LIS2DW12_H_

#include "hal_FactoryTest.h"
#include <lis2dw12_reg.h>

void lis2dw12_driver_init(void);
void lis2dw12_selectDevice(void);
void lis2dw12_unselectDevice(void);
self_test_result_t lis2dw12_self_test(void);
uint8_t lis2dw12_drdy_test(void);
int32_t lis2dw12_configure(float shimmerSamplingFreq,
    lis2dw12_odr_t rate,
    lis2dw12_fs_t range,
    lis2dw12_mode_t mode);
HAL_StatusTypeDef lis2dw12_accel_get(uint8_t *buf);
bool lis2dw12_is_drdy_int_enabled(void);
bool lis2dw12_is_shimmer_freq_higher(float shimmerSamplingFreq, lis2dw12_odr_t rate);
float lis2dw12_get_sensor_freq_from_rate(lis2dw12_odr_t rate);

int32_t lis2dw12_standby(void);
int32_t lis2dw12_temperature_get(float_t *tempCal);

#endif /* LIS2DW12_HAL_LIS2DW12_H_ */
