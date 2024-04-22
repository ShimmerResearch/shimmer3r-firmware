/*
 * hal_bmp3.h
 *
 *  Created on: Apr 17, 2024
 *      Author: MarkNolan
 */

#ifndef BMP3_BMP3_SENSORAPI_HAL_BMP3_H_
#define BMP3_BMP3_SENSORAPI_HAL_BMP3_H_

#include <stdint.h>

void bmp3_SelectDevice(void);
void bmp3_UnselectDevice(void);

void bmp390_driver_init(void);
int8_t bmp390_self_test(void);

#endif /* BMP3_BMP3_SENSORAPI_HAL_BMP3_H_ */
