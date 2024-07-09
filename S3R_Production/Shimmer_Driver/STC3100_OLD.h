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

#ifndef STC3100_H
#define STC3100_H

#pragma anon_unions

#include "stm32u5xx_hal.h"
#include <stdint.h>

#define STC3100_ADDR             0x70 << 1 //7 bit address I2C address of the STC3100

//Control registers
#define STC3100_REG_MODE         0x00
#define STC3100_REG_CTRL         0x01
#define STC3100_REG_CHARGE_LOW   0x02
#define STC3100_REG_CHARGE_HIGH  0x03
#define STC3100_REG_COUNTER_LOW  0x04
#define STC3100_REG_COUNTER_HIGH 0x05
#define STC3100_REG_CURRENT_LOW  0x06
#define STC3100_REG_CURRENT_HIGH 0x07
#define STC3100_REG_VOLTAGE_LOW  0x08
#define STC3100_REG_VOLTAGE_HIGH 0x09
#define STC3100_REG_TEMP_LOW     0x0A
#define STC3100_REG_TEMP_HIGH    0x0B

//Device ID Registers
#define STC3100_REG_ID0          0x18 //part type ID = 10h
#define STC3100_REG_ID7          0x1F //device ID CRC

//REG operations
#define REG_CTRL_STARTUP         0x02 //0b 0000 0010
#define REG_MODE_STARTUP_14B     0x10 //0b 0001 0000 - 14 bit resolution
#define REG_MODE_STARTUP_13B     0x12 //0b 0001 0010 - 13 bit resolution
#define REG_MODE_STARTUP_12B     0x14 //0b 0001 0100 - 12 bit resolution
#define REG_MODE_ADC_CALIBRATION 0x18 //0b 0001 1000
#define DEVICE_SHUTDOWN          0x00

//I2C operations
#define TIMEOUT                  0xFF

//Battery gas gauge RAM register
#define REG_GAS_GAUGE            0x32


extern void STC3100_init(I2C_HandleTypeDef *hi2c);

/**
 * Register map of the STC3100 chip. All the accessible registers are mapped here.
 */
