/*
 * lis2mdl.h
 *
 *  Created on: May 3, 2024
 *      Author: MarkNolan
 */

#ifndef LIS2MDL_HAL_LIS2MDL_H_
#define LIS2MDL_HAL_LIS2MDL_H_

#include <lis2mdl_reg.h>

void lis2mdl_driver_init(void);
uint8_t lis2mdl_self_test(void);
void lis2mdl_configure(float shimmerSamplingFreq, lis2mdl_odr_t rate);
HAL_StatusTypeDef lis2mdl_mag_get(uint8_t *buf);
bool lis2mdl_is_drdy_int_enabled(void);
bool lis2mdl_is_shimmer_freq_higher(float shimmerSamplingFreq, lis2mdl_odr_t rate);
float lis2mdl_get_sensor_freq_from_rate(lis2mdl_odr_t rate);

void lis2mdl_set_default_config(void);
void lis2mdl_sleep(void);

#endif /* LIS2MDL_HAL_LIS2MDL_H_ */
