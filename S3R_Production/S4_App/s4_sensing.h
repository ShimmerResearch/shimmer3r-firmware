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
 
#include<stdint.h>

#ifndef S4_SENSING_H
#define S4_SENSING_H

#include "s4.h"
#include "s4__cfg.h"


//uint8_t pDataTs, pDataTemp, pDataPres, pDataLsm303dlhcAccel, pDataLsm303dlhcMag, pDataMpu9250Gyro, pDataMpu9250Accel,
//        pDataMpu9250Mag, pDataAnalogAccel, pDataStc3100Batt, pDataExg1, pDataExg2, pDataGSR, pDataStrain,
//        pDataExtADC_A6, pDataExtADC_A7, pDataExtADC_A15, pDataIntADC_A12, pDataIntADC_A13, pDataIntADC_A14,
//        pDataBatteryAnalog;
        
        
typedef struct {//data ptr (offset)
   uint8_t ts;
   uint8_t temp;
   uint8_t pres;
   uint8_t lsm303dlhcAccel;
   uint8_t lsm303dlhcMag;
   uint8_t mpu9250Gyro;
   uint8_t mpu9250Accel;
   uint8_t mpu9250Mag;
   uint8_t analogAccel;
   uint8_t stc3100Batt;
   uint8_t exg1;
   uint8_t exg2;
   uint8_t gsr;
   uint8_t strainGauge;
   uint8_t ppg;
   uint8_t extADC_9;
   uint8_t extADC_8;
   uint8_t extADC_1;
   uint8_t intADC_10;
   uint8_t intADC_12;
   uint8_t intADC_11;
   uint8_t intADC_0;
   uint8_t intADC_2;
   uint8_t batteryAnalog;
} DATAPTRTypeDef;

typedef struct {//sensor data 
   //uint8_t     en;
   //uint8_t     configuring;
   //uint8_t     i2cSensor;
   uint8_t     isSampling;
   uint8_t     isSdOperating;
   uint8_t     isFileCreated;
   uint8_t     inSdWr;   
   uint16_t    inSdWrCnt;   
   float       freq;
   uint16_t    clkInterval4096;
   uint16_t    clkInterval16k;
   uint8_t     status;
   uint8_t     cc[MAX_NUM_CHANNELS];//channelContents
   uint8_t     ccLen;//channelContentsLength
   uint8_t     nbrAdcChans;
   uint8_t     nbrDigiChans;
   uint8_t     dataLen;
//   uint8_t     sdlogEnabled;
//   uint8_t     btStreamEnabled;
   uint64_t    startTs;
   uint64_t    latestTs;
   uint8_t     dataBuf[100];
   //STATTypeDef stat;
   DATAPTRTypeDef ptr;
} SENSINGTypeDef;

extern SENSINGTypeDef sensing;

void S4Sens_init(void);
SENSINGTypeDef* S4Sens_getSensing(void);
void S4Sens_configureChannels(void);
uint8_t S4Sens_checkStartSensorConditions(void);
uint8_t S4Sens_checkStartLoggingConditions(void);
uint8_t S4Sens_checkStartStreamingConditions(void);
uint8_t S4Sens_checkStopSensorConditions(void);
uint8_t S4Sens_checkStopLoggingConditions(void);
void S4Sens_startSensing(void);
void S4Sens_stopSensing(void);
void S4Sens_stopPeripherals(void);
void S4Sens_streamData(void);
void S4Sens_bufPoll(void);
void S4Sens_gatherData(void);

void S4Sens_stepInit(void);
void S4Sens_step1Start(void);
void S4Sens_step2Start(void);
void S4Sens_step3Start(void);
void S4Sens_step4Start(void);
void S4Sens_step5Start(void);
void S4Sens_stepDone(void);

#endif // S4_SENSING_H
