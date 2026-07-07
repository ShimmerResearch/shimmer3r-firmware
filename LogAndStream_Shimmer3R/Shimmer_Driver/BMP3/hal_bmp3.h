/*
 * hal_bmp3.h
 *
 *  Created on: Apr 17, 2024
 *      Author: MarkNolan
 */

#ifndef BMP3_BMP3_SENSORAPI_HAL_BMP3_H_
#define BMP3_BMP3_SENSORAPI_HAL_BMP3_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32u5xx.h"

#define BMP_LEN_CALIB_DATA      BMP3_LEN_CALIB_DATA

#define BMP390_TEMP_SKIPPED     0x800000
#define BMP390_PRESS_SKIPPED    0x800000

/* Offset used to separate out the BMP3 specific self-test errors/warnings from
 * those used normally in Shimmer's self-test code structure. Value chosen
 * partly arbitrarily to ensure there's no overlap between the errors from each
 * source. */
#define BMP390_API_ERROR_OFFSET 100

void bmp3_setup_dev(void);
void bmp3_driver_init(void);
int8_t bmp3_verify_chip_id(void);
void bmp3_selectDevice(void);
void bmp3_unselectDevice(void);
uint8_t bmp3_self_test(void);
int8_t bmp3_drdy_test(void);
int8_t bmp3_configure(float shimmerSamplingFreq, uint8_t overSamplingRatio);
HAL_StatusTypeDef bmp3_pressure_temperature_get(uint8_t *buf);
bool bmp3_is_drdy_int_enabled(void);
bool bmp3_is_shimmer_freq_higher(float shimmerSamplingFreq, uint8_t rate);
float bmp3_get_sensor_freq_from_rate(uint8_t rate);

int8_t bmp3_restore_default_config(void);
uint8_t *get_bmp_calib_data_bytes(void);
uint8_t get_bmp_calib_data_bytes_len(void);
int8_t save_calib_data_bytes(void);
int8_t bmp3_read_sensor_status(void);
/*!
 *  @brief Prints the execution status of the APIs.
 *
 *  @param[in] api_name : Name of the API whose execution status has to be printed.
 *  @param[in] rslt     : Error code returned by the API whose execution status has to be printed.
 *
 *  @return void.
 */
void bmp3_check_rslt(const char api_name[], int8_t rslt, char *outputStr);

#endif /* BMP3_BMP3_SENSORAPI_HAL_BMP3_H_ */
