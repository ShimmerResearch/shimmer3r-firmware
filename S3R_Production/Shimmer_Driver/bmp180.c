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

#include "bmp180.h"
#include "stm32u5xx_hal.h"

static int16_t AC1, AC2, AC3, B1, B2, MB, MC, MD;
static uint16_t AC4, AC5, AC6, UT;
static int32_t X1, X2, X3, B3, B5, B6, p; //T,
static uint32_t B4, B7, UP;
static uint8_t BMP180_lib_initialized = 0;

I2C_HandleTypeDef *hi2c_BMP180;

//configure I2C
void BMP180_init(I2C_HandleTypeDef *hi2c)
{
  hi2c_BMP180 = hi2c;
}

uint8_t BMP180_getId(void)
{
  uint8_t buf = BMP180_ID;
  HAL_I2C_Master_Transmit(hi2c_BMP180, BMP180_ADDR, &buf, 1, 1000);
  HAL_I2C_Master_Receive(hi2c_BMP180, BMP180_ADDR, &buf, 1, 1000);
  return buf;
}

HAL_StatusTypeDef BMP180_getCalib(uint8_t *rx_buf)
{
  *rx_buf = BMP180_AC1_MSB;
  HAL_StatusTypeDef status;
  uint8_t i = 0;
  HAL_I2C_Master_Transmit(hi2c_BMP180, BMP180_ADDR, rx_buf, 1, 1000);
  status = HAL_I2C_Master_Receive(hi2c_BMP180, BMP180_ADDR, rx_buf, 22, 1000);
  //rx_buf[] = 1c01fb84c7a87d8e60354165157a003d8000d4bd0980 as an example :)

  if (status == HAL_OK)
  {
    /* Set configuration values */
    AC1 = (int16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    AC2 = (int16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    AC3 = (int16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    AC4 = (uint16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    AC5 = (uint16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    AC6 = (uint16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    B1 = (int16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    B2 = (int16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    MB = (int16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    MC = (int16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);
    i += 2;
    MD = (int16_t) (rx_buf[i] << 8 | rx_buf[i + 1]);

    BMP180_lib_initialized = 1;
  }
  return status;
}

float BMP180_calcTemp(uint16_t temp_val_16)
{
  if (!BMP180_lib_initialized)
    return 0;
  UT = temp_val_16;
  X1 = (UT - AC6) * AC5 * BMP180_1_32768;
  X2 = MC * 2048 / (X1 + MD);
  B5 = X1 + X2;

  return (B5 + 8) / ((float) 160);
}

uint32_t BMP180_calcPres(uint32_t pres_val_32, uint8_t oss, float *altitude)
{
  if (!BMP180_lib_initialized)
    return 0;

  /* Calculate true pressure */
  UP = pres_val_32 >> (8 - oss);

  B6 = B5 - 4000;
  X1 = (B2 * (B6 * B6 * BMP180_1_4096)) * BMP180_1_2048;
  X2 = AC2 * B6 * BMP180_1_2048;
  X3 = X1 + X2;
  B3 = (((AC1 * 4 + X3) << oss) + 2) * 0.25;
  X1 = AC3 * B6 * BMP180_1_8192;
  X2 = (B1 * (B6 * B6 * BMP180_1_4096)) * BMP180_1_65536;
  X3 = ((X1 + X2) + 2) * 0.25;
  B4 = AC4 * (uint32_t) (X3 + 32768) * BMP180_1_32768;
  B7 = ((uint32_t) UP - B3) * (50000 >> oss);
  if (B7 < 0x80000000)
  {
    p = (B7 * 2) / B4;
  }
  else
  {
    p = (B7 / B4) * 2;
  }
  X1 = ((float) p * BMP180_1_256) * ((float) p * BMP180_1_256);
  X1 = (X1 * 3038) * BMP180_1_65536;
  X2 = (-7357 * p) * BMP180_1_65536;
  p = p + (X1 + X2 + 3791) * BMP180_1_16;

  /* Calculate altitude */
  if (altitude)
    *altitude = (float) 44330.0
        * (float) ((float) 1.0 - (float) pow((float) p * BMP180_1_101325, 0.19029495));

  return p;
}

HAL_StatusTypeDef BMP180_tempStartMeasurement(void)
{
  uint8_t i2c_tx_data[2];
  i2c_tx_data[0] = BMP180_CTRL_MEAS;
  i2c_tx_data[1] = 0x2E;
  return HAL_I2C_Master_Transmit_DMA(hi2c_BMP180, BMP180_ADDR, i2c_tx_data, 2);
}

HAL_StatusTypeDef BMP180_tempReadTx(void)
{
  uint8_t i2c_tx_data = BMP180_OUT_MSB;
  return HAL_I2C_Master_Transmit_DMA(hi2c_BMP180, BMP180_ADDR, &i2c_tx_data, 1);
}

HAL_StatusTypeDef BMP180_tempReadRx(uint8_t *rx_buf)
{
  return HAL_I2C_Master_Receive_DMA(hi2c_BMP180, BMP180_ADDR, rx_buf, 2);
}

HAL_StatusTypeDef BMP180_presStartMeasurement(uint8_t oss)
{
  uint8_t i2c_tx_data[2];
  i2c_tx_data[0] = BMP180_CTRL_MEAS;
  i2c_tx_data[1] = ((oss & 0x03) << 6) + 0x34;
  return HAL_I2C_Master_Transmit_DMA(hi2c_BMP180, BMP180_ADDR, i2c_tx_data, 2);
}

HAL_StatusTypeDef BMP180_presReadTx(void)
{
  uint8_t i2c_tx_data = BMP180_OUT_MSB;
  return HAL_I2C_Master_Transmit_DMA(hi2c_BMP180, BMP180_ADDR, &i2c_tx_data, 1);
}

HAL_StatusTypeDef BMP180_presReadRx(uint8_t *rx_buf)
{
  return HAL_I2C_Master_Receive_DMA(hi2c_BMP180, BMP180_ADDR, rx_buf, 3);
}
