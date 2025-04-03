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
 * @author Mike Healy
 * @date December, 2013
 */

#include "exg.h"
#include <string.h>

static uint8_t data[9];

uint8_t EXG_init(SPI_HandleTypeDef *hspi)
{
  /*ADS1292_disableDrdyInterrupts(ADS1292_DRDY_INT_CHIP1 + ADS1292_DRDY_INT_CHIP2);
  ADS1292_init(hspi);
  ADS1292_resetPulse();
  ADS1292_chip1CsEnable(1);
  ADS1292_readDataContinuousMode(0);
  ADS1292_chip2CsEnable(1);
  ADS1292_readDataContinuousMode(0);
  ADS1292_chip2CsEnable(0);*/
  // ADS1292_init();
  HAL_StatusTypeDef res = HAL_OK;
  setSpiHandle(hspi);
  ADS1292_resetPulse();
  HAL_Delay(5);
  ADS1292_chip1CsEnable(1);
  res = ADS1292_readDataContinuousMode(0);
  if (res != HAL_OK)
  {
    ADS1292_chip1CsEnable(0);
    return 1;
  }
  ADS1292_chip2CsEnable(1);
  res = ADS1292_enableInternalReference();
  if (res != HAL_OK)
  {
    ADS1292_chip1CsEnable(0);
    return 1;
  }
  res = ADS1292_readDataContinuousMode(0);
  ADS1292_chip2CsEnable(0);
  if (res != HAL_OK)
  {
    return 1;
  }
  return 0;
}

uint8_t EXG_self_test(void)
{
  uint8_t temp_buf[13] = {0,};
  uint8_t ret_val = 0;
  HAL_StatusTypeDef res = HAL_OK;
  res = EXG_readRegs(0, ADS1292R_DEVID, 1, &temp_buf[0]);
  if(res != HAL_OK)
  {
    return 0xFF;
  }
  else if (temp_buf[0] != (uint8_t) 0x73)
  {
     ret_val |= 0x01;
  }
  memset(temp_buf, 0, 13);
  res = EXG_readRegs(1, ADS1292R_DEVID, 1, &temp_buf[0]);
  if (res != HAL_OK)
  {
    return 0xFF;
  }
  else if (temp_buf[0] != (uint8_t) 0x73)
  {
    ret_val |= 0x02;
  }
  return ret_val;
}

void EXG_setRdatac(uint8_t chip, uint8_t en)
{
  if (chip)
  {
    ADS1292_chip2CsEnable(1);
    ADS1292_readDataContinuousMode(en);
    ADS1292_chip2CsEnable(0);
  }
  else
  {
    ADS1292_chip1CsEnable(1);
    ADS1292_readDataContinuousMode(en);
    ADS1292_chip1CsEnable(0);
  }
}

void EXG_start(uint8_t chip)
{
  if (!chip)
  {
    ADS1292_chip1CsEnable(1);
    ADS1292_readDataContinuousMode(1);
    ADS1292_enableDrdyInterrupts(ADS1292_DRDY_INT_CHIP1);
    ADS1292_start(1);
    ADS1292_chip1CsEnable(0);
  }
  else if (chip == 1)
  {
    ADS1292_chip2CsEnable(1);
    ADS1292_readDataContinuousMode(1);
    ADS1292_enableDrdyInterrupts(ADS1292_DRDY_INT_CHIP2);
    ADS1292_start(1);
    ADS1292_chip2CsEnable(0);
  }
  else
  {
    //Put both chips in RDATAC mode then start both one after
    //the other as quickly as possible
    ADS1292_chip2CsEnable(1);
    ADS1292_readDataContinuousMode(1);
    ADS1292_chip1CsEnable(1);
    ADS1292_readDataContinuousMode(1);
    ADS1292_enableDrdyInterrupts(ADS1292_DRDY_INT_CHIP1 + ADS1292_DRDY_INT_CHIP2);
    ADS1292_start(1);
    ADS1292_chip2CsEnable(1);
    ADS1292_start(1);
    ADS1292_chip2CsEnable(0);
  }
}

//stop ADC1292R chip sampling and put in SDATAC mode
//also disable data ready interrupt
void EXG_stop(uint8_t chip)
{
  if (chip)
  {
    ADS1292_disableDrdyInterrupts(ADS1292_DRDY_INT_CHIP2);
    ADS1292_chip2CsEnable(1);
    ADS1292_start(0);
    ADS1292_readDataContinuousMode(0);
    ADS1292_chip2CsEnable(0);
  }
  else
  {
    ADS1292_disableDrdyInterrupts(ADS1292_DRDY_INT_CHIP1);
    ADS1292_chip1CsEnable(1);
    ADS1292_start(0);
    ADS1292_readDataContinuousMode(0);
    ADS1292_chip1CsEnable(0);
  }
}

