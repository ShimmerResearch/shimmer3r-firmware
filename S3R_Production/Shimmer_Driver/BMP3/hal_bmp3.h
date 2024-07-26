/*
 * hal_bmp3.h
 *
 *  Created on: Apr 17, 2024
 *      Author: MarkNolan
 */

#ifndef BMP3_BMP3_SENSORAPI_HAL_BMP3_H_
#define BMP3_BMP3_SENSORAPI_HAL_BMP3_H_

#include "stm32u5xx.h"
#include <stdint.h>

#define BMP_LEN_CALIB_DATA BMP3_LEN_CALIB_DATA

void bmp3_driver_init(void);
void bmp3_power_on(void);
void bmp3_power_off(void);
void bmp3_selectDevice(void);
void bmp3_unselectDevice(void);
int8_t bmp3_self_test(void);
int8_t bmp3_restore_default_config(void);
void bmp3_config_set(uint8_t precision);
HAL_StatusTypeDef bmp3_pressure_temperature_get(uint8_t *buf);

uint8_t *get_bmp_calib_data_bytes(void);
uint8_t get_bmp_calib_data_bytes_len(void);
int8_t save_calib_data_bytes(void);
/*!
 *  @brief Prints the execution status of the APIs.
 *
 *  @param[in] api_name : Name of the API whose execution status has to be printed.
 *  @param[in] rslt     : Error code returned by the API whose execution status has to be printed.
 *
 *  @return void.
 */
void bmp3_check_rslt(const char api_name[], int8_t rslt);

#endif /* BMP3_BMP3_SENSORAPI_HAL_BMP3_H_ */
