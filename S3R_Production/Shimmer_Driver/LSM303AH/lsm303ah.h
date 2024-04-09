/*
 * lsm303ah.h
 *
 *  Created on: Apr 8, 2024
 *      Author: MarkNolan
 */

#ifndef LSM303AH_LSM303AH_H_
#define LSM303AH_LSM303AH_H_

#include <stdbool.h>

void lsm303ah_self_test(void);

void lsm303ah_driver_init(void);
void lsm303ah_power_on(void);
void lsm303ah_power_off(void);

#endif /* LSM303AH_LSM303AH_H_ */
