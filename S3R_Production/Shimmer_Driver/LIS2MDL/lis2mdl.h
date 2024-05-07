/*
 * lis2mdl.h
 *
 *  Created on: May 3, 2024
 *      Author: MarkNolan
 */

#ifndef LIS2MDL_LIS2MDL_H_
#define LIS2MDL_LIS2MDL_H_

void lis2mdl_self_test(void);
void lis2mdl_driver_init(void);
void lis2mdl_set_default_config(void);
void lis2mdl_config_mag(uint8_t rate, uint8_t range);
void lis2mdl_sleep(void);

#endif /* LIS2MDL_LIS2MDL_H_ */
