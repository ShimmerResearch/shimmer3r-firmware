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

void init_lsm303ah(void);
//TODO move to spi.h
void set_power_spi2_bus(bool state);

#endif /* LSM303AH_LSM303AH_H_ */
