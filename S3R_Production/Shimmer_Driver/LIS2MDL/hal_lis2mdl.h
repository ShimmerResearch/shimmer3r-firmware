/*
 * lis2mdl.h
 *
 *  Created on: May 3, 2024
 *      Author: MarkNolan
 */

#ifndef LIS2MDL_HAL_LIS2MDL_H_
#define LIS2MDL_HAL_LIS2MDL_H_

#include <lis2mdl_reg.h>

void lis2mdl_self_test(void);
void lis2mdl_driver_init(void);
void lis2mdl_base_settings_init(void);
void lis2mdl_power_on(void);
void lis2mdl_power_off(void);
void lis2mdl_set_default_config(void);
void lis2mdl_config_mag(lis2mdl_odr_t rate);
HAL_StatusTypeDef lis2mdl_mag_get(uint8_t *buf);
void lis2mdl_sleep(void);

#endif /* LIS2MDL_HAL_LIS2MDL_H_ */
