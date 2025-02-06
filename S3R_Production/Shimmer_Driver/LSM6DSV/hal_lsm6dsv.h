/*
 * lsm6dsv.h
 *
 *  Created on: Apr 9, 2024
 *      Author: MarkNolan
 */

#ifndef LSM6DSV_HAL_LSM6DSV_H_
#define LSM6DSV_HAL_LSM6DSV_H_

#include "hal_FactoryTest.h"
#include <lsm6dsv_reg.h>

void lsm6dsv_driver_init(void);
void lsm6dsv_selectDevice(void);
void lsm6dsv_unselectDevice(void);
self_test_result_t lsm6dsv_self_test(void);
uint8_t lsm6dsv_drdy_test(void);
void lsm6dsv_configure(float shimmerSamplingFreq,
    uint8_t isGyroEn,
    uint8_t isAccelEn,
    uint8_t rate,
    uint8_t rangeGyro,
    uint8_t rangeAccel);
HAL_StatusTypeDef lsm6dsv_accel_get(uint8_t *buf);
HAL_StatusTypeDef lsm6dsv_gyro_get(uint8_t *buf);
HAL_StatusTypeDef lsm6dsv_gyro_accel_get(uint8_t *buf);
bool lsm6dsv_is_drdy_int_enabled(void);
bool lsm6dsv_is_shimmer_freq_higher(float shimmerSamplingFreq, lsm6dsv_data_rate_t rate);
float lsm6dsv_get_sensor_freq_from_rate(lsm6dsv_data_rate_t rate);

void lsm6dsv_restore_default_config(void);
void lsm6dsv_status_get(void);
int32_t lsm6dsv_temperature_get(float_t *tempCal);

#endif /* LSM6DSV_HAL_LSM6DSV_H_ */
