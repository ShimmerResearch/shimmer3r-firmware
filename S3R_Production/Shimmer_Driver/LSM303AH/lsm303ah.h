/*
 * lsm303ah.h
 *
 *  Created on: Apr 8, 2024
 *      Author: MarkNolan
 */

#ifndef LSM303AH_LSM303AH_H_
#define LSM303AH_LSM303AH_H_

#include <stdint.h>
#include <stdbool.h>

void lsm303ah_self_test(void);

void lsm303ah_driver_init(void);
void lsm303ah_power_on(void);
void lsm303ah_power_off(void);
void lsm303ah_SelectDevice(void);
void lsm303ah_UnselectDevice(void);

//initialize the accelerometer
void lsm303ah_accelInit(uint8_t samplingrate, uint8_t range, uint8_t lowpower, uint8_t highresolution);

//initialize the magnetometer
void lsm303ah_magInit(uint8_t samplingrate);

//put x, y and z accel values into buf
void lsm303ah_getAccel(uint8_t *buf);

//put x, y and z mag values into buf
void lsm303ah_getMag(uint8_t *buf);

//powers down the lsm303ah
void lsm303ah_sleep(void);

void lsm303ah_restore_default_config(void);

#endif /* LSM303AH_LSM303AH_H_ */