typedef union
{
  /**Used for simpler readings of the chip.*/
  unsigned char ByteArray[64];

  struct
  {
    /**Mode register*/
    unsigned char Mode;
    /**Control and status register*/
    unsigned char ControlStatus;
    /**Gas gauge charge data, bits 0-7*/
    unsigned char ChargeLow;
    /**Gas gauge charge data, bits 8-15*/
    unsigned char ChargeHigh;
    /**Number of conversions, bits 0-7*/
    unsigned char CounterLow;
    /**Number of conversions, bits 8-15*/
    unsigned char CounterHigh;
    /**Battery current value, bits 0-7*/
    unsigned char CurrentLow;
    /**Battery current value, bits 8-15*/
    unsigned char CurrentHigh;
    /**Battery voltage value, bits 0-7*/
    unsigned char VoltageLow;
    /**Battery voltage value, bits 8-15*/
    unsigned char VoltageHigh;
    /**Temperature value, bits 0-7*/
    unsigned char TemperatureLow;
    /**Temperature value, bits 8-15*/
    unsigned char TemperatureHigh;
    /**Not implemented*/
    unsigned char Reserved[13];
    /**Part type ID = 10h*/
    unsigned char ID0;
    /**Unique part ID, bits 0-7*/
    unsigned char ID1;
    /**Unique part ID, bits 8-15*/
    unsigned char ID2;
    /**Unique part ID, bits 16-23*/
    unsigned char ID3;
    /**Unique part ID, bits 24-31*/
    unsigned char ID4;
    /**Unique part ID, bits 32-39*/
    unsigned char ID5;
    /**Unique part ID, bits 40-47*/
    unsigned char ID6;
    /**Device ID CRC*/
    unsigned char ID7;
    /**General-purpose RAM register 0*/
    unsigned char RAM0;
    /**General-purpose RAM register 1*/
    unsigned char RAM1;
    /**General-purpose RAM register 2*/
    unsigned char RAM2;
    /**General-purpose RAM register 3*/
    unsigned char RAM3;
    /**General-purpose RAM register 4*/
    unsigned char RAM4;
    /**General-purpose RAM register 5*/
    unsigned char RAM5;
    /**General-purpose RAM register 6*/
    unsigned char RAM6;
    /**General-purpose RAM register 7*/
    unsigned char RAM7;
    /**General-purpose RAM register 8*/
    unsigned char RAM8;
    /**General-purpose RAM register 9*/
    unsigned char RAM9;
    /**General-purpose RAM register 10*/
    unsigned char RAM10;
    /**General-purpose RAM register 11*/
    unsigned char RAM11;
    /**General-purpose RAM register 12*/
    unsigned char RAM12;
    /**General-purpose RAM register 13*/
    unsigned char RAM13;
    /**General-purpose RAM register 14*/
    unsigned char RAM14;
    /**General-purpose RAM register 15*/
    unsigned char RAM15;
    /**General-purpose RAM register 16*/
    unsigned char RAM16;
    /**General-purpose RAM register 17*/
    unsigned char RAM17;
    /**General-purpose RAM register 18*/
    unsigned char RAM18;
    /**General-purpose RAM register 19*/
    unsigned char RAM19;
    /**General-purpose RAM register 20*/
    unsigned char RAM20;
    /**General-purpose RAM register 21*/
    unsigned char RAM21;
    /**General-purpose RAM register 22*/
    unsigned char RAM22;
    /**General-purpose RAM register 23*/
    unsigned char RAM23;
    /**General-purpose RAM register 24*/
    unsigned char RAM24;
    /**General-purpose RAM register 25*/
    unsigned char RAM25;
    /**General-purpose RAM register 26*/
    unsigned char RAM26;
    /**General-purpose RAM register 27*/
    unsigned char RAM27;
    /**General-purpose RAM register 28*/
    unsigned char RAM28;
    /**General-purpose RAM register 29*/
    unsigned char RAM29;
    /**General-purpose RAM register 30*/
    unsigned char RAM30;
    /**General-purpose RAM register 31*/
    unsigned char RAM31;
  };

  /**
   * Mode register bits.
   */
  struct
  {
    /**32,768 Hz clock source: 0: auto-detect, 1: external clock*/
    unsigned char ClockSource : 1;
    /**Gas gauge ADC resolution: 00:14 bits, 01:13 bits, 10:12 bits*/
    unsigned char GasGaugeADCResolution : 2;
    /**ADC Calibration: 0: no effect, 1: used to calibrate the AD converters*/
    unsigned char CalibrateAD : 1;
    /**
     * 0: standby mode. Accumulator and counter registers are frozen, gas
     * gauge and battery monitor functions are in standby.
     * 1: operating mode.
     */
    unsigned char StandbyOperating : 1;
    /**Unused*/
    unsigned char : 3;
  } MODEbits;

  /**
   * Control / Status Register detailed.
   */
  struct
  {
    /**Unused*/
    unsigned char : 8;
    /**
     * Port IO0 data status:
     * 0 = IO0 input is low
     * 1 = IO0 input is high
     *
     * Port IO0 data output drive:
     * 0 = IO0 output is driven low
     * 1 = IO0 output is open
     */
    unsigned char IO0Data : 1;
    /**
     * Gas Gauge Reset
     * 0: no effect
     * 1: resets the charge accumulator and conversion counter
     * GG_RST is a self-clearing bit.
     */
    unsigned char GaugeReset : 1;
    /**
     * Set at the end of a battery current conversion cycle.
     * Clears upon reading.
     */
    unsigned char CurrentConvertionStatus : 1;
    /**
     * Set at the end of a battery voltage or temperature conversion cycle.
     * Clears upon reading.
     */
    unsigned char VoltageTemperatureConvertionStatus : 1;
    /**
     * Power on reset (POR) detection bit:
     * 0 = no POR event occurred,
     * 1 = POR event occurred
     *
     * Soft reset:
     * 0 = release the soft-reset and clear the POR detection bit,
     * 1 = assert the soft-reset and set the POR detection bit
     */
    unsigned char PowerOnReset : 1;
    /**Unused*/
    unsigned char : 3;
  } CONTROLSTATUSbits;


} tSTC31000Data;


//Struct that stores the updated values.

typedef struct
{
  /**Value of the voltage in mV.*/
  float voltage;
  /**Value of the current in mA*/
  float current;
  /**Value of the temperature in �C*/
  float temperature;
  /**Value of the current charge of the battery in mAh*/
  float charge;
  /**Initial value of the current charge of the battery in mAh*/
  float chargeInitial;
  /**Gas gauge value of battery charge in percentage*/
  float battPerc;
  /**Gas gauge value of battery charge in minutes remaining*/
  float timeLeft;
} tBatteryData;

extern tSTC31000Data STC3100Data;
extern tBatteryData BatteryData;

//Read all 64 bytes of data from the chip and store in STC3100Data
HAL_StatusTypeDef STC3100_readChip(void);

void STC3100_updateVoltage(void);
void STC3100_updateCurrent(void);
void STC3100_updateTemperature(void);
void STC3100_updateCharge(void);
void STC3100_updateBattPerc(void);

void STC3100_updateBatteryData(void);

//If wakeup is 0 puts STC3100 to sleep
//else wakes it up
void STC3100_wake(int wakeup);


#endif //STC3100_H
