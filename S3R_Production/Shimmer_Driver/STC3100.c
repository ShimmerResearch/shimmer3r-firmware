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
 * @author Sam O'Mahony
 * @date August, 2016
 */

#include "STC3100.h"
#include "stm32u5xx_hal.h"

I2C_HandleTypeDef *hi2c_STC3100;

/**Variable that contains the register values of the entire chip.*/
tSTC31000Data STC3100Data;


void STC3100_init(I2C_HandleTypeDef *hi2c)
{

  hi2c_STC3100 = hi2c;
}

//void STC3100_wake(int wakeup) {

//uint8_t pData[2];
//pData[0] = REG_CTRL_STARTUP;
//pData[1] = (wakeup) ? REG_MODE_STARTUP_14B : DEVICE_SHUTDOWN;
//// pData[1] = (wakeup) ? REG_MODE_ADC_CALIBRATION : DEVICE_SHUTDOWN;

//// reset accumulator & counter and clear the Power On Reset DETection (PORDET) bit
//HAL_I2C_Mem_Write(hi2c_STC3100, STC3100_ADDR, STC3100_REG_CTRL, 1, &pData[0], 1, TIMEOUT);

//// start the stc3100 in 14-bit resolution mode
//HAL_I2C_Mem_Write(hi2c_STC3100, STC3100_ADDR, STC3100_REG_MODE, 1, &pData[1], 1, TIMEOUT);

//// Read initial battery voltage to determine capacity:
//STC3100_readChip();
//
//HAL_Delay(10);
//
//HAL_I2C_Mem_Write(hi2c_STC3100, STC3100_ADDR, STC3100_REG_VOLT_INITIAL, 1, &(STC3100Data.VoltageLow), 2, TIMEOUT);
//
//HAL_Delay(10);
//}
void STC3100_wake(int wakeup)
{
  uint8_t data;
  if (wakeup)
  {
    data = REG_CTRL_STARTUP;
    HAL_I2C_Mem_Write(hi2c_STC3100, STC3100_ADDR, STC3100_REG_MODE, 1, &data, 1, STC3100_TIMEOUT);
    //start the stc3100 in 14-bit resolution mode
    data = REG_MODE_STARTUP_14B;
    HAL_I2C_Mem_Write(hi2c_STC3100, STC3100_ADDR, STC3100_REG_MODE, 1, &data, 1, STC3100_TIMEOUT);
  }
  else
  {
    data = DEVICE_SHUTDOWN;
    HAL_I2C_Mem_Write(hi2c_STC3100, STC3100_ADDR, STC3100_REG_MODE, 1, &data, 1, STC3100_TIMEOUT);
  }
}

/**
 * Read the entire chip memory to the #STC3100Data variable.
 */
void STC3100_readChip(void)
{
  HAL_I2C_Mem_Read_IT(hi2c_STC3100, STC3100_ADDR, STC3100_REG_CHARGE_LOW, 1,
      &(STC3100Data.ByteArray[0]), 23);
}

void STC3100_readData(uint8_t *buf)
{
  HAL_I2C_Mem_Read(hi2c_STC3100, STC3100_ADDR, STC3100_REG_CHARGE_LOW, 1, buf,
      STC3100_DATA_LEN, STC3100_TIMEOUT);
}
void STC3100_readData_it(uint8_t *buf)
{
  HAL_I2C_Mem_Read_IT(hi2c_STC3100, STC3100_ADDR, STC3100_REG_CHARGE_LOW, 1,
      buf, STC3100_DATA_LEN);
}
