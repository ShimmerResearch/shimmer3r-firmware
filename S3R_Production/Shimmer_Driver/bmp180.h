/*
 * Copyright (c) 2013, Shimmer Research, Ltd.
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of Shimmer Research, Ltd. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *    * You may not use or distribute this Software or any derivative works
 *      in any form for commercial purposes with the exception of commercial
 *      purposes when used in conjunction with Shimmer products purchased
 *      from Shimmer or their designated agent or with permission from
 *      Shimmer.
 *      Examples of commercial purposes would be running business
 *      operations, licensing, leasing, or selling the Software, or
 *      distributing the Software for use with commercial products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author Weibo Pan
 * @date May, 2013
 */

#ifndef BMP180_H
#define BMP180_H

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <math.h>

#define BMP180_ADDR  0xEE //7 bit address I2C address of the BMP180

//registers
#define BMP180_OUT_XLSB     0xF8
#define BMP180_OUT_LSB      0xF7
#define BMP180_OUT_MSB      0xF6
#define BMP180_CTRL_MEAS    0xF4
#define BMP180_SOFT_RESET   0xE0
#define BMP180_ID           0xD0
//calibration coefficients
#define BMP180_AC1_MSB      0xAA
#define BMP180_AC1_LSB      0xAB
#define BMP180_AC2_MSB      0xAC
#define BMP180_AC2_LSB      0xAD
#define BMP180_AC3_MSB      0xAE
#define BMP180_AC3_LSB      0xAF
#define BMP180_AC4_MSB      0xB0
#define BMP180_AC4_LSB      0xB1
#define BMP180_AC5_MSB      0xB2
#define BMP180_AC5_LSB      0xB3
#define BMP180_AC6_MSB      0xB4
#define BMP180_AC6_LSB      0xB5
#define BMP180_B1_MSB       0xB6
#define BMP180_B1_LSB       0xB7
#define BMP180_B2_MSB       0xB8
#define BMP180_B2_LSB       0xB9
#define BMP180_MB_MSB       0xBA
#define BMP180_MB_LSB       0xBB
#define BMP180_MC_MSB       0xBC
#define BMP180_MC_LSB       0xBD
#define BMP180_MD_MSB       0xBE
#define BMP180_MD_LSB       0xBF

/* Multiple is faster than divide */
#define BMP180_1_16     ((float) 0.0625)
#define BMP180_1_256    ((float) 0.00390625)
#define BMP180_1_2048   ((float) 0.00048828125)
#define BMP180_1_4096   ((float) 0.000244140625)
#define BMP180_1_8192   ((float) 0.0001220703125)
#define BMP180_1_32768  ((float) 0.000030517578125)
#define BMP180_1_65536  ((float) 0.0000152587890625)
#define BMP180_1_101325 ((float) 0.00000986923266726)

//initialise the I2C for use with the BMP180
void BMP180_init(I2C_HandleTypeDef *hi2c);

//returns the ID
//will always be 0x55
//useful for checking communication
uint8_t BMP180_getId(void);

//Initiate temperature measurement
//Need to wait 4.5ms before reading value
HAL_StatusTypeDef BMP180_tempStartMeasurement(void);

//read temperature
//16-bit value returned in buf
//big endian
HAL_StatusTypeDef BMP180_tempReadTx(void);
HAL_StatusTypeDef BMP180_tempReadRx(uint8_t *rx_buf) ;

//Initiate temperature measurement
//if OSS = 0 need to wait 4.5ms before reading value
//if OSS = 1 need to wait 7.5ms before reading value
//if OSS = 2 need to wait 13.5ms before reading value
//if OSS = 3 need to wait 25.5ms before reading value
HAL_StatusTypeDef BMP180_presStartMeasurement(uint8_t oss);

//read pressure
//19-bit (in 3 bytes) value returned in *buf
//big endian
HAL_StatusTypeDef BMP180_presReadTx(void);
HAL_StatusTypeDef BMP180_presReadRx(uint8_t *rx_buf);

//read calibration Coefficients
//AC1 to MD
//11x16-bit values returned in res
//big endian
HAL_StatusTypeDef BMP180_getCalib(uint8_t* rx_buf);

// calculate temperature
// must read calibration coefficients before this calculation
float BMP180_calcTemp(uint16_t temp_val_16);

// calculate pressure and altitude
// must read calibration coefficients before this calculation
// return pressure value as uint32_t value. 
// updates altitude value
// if altitude not needed, use 0 as input parameter
uint32_t BMP180_calcPres(uint32_t pres_val_32, uint8_t oss, float* altitude);

#endif //BMP180_H
