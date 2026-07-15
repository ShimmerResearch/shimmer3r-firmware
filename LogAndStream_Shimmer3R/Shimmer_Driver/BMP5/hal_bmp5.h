/*
 * hal_bmp5.h
 *
 *  Created on: Jul 7, 2026
 *      Author: SuhasVarna
 */

#ifndef BMP5_BMP5_SENSORAPI_HAL_BMP5_H_
#define BMP5_BMP5_SENSORAPI_HAL_BMP5_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32u5xx.h"

#include "BMP5_SensorAPI/bmp5_defs.h"

/* Offset used to separate out the BMP5 specific self-test errors/warnings from
 * those used normally in Shimmer's self-test code structure. Value chosen
 * partly arbitrarily to ensure there's no overlap between the errors from each
 * source. Note the BMP5 API error codes are negative so, unlike the BMP3, the
 * offset is added to a negative number. */
#define BMP581_API_ERROR_OFFSET 100

void bmp5_setup_dev(void);
void bmp5_driver_init(void);
int8_t bmp5_verify_chip_id(void);
uint8_t bmp5_read_chip_id(void); /* TEMP DEV-818 diagnostic */
void bmp5_selectDevice(void);
void bmp5_unselectDevice(void);
uint8_t bmp5_self_test(void);
int8_t bmp5_drdy_test(void);
int8_t bmp5_configure(float shimmerSamplingFreq, uint8_t overSamplingRatio);
HAL_StatusTypeDef bmp5_pressure_temperature_get(uint8_t *buf);
bool bmp5_is_drdy_int_enabled(void);
bool bmp5_is_data_ready(void);
bool bmp5_is_shimmer_freq_higher(float shimmerSamplingFreq, uint8_t rate);
float bmp5_get_sensor_freq_from_rate(uint8_t rate);

int8_t bmp5_restore_default_config(void);
int8_t bmp5_read_int_status(void);
struct bmp5_sensor_data *get_bmp5_selftest_data(void);

/*!
 *  @brief Prints the execution status of the APIs.
 *
 *  @param[in] api_name : Name of the API whose execution status has to be printed.
 *  @param[in] rslt     : Error code returned by the API whose execution status has to be printed.
 *
 *  @return void.
 */
void bmp5_check_rslt(const char api_name[], int8_t rslt, char *outputStr);

#endif /* BMP5_BMP5_SENSORAPI_HAL_BMP5_H_ */
