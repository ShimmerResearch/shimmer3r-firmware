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

#include "mpu9250.h"
#include "stm32u5xx_hal.h"

I2C_HandleTypeDef *hi2c_MPU9250;
uint8_t *mpu9250_mag_data_ptr;

uint8_t MPU9250_init(I2C_HandleTypeDef *hi2c)
{
  uint8_t buf[2];
  static HAL_StatusTypeDef i2c_status;
  //POWER NEET TO BE TURNED ON

  hi2c_MPU9250 = hi2c;

  *buf = MPU9250_USER_CTRL;
  buf[1] = 0; //ensure I2C_MST_EN is 0
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 2, 5);

  *buf = MPU9250_INT_PIN_CFG;
  buf[1] = 0x02; //set I2C_BYPASS_EN to 1
  i2c_status = HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 2, 5);
  return (HAL_OK == i2c_status);
}

uint8_t MPU9250_getId(void)
{
  uint8_t buf[2];
  buf[0] = MPU9250_WHO_AM_I;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  return buf[0];
}

uint8_t MPU9250_test(void)
{
  return MPU9250_getId() == 0x71 ? 0 : 1;
}

void MPU9250_wake(uint8_t wakeup)
{
  uint8_t buf[2];

  *buf = MPU9250_PWR_MGMT_1;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);

  if (wakeup)
  {
    //wakeup
    buf[1] = buf[0] & 0xBF;
  }
  else
  {
    //go back to sleep
    buf[1] = buf[0] | 0x40;
  }
  *buf = MPU9250_PWR_MGMT_1;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  __NOP();
}

void MPU9250_getGyro(uint8_t *buf)
{
  *buf = MPU9250_GYRO_XOUT_H;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_ADDR, buf, 6, 5);
}

HAL_StatusTypeDef MPU9250_gyroReadStart()
{
  static uint8_t buf = MPU9250_GYRO_XOUT_H;
  return HAL_I2C_Master_Transmit_DMA(hi2c_MPU9250, MPU9250_ADDR, &buf, 1);
}

HAL_StatusTypeDef MPU9250_gyroReadDone(uint8_t *buf)
{
  return HAL_I2C_Master_Receive_DMA(hi2c_MPU9250, MPU9250_ADDR, buf, 6);
}

void MPU9250_getAccel(uint8_t *buf)
{
  *buf = MPU9250_ACCEL_XOUT_H;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_ADDR, buf, 6, 5);
}

HAL_StatusTypeDef MPU9250_accelReadStart()
{
  uint8_t buf = MPU9250_ACCEL_XOUT_H;
  return HAL_I2C_Master_Transmit_DMA(hi2c_MPU9250, MPU9250_ADDR, &buf, 1);
}

HAL_StatusTypeDef MPU9250_accelReadDone(uint8_t *buf)
{
  return HAL_I2C_Master_Receive_DMA(hi2c_MPU9250, MPU9250_ADDR, buf, 6);
}

void MPU9250_setGyroSensitivity(uint8_t val)
{
  uint8_t buf[2];

  *buf = MPU9250_GYRO_CONFIG;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);

  if (val > 0 && val < 4)
  {
    buf[1] = (buf[0] & 0xE7) | (val << 3);
  }
  else
  {
    buf[1] = (buf[0] & 0xE7);
  }

  *buf = MPU9250_GYRO_CONFIG;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 2, 5);
}

void MPU9250_setAccelRange(uint8_t val)
{
  uint8_t buf[2];

  *buf = MPU9250_ACCEL_CONFIG;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_ADDR, buf, 1, 5);

  if (val > 0 && val < 4)
  {
    buf[1] = (buf[0] & 0xE7) | (val << 3);
  }
  else
  {
    buf[1] = (buf[0] & 0xE7);
  }

  *buf = MPU9250_ACCEL_CONFIG;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 2, 5);
}

void MPU9250_setSamplingRate(uint8_t sampleRateDiv)
{
  uint8_t buf[2];

  buf[0] = MPU9250_SMPLRT_DIV;
  buf[1] = sampleRateDiv;

  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_ADDR, buf, 2, 5);
}

//======== mag ==============

uint8_t MPU9250_getMagId(void)
{
  uint8_t buf[2];
  buf[0] = MPU9250_MAG_WIA;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_MAG_ADDR, buf, 1, 5);
  return buf[0]; //should be 0x48
}

