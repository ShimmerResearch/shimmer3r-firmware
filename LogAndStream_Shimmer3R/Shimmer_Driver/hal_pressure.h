/*
 * hal_pressure.h
 *
 * Thin dispatch layer between the BMP390 (BMP3 API) and BMP581 (BMP5 API)
 * pressure sensor HALs. The BMP581 is a pin-compatible replacement for the
 * BMP390 fitted to up-rev'd Shimmer3R boards (DEV-95/DEV-818). Which sensor
 * is fitted is determined at boot from the SR number, confirmed by reading
 * the chip ID.
 *
 *  Created on: Jul 7, 2026
 *      Author: SuhasVarna
 */

#ifndef HAL_PRESSURE_H_
#define HAL_PRESSURE_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32u5xx.h"

void PressureSensor_detect(void);
void PressureSensor_init(void);
int8_t PressureSensor_configure(float shimmerSamplingFreq, uint8_t overSamplingRatio);
HAL_StatusTypeDef PressureSensor_getDataDma(uint8_t *buf);
bool PressureSensor_isDrdyIntEnabled(void);
void PressureSensor_clearDrdyInt(void);
void PressureSensor_selectDevice(void);
void PressureSensor_unselectDevice(void);
uint8_t PressureSensor_selfTest(void);

uint8_t isBmp390InUse(void);
uint8_t isBmp581InUse(void);

#endif /* HAL_PRESSURE_H_ */
