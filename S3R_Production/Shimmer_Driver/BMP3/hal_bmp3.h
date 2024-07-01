/*
 * hal_bmp3.h
 *
 *  Created on: Apr 17, 2024
 *      Author: MarkNolan
 */

#ifndef BMP3_BMP3_SENSORAPI_HAL_BMP3_H_
#define BMP3_BMP3_SENSORAPI_HAL_BMP3_H_

#include <stdint.h>
#include "stm32u5xx.h"

/*!
 *  @brief Prints the execution status of the APIs.
 *
 *  @param[in] api_name : Name of the API whose execution status has to be printed.
 *  @param[in] rslt     : Error code returned by the API whose execution status has to be printed.
 *
 *  @return void.
 */
void bmp3_check_rslt(const char api_name[], int8_t rslt);

void bmp390_driver_init(void);
void bmp3_SelectDevice(void);
void bmp3_UnselectDevice(void);
HAL_StatusTypeDef bmp3_pressure_temperature_get(uint8_t *buf);
int8_t bmp390_self_test(void);
int8_t bmp390_restore_default_config(void);
void bmp390_config_set(uint8_t precision);
uint8_t* get_bmp3_calib_data_bytes(void);

#endif /* BMP3_BMP3_SENSORAPI_HAL_BMP3_H_ */