//Set the mag to single measurement mode
//can take between 7.3ms to 9ms before data is ready
HAL_StatusTypeDef MPU9250_magStartSingleMeasurement(void)
{
  uint8_t buf[2];
  *buf = MPU9250_MAG_CNTL;
  buf[1] = 0x01; //single measurement mode
  return HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, buf, 2, 5);
}

HAL_StatusTypeDef MPU9250_magStart(void)
{
  uint8_t buf[2];
  *buf = MPU9250_MAG_CNTL;
  buf[1] = 0x02; //continuous measurement mode
  return HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, buf, 2, 5);
}

HAL_StatusTypeDef MPU9250_magReadStart(void)
{
  uint8_t buf = MPU9250_MAG_XOUT_L;
  return HAL_I2C_Master_Transmit_DMA(hi2c_MPU9250, MPU9250_MAG_ADDR, &buf, 1);
}

HAL_StatusTypeDef MPU9250_magReadDone(uint8_t *rx_buf)
{
  mpu9250_mag_data_ptr = rx_buf;
  return HAL_I2C_Master_Receive_DMA(hi2c_MPU9250, MPU9250_MAG_ADDR, rx_buf, 6);
}

HAL_StatusTypeDef MPU9250_magStatusStart(void)
{
  uint8_t buf = MPU9250_MAG_ST2;
  return HAL_I2C_Master_Transmit_DMA(hi2c_MPU9250, MPU9250_MAG_ADDR, &buf, 1);
}

HAL_StatusTypeDef MPU9250_magStatusDone(void)
{
  uint8_t status = MPU9250_MAG_ST2;
  HAL_StatusTypeDef ret_val;
  ret_val = HAL_I2C_Master_Receive_DMA(hi2c_MPU9250, MPU9250_MAG_ADDR, &status, 1);

  if (status)
  {
    //either a read error or mag sensor overflow occurred
    mpu9250_mag_data_ptr[0] = 0xFF;
    mpu9250_mag_data_ptr[1] = 0x7F;
    mpu9250_mag_data_ptr[2] = 0xFF;
    mpu9250_mag_data_ptr[3] = 0x7F;
    mpu9250_mag_data_ptr[4] = 0xFF;
    mpu9250_mag_data_ptr[5] = 0x7F;
  }
  return ret_val;
}

//put x, y and z mag values in buf (little endian)
//-4096 to 4095
//if values are 32767 they are not valid
//either due to data read error or magnetic sensor overflow
void MPU9250_getMag(uint8_t *buf)
{
  uint8_t status;

  *buf = MPU9250_MAG_XOUT_L;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, buf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_MAG_ADDR, buf, 6, 5);

  //check status register
  status = MPU9250_MAG_ST2;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, &status, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_MAG_ADDR, &status, 1, 5);
  if (status)
  {
    //either a read error or mag sensor overflow occurred
    buf[0] = 0xFF;
    buf[1] = 0x7F;
    buf[2] = 0xFF;
    buf[3] = 0x7F;
    buf[4] = 0xFF;
    buf[5] = 0x7F;
  }
}

//read the x, y and z mag sensitivity adjustment values
void MPU9250_getMagSensitivityAdj(uint8_t *buf)
{
  uint8_t localbuf[2];

  //ensure starting from power down mode
  *localbuf = MPU9250_MAG_CNTL;
  localbuf[1] = 0x00; //powerdown mode
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, localbuf, 2, 5);
  HAL_Delay(1); //100us

  //set to fuse ROM mode
  *localbuf = MPU9250_MAG_CNTL;
  localbuf[1] = 0x0F; //Fuse ROM mode
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, localbuf, 2, 5);

  //read sensitivity adjustment data for the 3 axes
  *buf = MPU9250_MAG_ASAX;
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, localbuf, 1, 5);
  HAL_I2C_Master_Receive(hi2c_MPU9250, MPU9250_MAG_ADDR, buf, 3, 5);

  //return to power down mode
  *localbuf = MPU9250_MAG_CNTL;
  localbuf[1] = 0x00; //powerdown mode
  HAL_I2C_Master_Transmit(hi2c_MPU9250, MPU9250_MAG_ADDR, localbuf, 2, 5);
  HAL_Delay(1); //100us
}
