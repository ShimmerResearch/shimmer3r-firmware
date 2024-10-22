/*
 * lis3mdl.h
 *
 *  Created on: Apr 9, 2024
 *      Author: MarkNolan
 */

#ifndef LIS3MDL_HAL_LIS3MDL_H_
#define LIS3MDL_HAL_LIS3MDL_H_

#include <lis3mdl_reg.h>
#include "hal_FactoryTest.h"

void lis3mdl_driver_init(void);
void lis3mdl_selectDevice(void);
void lis3mdl_unselectDevice(void);
self_test_result_t lis3mdl_self_test(void);
void lis3mdl_configure(float shimmerSamplingFreq, lis3mdl_om_t rate, lis3mdl_fs_t range);
HAL_StatusTypeDef lis3mdl_mag_get(uint8_t *buf);
bool lis3mdl_is_drdy_int_enabled(void);
bool lis3mdl_is_shimmer_freq_higher(float shimmerSamplingFreq, lis3mdl_om_t rate);
float lis3mdl_get_sensor_freq_from_rate(lis3mdl_om_t rate);

void lis3mdl_restore_default_config(void);
void lis3mdl_spi_three_wire_set(void);
int32_t lis3mdl_temperature_get(float_t *tempCal);

#endif /* LIS3MDL_HAL_LIS3MDL_H_ */
