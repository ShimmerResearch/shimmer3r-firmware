/*Copyright (c) 2017, Shimmer Research, Ltd.
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
 * @date December, 2017

 * @author Mark Nolan
 * @date December, 2024
 *
 * RemoteSweeper_v0.1.0
 *
 * Based on:
 * https://github.com/ShimmerEngineering/FW_Sandbox_CCS
 */

#include "i2c.h"
#include <GSRTestRig/gsrTestRig.h>

static uint8_t switchModeCurrent = 0;

void *handle;

/*
 *  ADG715 Switch Select
 */
void setADG715SwitchMode(uint8_t value)
{
  switchModeCurrent = value;
  HAL_I2C_Master_Transmit(handle, ADDR_U3_ADG715, &value, 1, 1000);
}

void gsrTestRigInit(I2C_HandleTypeDef *hi2c)
{
  handle = hi2c;
  //Configure Switch
  setADG715SwitchMode(ADG715_SWITCH_RESET);
}

/* TODO implement parallel connected resistors in various combinations (i.e.
 * 2-4 in parallel, 2-3 in parallel with 1-2 in series) */
void setGsrTestRigResistance(uint32_t resistance)
{
  uint8_t U1_RDAC1 = 0, U1_RDAC2 = 0, U2_RDAC1 = 0, U2_RDAC2 = 0;
  uint32_t res_u1_1 = 0, res_u1_2 = 0, res_u2_1 = 0, res_u2_2 = 0;

  uint8_t switchModeToSet = ADG715_SETTING;

  setADG715SwitchMode(ADG715_SWITCH_RESET);

  if (resistance > 500000)
  {
    if (resistance > (3 * ADG715_1M_TOTAL_RES))
    {
      //Connect all resistors in series
      switchModeToSet = ADG715_S2 | ADG715_S4 | ADG715_S5 | ADG715_S7;

      res_u1_1 = ADG715_1M_TOTAL_RES;
      res_u1_2 = ADG715_1M_TOTAL_RES;
      res_u2_1 = ADG715_1M_TOTAL_RES;
      res_u2_2 = resistance - (3 * ADG715_1M_TOTAL_RES);
    }
    else if (resistance > (2 * ADG715_1M_TOTAL_RES))
    {
      //Connect U1.1, U1.2 and U2.1 resistors in series
      switchModeToSet = ADG715_S5 | ADG715_S2 | ADG715_S7 | ADG715_S8;

      res_u1_1 = ADG715_1M_TOTAL_RES;
      res_u1_2 = ADG715_1M_TOTAL_RES;
      res_u2_1 = resistance - (2 * ADG715_1M_TOTAL_RES);
      res_u2_2 = 0;
    }
    else if (resistance > (1 * ADG715_1M_TOTAL_RES))
    {
      //Connect U1.1 and U1.2 resistors in series
      switchModeToSet = ADG715_S5 | ADG715_S2 | ADG715_S3 | ADG715_S4;

      res_u1_1 = ADG715_1M_TOTAL_RES;
      res_u1_2 = resistance - ADG715_1M_TOTAL_RES;
      res_u2_1 = 0;
      res_u2_2 = 0;
    }
    else if (resistance > 500000)
    {
      //Connect U1.1 resistor
      switchModeToSet = ADG715_S5 | ADG715_S6 | ADG715_S7 | ADG715_S8;

      res_u1_1 = resistance;
      res_u1_2 = 0;
      res_u2_1 = 0;
      res_u2_2 = 0;
    }
  }
  else
  {
    //TODO implement parallel connected resistors. For the moment, utilising a single resistor

    //Connect U2.2 resistor
    switchModeToSet = ADG715_S1 | ADG715_S2 | ADG715_S3 | ADG715_S8;

    res_u1_1 = resistance;
    res_u1_2 = 0;
    res_u2_1 = 0;
    res_u2_2 = 0;
  }

  U1_RDAC1 = calculateRdacValue(res_u1_1);
  U1_RDAC2 = calculateRdacValue(res_u1_2);
  U2_RDAC1 = calculateRdacValue(res_u2_1);
  U2_RDAC2 = calculateRdacValue(res_u2_2);

  setGsrTestRig(switchModeToSet, U1_RDAC1, U1_RDAC2, U2_RDAC1, U2_RDAC2);
}

uint8_t calculateRdacValue(uint32_t resistance)
{
  if (resistance < ADG715_1M_TOTAL_RES_MIN)
  {
    return 255;
  }

  float result = (256.0 - ((((float) resistance - 60.0) / (float) ADG715_1M_RAB) * 256.0));
  return (uint8_t) result;
}

void setGsrTestRig(uint8_t switchMode, uint8_t u1Rdac1, uint8_t u1Rdac2, uint8_t u2Rdac1, uint8_t u2Rdac2)
{
  if (switchMode != switchModeCurrent)
  {
    //Configure Switch
    setADG715SwitchMode(switchMode);
  }

  //Now deal with other 4 bytes:
  uint8_t buf[2];
  buf[0] = RDAC1;
  buf[1] = u1Rdac1;
  HAL_I2C_Master_Transmit(handle, ADDR_U1_AD5242, buf, 2, 1000);

  buf[0] = RDAC2;
  buf[1] = u1Rdac2;
  HAL_I2C_Master_Transmit(handle, ADDR_U1_AD5242, buf, 2, 1000);

  buf[0] = RDAC1;
  buf[1] = u2Rdac1;
  HAL_I2C_Master_Transmit(handle, ADDR_U2_AD5242, buf, 2, 1000);

  buf[0] = RDAC2;
  buf[1] = u2Rdac2;
  HAL_I2C_Master_Transmit(handle, ADDR_U2_AD5242, buf, 2, 1000);
}

void run_gsrTestRig(uint8_t *config)
{
  setGsrTestRig(config[0], config[1], config[2], config[3], config[4]);
}

void digital_pot(void)
{
  int digitalval = 255, digitalval1 = 0; //Digital value (see AD5242 data sheet//
  unsigned int i = 0;
  uint8_t buf[255], buf1[255];
  memset(buf, 0, 255);
  memset(buf1, 0, 255);
  buf[0] = RDAC1; //ensure I2C_MST_EN is 0
  for (i = 1; i < 255; i++)
  {
    buf[i] = digitalval - 1;
    digitalval--;
    buf1[i] = digitalval1 + 1;
    digitalval1++;
  }
  HAL_I2C_Master_Transmit(handle, ADDR_U1_AD5242, buf, 254, 1000);
  buf1[0] = RDAC1; //ensure I2C_MST_EN is 0
  HAL_I2C_Master_Transmit(handle, ADDR_U1_AD5242, buf1, 254, 1000);
}