//power off both ExG chips
//void EXG_powerOff(void) {
//   ADS1292_powerOff();
//}

void EXG_resetRegs(uint8_t chip)
{
  if (chip)
  {
    ADS1292_chip2CsEnable(1);
    ADS1292_resetRegs();
    ADS1292_chip2CsEnable(0);
  }
  else
  {
    ADS1292_chip1CsEnable(1);
    ADS1292_resetRegs();
    ADS1292_chip1CsEnable(0);
  }
}

void EXG_offsetCal(uint8_t chip)
{
  if (chip)
  {
    ADS1292_chip2CsEnable(1);
    ADS1292_offsetCal();
    ADS1292_chip2CsEnable(0);
  }
  else
  {
    ADS1292_chip1CsEnable(1);
    ADS1292_offsetCal();
    ADS1292_chip1CsEnable(0);
  }
}

HAL_StatusTypeDef EXG_readRegs(uint8_t chip, uint8_t startaddress, uint8_t size, uint8_t *rdata)
{
  HAL_StatusTypeDef res = HAL_OK;
  if (chip)
  {
    ADS1292_chip2CsEnable(1);
  }
  else
  {
    ADS1292_chip1CsEnable(1);
  }
  res = ADS1292_regRead(startaddress, size, rdata);
  if (chip)
  {
    ADS1292_chip2CsEnable(0);
  }
  else
  {
    ADS1292_chip1CsEnable(0);
  }
  return res;
}

HAL_StatusTypeDef EXG_writeRegs(uint8_t chip, uint8_t startaddress, uint8_t size, uint8_t *wdata)
{
  HAL_StatusTypeDef res = HAL_OK;
  if (chip)
  {
    ADS1292_chip2CsEnable(1);
  }
  else
  {
    ADS1292_chip1CsEnable(1);
  }
  res = ADS1292_regWrite(startaddress, size, wdata);
  if (chip)
  {
    ADS1292_chip2CsEnable(0);
  }
  else
  {
    ADS1292_chip1CsEnable(0);
  }
  return res;
}

void EXG_readData(uint8_t chip, uint8_t size, uint8_t *buf)
{
  if (chip)
  {
    if (ADS1292_readDataChip2(data))
    {
      //valid data
      *buf = ((*data & 0x4F) << 1) + ((*(data + 1) & 0x80) >> 7);
      if (size)
      {
        //16-bit
        buf[1] = (uint8_t) (((data[4] >> 7) & 0x01) + ((data[3] << 1) & 0x7E))
            + (data[3] & 0x80);
        buf[2] = (uint8_t) (((data[5] >> 7) & 0x01) + ((data[4] << 1) & 0xFE));
        buf[3] = (uint8_t) (((data[7] >> 7) & 0x01) + ((data[6] << 1) & 0x7E))
            + (data[6] & 0x80);
        buf[4] = (uint8_t) (((data[8] >> 7) & 0x01) + ((data[7] << 1) & 0xFE));
      }
      else
      {
        //24-bit
        memcpy(buf + 1, data + 3, 6);
      }
    }
    else
    {
      *buf &= 0x7F;
    }
  }
  else
  {
    if (ADS1292_readDataChip1(data))
    {
      //valid data
      *buf = ((*data & 0x4F) << 1) + ((*(data + 1) & 0x80) >> 7);
      if (size)
      {
        //16-bit
        buf[1] = (uint8_t) (((data[4] >> 7) & 0x01) + ((data[3] << 1) & 0xFE));
        buf[2] = (uint8_t) (((data[5] >> 7) & 0x01) + ((data[4] << 1) & 0xFE));
        buf[3] = (uint8_t) (((data[7] >> 7) & 0x01) + ((data[6] << 1) & 0xFE));
        buf[4] = (uint8_t) (((data[8] >> 7) & 0x01) + ((data[7] << 1) & 0xFE));
      }
      else
      {
        //24-bit
        memcpy(buf + 1, data + 3, 6);
      }
    }
    else
    {
      *buf &= 0x7F;
    }
  }
}

//void EXG_dataReadyChip1() {
//   ADS1292_dataReadyChip1();
//}
//void EXG_dataReadyChip2() {
//   ADS1292_dataReadyChip2();
//}
