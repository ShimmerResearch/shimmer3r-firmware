/*
 * lsm6dsv.h
 *
 *  Created on: Apr 9, 2024
 *      Author: MarkNolan
 */

#ifndef LSM6DSV_LSM6DSV_H_
#define LSM6DSV_LSM6DSV_H_

void lsm6dsv_self_test(void);

void lsm6dsv_driver_init(void);
void lsm6dsv_power_on(void);
void lsm6dsv_power_off(void);

#endif /* LSM6DSV_LSM6DSV_H_ */
