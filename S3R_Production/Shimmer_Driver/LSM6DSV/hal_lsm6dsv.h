/*
 * lsm6dsv.h
 *
 *  Created on: Apr 9, 2024
 *      Author: MarkNolan
 */

#ifndef LSM6DSV_HAL_LSM6DSV_H_
#define LSM6DSV_HAL_LSM6DSV_H_

void lsm6dsv_driver_init(void);
void lsm6dsv_base_settings_init(void);
void lsm6dsv_power_on(void);
void lsm6dsv_power_off(void);
void lsm6dsv_selectDevice(void);
void lsm6dsv_unselectDevice(void);
uint8_t lsm6dsv_self_test(void);
void lsm6dsv_restore_default_config(void);
void lsm6dsv_config_accel(uint8_t rate, uint8_t range);
void lsm6dsv_config_gyro(uint8_t rate, uint8_t range);
HAL_StatusTypeDef lsm6dsv_accel_get(uint8_t *buf);
HAL_StatusTypeDef lsm6dsv_gyro_get(uint8_t *buf);

void lsm6dsv_status_get(void);

#endif /* LSM6DSV_HAL_LSM6DSV_H_ */
