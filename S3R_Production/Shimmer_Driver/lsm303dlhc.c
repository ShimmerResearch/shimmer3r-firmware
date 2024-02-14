/*
 * Copyright (c) 2016, Shimmer Research, Ltd.
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
 * @date May, 2016
 */
 
#include "stm32u5xx_hal.h"
#include "lsm303dlhc.h"

static I2C_HandleTypeDef *hi2c_LSM303DLHC;

void LSM303DLHC_init(I2C_HandleTypeDef *hi2c) {
   hi2c_LSM303DLHC = hi2c;
}

uint8_t LSM303DLHC_accelInit(uint8_t samplingrate, uint8_t range, uint8_t lowpower, uint8_t highresolution) {
   uint8_t i2c_buffer[2], temp;
   HAL_StatusTypeDef i2c_status;
   
   if(samplingrate > 9) samplingrate = 5;
   if(range > 3) range = 0;
   if(lowpower > 1) lowpower = 0;
   if(highresolution > 1) highresolution = 0;
   
   //Configure Accel
   i2c_buffer[0] = CTRL_REG1_A;
   i2c_buffer[1] = (samplingrate << 4) + (lowpower << 3) + 0x07;
   HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, i2c_buffer, 2, 5);
   //write CTRL_REG4_A register
   i2c_buffer[0] = CTRL_REG4_A;
   i2c_buffer[1] = (range << 4) + (highresolution << 3);
   i2c_status = HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, i2c_buffer, 2, 5);
   
   temp = i2c_buffer[0];
   i2c_status = HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, &temp, 1, 5);
   i2c_status = HAL_I2C_Master_Receive(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, &temp, 1, 5);
   if((i2c_status == HAL_OK) &&
      (temp == i2c_buffer[1])){
      return 0;
   }else{
      return 1;
   }   
   //return (i2c_status == HAL_OK);
}

uint8_t LSM303DLHC_magInit(uint8_t samplingrate, uint8_t gain) {
   uint8_t i2c_buffer[2], temp;
   HAL_StatusTypeDef i2c_status;

   if(samplingrate > 7) samplingrate = 6;
   if(gain<1 || gain>7) gain = 1;

   //write CRA_REG_M register
   i2c_buffer[0] = CRA_REG_M;
   i2c_buffer[1] = samplingrate << 2;
   HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, i2c_buffer, 2, 5);
   //write CRB_REG_M register
   i2c_buffer[0] = CRB_REG_M;
   i2c_buffer[1] = gain << 5;
   HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, i2c_buffer, 2, 5);
   //write MR_REG_M register
   i2c_buffer[0] = MR_REG_M;
   i2c_buffer[1] = 0x00; //continuous-conversion mode
   i2c_status = HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, i2c_buffer, 2, 5);
   
   temp = i2c_buffer[0];
   i2c_status = HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, &temp, 1, 5);
   i2c_status = HAL_I2C_Master_Receive(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, &temp, 1, 5);
   if((i2c_status == HAL_OK) &&
      (temp == i2c_buffer[1])){
      return 0;
   }else{
      return 1;
   }   
   //return (i2c_status == HAL_OK);
}

void LSM303DLHC_getAccel(uint8_t *buf) {
   // need to assert MSB of sub-address in order to read multiple bytes.
   // See section 5.1.2 of LSM303DLHC datasheet (April 2011, Doc ID 018771 Rev1) for details
   *buf = OUT_X_L_A | 0x80;
   HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, buf, 1, 5);
   HAL_I2C_Master_Receive(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, buf, 6, 5);
}

HAL_StatusTypeDef LSM303DLHC_accelReadStart() {
   uint8_t buf = OUT_X_L_A | 0x80;
   return HAL_I2C_Master_Transmit_DMA(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, &buf, 1);
}

HAL_StatusTypeDef LSM303DLHC_accelReadDone(uint8_t *buf) {
   return HAL_I2C_Master_Receive_DMA(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, buf, 6);
}

void LSM303DLHC_getMag(uint8_t *buf) {
   *buf = OUT_X_H_M;
   HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, buf, 1, 5);
   HAL_I2C_Master_Receive(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, buf, 6, 5);
}

HAL_StatusTypeDef LSM303DLHC_magReadStart() {
   uint8_t buf = OUT_X_H_M;
   return HAL_I2C_Master_Transmit_DMA(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, &buf, 1);
}

HAL_StatusTypeDef LSM303DLHC_magReadDone(uint8_t *buf) {
   return HAL_I2C_Master_Receive_DMA(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, buf, 6);
}

uint8_t LSM303DLHC_sleep(void) {
   uint8_t i2c_buffer[2];
   HAL_StatusTypeDef i2c_status;

   //write CTRL_REG1_A register
   i2c_buffer[0] = CTRL_REG1_A;
   i2c_buffer[1] = 0;      //power down mode, 3 axes disabled
   HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_ACCEL_ADDR, i2c_buffer, 2, 5);

   //write MR_REG_M register
   i2c_buffer[0] = MR_REG_M;
   i2c_buffer[1] = 0x02;   //sleep mode
   i2c_status = HAL_I2C_Master_Transmit(hi2c_LSM303DLHC, LSM303DHLC_MAG_ADDR, i2c_buffer, 2, 5);
   return (HAL_OK == i2c_status);
}


