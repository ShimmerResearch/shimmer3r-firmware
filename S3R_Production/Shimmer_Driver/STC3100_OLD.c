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
 * @date July, 2016
 */

#include "stm32f7xx_hal.h"
#include "STC3100.h"

I2C_HandleTypeDef *hi2c_STC3100;

/**Variable that contains the register values of the entire chip.*/
tSTC31000Data STC3100Data;
/**Stores the battery data.*/
tBatteryData BatteryData;

// User defined local variables
unsigned int high_byte = 0;
uint8_t twosC = 0;
int value = 0;

void STC3100_init(I2C_HandleTypeDef *hi2c) {

   hi2c_STC3100 = hi2c;
}

void STC3100_wake(int wakeup) {

   uint8_t pData[2];
   pData[0] = REG_CTRL_STARTUP;
   pData[1] = (wakeup) ? REG_MODE_STARTUP_14B : DEVICE_SHUTDOWN;
   // pData[1] = (wakeup) ? REG_MODE_ADC_CALIBRATION : DEVICE_SHUTDOWN;

   // reset accumulator & counter and clear the Power On Reset DETection (PORDET) bit
   HAL_I2C_Mem_Write(hi2c_STC3100, STC3100_ADDR, STC3100_REG_CTRL, 1, &pData[0], 1, TIMEOUT);

   // start the stc3100 in 14-bit resolution mode
   HAL_I2C_Mem_Write(hi2c_STC3100, STC3100_ADDR, STC3100_REG_MODE, 1, &pData[1], 1, TIMEOUT);

   // Read initial battery voltage to determine capacity:
   STC3100_readChip();

   while (BatteryData.voltage < 1000.0f) {
      STC3100_updateVoltage();
      HAL_Delay(100);
   }

   // Assuming FS: 3.2 Volts -> 4.0 Volts
   BatteryData.chargeInitial = (BatteryData.voltage - 3200.0f) * 0.45f / 0.8f;

}

/**
 * Read the entire chip memory to the #STC3100Data variable.
 */
HAL_StatusTypeDef STC3100_readChip(void)
{
   return HAL_I2C_Mem_Read_IT(hi2c_STC3100, STC3100_ADDR, STC3100_REG_MODE, 1, &(STC3100Data.ByteArray[0]), 12);
}

/**
 * Update the Voltage, Current, Temperature and Charge values based on the actual
 * values present on the #STC3100Data variable and stores that values on the
 * #BatteryData variable.
 */
void STC3100_updateBatteryData(void)
{
   STC3100_updateVoltage();
   STC3100_updateCurrent();
   STC3100_updateTemperature();
   STC3100_updateCharge();
   STC3100_updateBattPerc();
   STC3100_readChip();
}

void STC3100_updateVoltage(void) {

   high_byte = (unsigned int) STC3100Data.VoltageHigh;
   high_byte <<= 8;
   value = (high_byte & 0x0700) | STC3100Data.VoltageLow;
   // convert from 2's complement
   twosC = (value & (1 << 11)) != 0;
   value = (twosC) ? (((value & 0x0700) ^ 0x0700) + 1) : value;
   BatteryData.voltage = twosC ?
                         -(float) value * 2.44f :
                         (float) value * 2.44f;
}

void STC3100_updateCurrent(void) {

   high_byte = (unsigned int) STC3100Data.CurrentHigh;
   high_byte <<= 8;
   value = (high_byte & 0x3F00) | STC3100Data.CurrentLow;
   // convert from 2's complement
   twosC = (value & (1 << 13)) != 0;
   value = (twosC) ? (((value & 0x3FFF) ^ 0x3FFF) + 1) : value;
   value <<= 2;
   BatteryData.current = twosC ?
                         -((((float) value * 11.77f) / 33.0f) / 4.0f) :
                         ((((float) value * 11.77f) / 33.0f) / 4.0f);
   BatteryData.current += (twosC) ? 10.0f : -10.0f;
}

void STC3100_updateTemperature(void) {

   high_byte = (unsigned int) STC3100Data.TemperatureHigh;
   high_byte <<= 8;
   value = (high_byte & 0x0700) | STC3100Data.TemperatureLow;
   value <<= 4;
   BatteryData.temperature = ((float) value * 0.125f) / 16.0f;
}

void STC3100_updateCharge(void) {

   high_byte = (unsigned int) STC3100Data.ChargeHigh;
   high_byte <<= 8;
   value = (high_byte & 0xFF00) | STC3100Data.ChargeLow;
   // convert from 2's complement
   twosC = (value & (1 << 15)) != 0;
   value = (twosC) ? (((value & 0xFFFF) ^ 0xFFFF) + 1) : value;
   BatteryData.charge = (twosC) ?
                        -((float) value * 6.70f) / 33.0f :
                        ((float) value * 6.70f) / 33.0f;

   // time remaining in minutes
   BatteryData.timeLeft = (BatteryData.current < 0) ?
                          -60.0f * ((BatteryData.chargeInitial + BatteryData.charge) / BatteryData.current) :
                          60.0f * ((BatteryData.chargeInitial + BatteryData.charge) / BatteryData.current);
}

void STC3100_updateBattPerc(void) {
   BatteryData.battPerc = 100.0f * (BatteryData.chargeInitial + BatteryData.charge) / 450.0f;
}

