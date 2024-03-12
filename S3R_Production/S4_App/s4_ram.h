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

#ifndef S4_RAM_H
#define S4_RAM_H

#include "s4.h"
#include "s4__cfg.h"

#define STOREDCONFIG_SIZE 512

typedef struct __attribute__((packed)) gConfigBytes
{
	//cfg in common
	uint16_t samplingRate;
	uint8_t bufferSize;

	//sensors0 cfg
	uint8_t chEnExtADC6 :1;
	uint8_t chEnexGADC7 :1;
	uint8_t chEnGsr :1;
	uint8_t chEnExg2_24Bit :1;
	uint8_t chEnExg1_24Bit :1;
	uint8_t chEnMag :1;
	uint8_t chEnGyro :1;
	uint8_t chEnlnAccel :1;

	//sensors1 cfg
	uint8_t chEnIntADC13 :1;
	uint8_t chEnIntADC12 :1;
	uint8_t chEnIntADC1 :1;
	uint8_t chEnExtADC15 :1;
	uint8_t chEnWrAccel :1;
	uint8_t chEnBattery :1;
	uint8_t unusedIdx4Bit6 :1;
	uint8_t chEnBrAmp :1;

	//sensors2 cfg
	uint8_t unusedIdx5Bit0 :1;
	uint8_t chEnMPU9150temp :1;
	uint8_t chEnPressure :1;
	uint8_t chEnExG2_16Bit :1;
	uint8_t chEnExG1_16Bit :1;
	uint8_t chEnMPU9150Mag :1;
	uint8_t chEnMPU9150Accel :1;
	uint8_t chEnIntADC14 :1;

	//Config setup Byte0
	uint8_t accelHRM :1;
	uint8_t accelLPM :1;
	uint8_t dAccelRange :2;
	uint8_t lSM303DigitalAccelRate :4;

	//Config setup Byte 1
	uint8_t mPU9150GyroRate;

	//Config setup Byte 2
	uint8_t mPU9150GyroRange :2;
	uint8_t lSM303MagRate :3;
	uint8_t lSM303MagRange :3;

	//Config setup Byte3
	uint8_t expPwr :1;
	uint8_t gsrRange :3;
	uint8_t pressurePrecision :2;
	uint8_t mPUAccelRange :2;

	uint8_t exgADS1292R_1_CONFIG1;
	uint8_t exgADS1292R_1_CONFIG2;
	uint8_t exgADS1292R_1_LOFF;
	uint8_t exgADS1292R_1_CH1SET;
	uint8_t exgADS1292R_1_CH2SET;
	uint8_t exgADS1292R_1_RLD_SENS;
	uint8_t exgADS1292R_1_LOFF_SENS;
	uint8_t exgADS1292R_1_LOFF_STAT;
	uint8_t exgADS1292R_1_RESP1;
	uint8_t exgADS1292R_1_RESP2;
	uint8_t exgADS1292R_2_CONFIG1;
	uint8_t exgADS1292R_2_CONFIG2;
	uint8_t exgADS1292R_2_LOFF;
	uint8_t exgADS1292R_2_CH1SET;
	uint8_t exgADS1292R_2_CH2SET;
	uint8_t exgADS1292R_2_RLD_SENS;
	uint8_t exgADS1292R_2_LOFF_SENS;
	uint8_t exgADS1292R_2_LOFF_STAT;
	uint8_t exgADS1292R_2_RESP1;
	uint8_t exgADS1292R_2_RESP2;
	uint8_t btCommsBaudRate;

	//nvDerivedChannels0(lsb);
	uint8_t chEnRes_amp :1;
	uint8_t chEnSkinTemp :1;
	uint8_t chEnPpg :1; //ppg_12.13
	uint8_t chEnPpg1 :1;  //ppg1
	uint8_t chEnPpg2 :1; //ppg2_1.14
	uint8_t chEnPpgToHr1 :1; //ppgtoHr_12.13
	uint8_t chEnPpgToHr2 :1; //ppgtoHr1.14

	//nvDerivedChannels1
	uint8_t chEnGsrMetricsGeneral :1;
	uint8_t chEnActivity :1;
	uint8_t chEnHrVFreq :1;
	uint8_t chEnHrVTime :1;
	uint8_t chEnEcg2HrChp2Ch2 :1;
	uint8_t chEnEcg2HrChp2Ch1 :1;
	uint8_t chEnEcg2HrChp1Ch2 :1;
	uint8_t chEnEcg2HrChp1Ch1 :1;

	//nvDerivedChannels2(msb)
	uint8_t chEnNineDofWrQuat :1;
	uint8_t chEnNineDofWrEuler :1;
	uint8_t chEnSixDofWrQuat :1;
	uint8_t chEnSixDofWrEuler :1;
	uint8_t chEnNineDofLnQuat :1;
	uint8_t chEnNineDofLnEuler :1;
	uint8_t chEnSixDofLnQuat :1;
	uint8_t chEnSixDofLnEuler :1;

	int16_t accelCalibOffset_B0;
	int16_t accelCalibOffset_B1;
	int16_t accelCalibOffset_B2;
	int16_t accelCalibSensitivity_K0;
	int16_t accelCalibSensitivity_K1;
	int16_t accelCalibSensitivity_K2;
	int8_t accelCalibAlignment_R00;
	int8_t accelCalibAlignment_R01;
	int8_t accelCalibAlignment_R02;
	int8_t accelCalibAlignment_R10;
	int8_t accelCalibAlignment_R11;
	int8_t accelCalibAlignment_R12;
	int8_t accelCalibAlignment_R20;
	int8_t accelCalibAlignment_R21;
	int8_t accelCalibAlignment_R22;

	int16_t mpu9150GyroCalibOffset_B0;
	int16_t mpu9150GyroCalibOffset_B1;
	int16_t mpu9150GyroCalibOffset_B2;
	int16_t mpu9150GyroCalibSensitivity_K0;
	int16_t mpu9150GyroCalibSensitivity_K1;
	int16_t mpu9150GyroCalibSensitivity_K2;
	int8_t mpu9150GyroCalibAlignment_R00;
	int8_t mpu9150GyroCalibAlignment_R01;
	int8_t mpu9150GyroCalibAlignment_R02;
	int8_t mpu9150GyroCalibAlignment_R10;
	int8_t mpu9150GyroCalibAlignment_R11;
	int8_t mpu9150GyroCalibAlignment_R12;
	int8_t mpu9150GyroCalibAlignment_R20;
	int8_t mpu9150GyroCalibAlignment_R21;
	int8_t mpu9150GyroCalibAlignment_R22;

	int16_t lsm303DlhcMagCalibOffset_B0;
	int16_t lsm303DlhcMagCalibOffset_B1;
	int16_t lsm303DlhcMagCalibOffset_B2;
	int16_t lsm303DlhcMagCalibSensitivtiy_K0;
	int16_t lsm303DlhcMagCalibSensitivity_K1;
	int16_t lsm303DlhcMagCalibSensitivity_K2;
	int8_t lsm303DlhcMagCalibAlignment_R00;
	int8_t lsm303DlhcMagCalibAlignment_R01;
	int8_t lsm303DlhcMagCalibAlignment_R02;
	int8_t lsm303DlhcMagCalibAlignment_R10;
	int8_t lsm303DlhcMagCalibAlignment_R11;
	int8_t lsm303DlhcMagCalibAlignment_R12;
	int8_t lsm303DlhcMagCalibAlignment_R20;
	int8_t lsm303DlhcMagCalibAlignment_R21;
	int8_t lsm303DlhcMagCalibAlignment_R22;

	int16_t lsm303DlhcAccelCalibOffset_B0;
	int16_t lsm303DlhcAccelCalibOffset_B1;
	int16_t lsm303DlhcAccelCalibOffset_B2;
	int16_t lsm303DlhcAccelCalibSensitivity_K0;
	int16_t lsm303DlhcAccelCalibSensitivity_K1;
	int16_t lsm303DlhcAccelCalibSensitivity_K2;
	int8_t lsm303DlhcAccelCalibAlignment_R00;
	int8_t lsm303DlhcAccelCalibAlignment_R01;
	int8_t lsm303DlhcAccelCalibAlignment_R02;
	int8_t lsm303DlhcAccelCalibAlignment_R10;
	int8_t lsm303DlhcAccelCalibAlignment_R11;
	int8_t lsm303DlhcAccelCalibAlignment_R12;
	int8_t lsm303DlhcAccelCalibAlignment_R20;
	int8_t lsm303DlhcAccelCalibAlignment_R21;
	int8_t lsm303DlhcAccelCalibAlignment_R22;

	//cfg in SDlog line InfoMem 118-122
	//nVDerivedChannels3
	uint8_t chEnEmgProcessingCh2 :1;
	uint8_t chEnEmgProcessingCh1 :1;
	uint8_t chEnGsrBaseline :1;
	uint8_t chEnGsrMetricsTrendPeak :1;
	uint8_t chEnGaitModule :1;
	uint8_t chEnGyroOnTheFlyCalib :1;
	uint8_t unusedByte118Bit6:1;
	uint8_t unusedByte118Bit7:1;
	uint8_t nvDerivedChannels4;
	uint8_t nvDerivedChannels5MSB;
	uint8_t nvDerivedChannels6;
	uint8_t nvDerivedChannels7;
	uint8_t unusedIdx123;
	uint8_t unusedIdx124;
	uint8_t unusedIdx125;
	uint8_t unusedIdx126;
	uint8_t unusedIdx127;

	//cfg for sd
	//nVSensors3
	uint8_t chEnMplMotionOrient :1;
	uint8_t chEnMplTap :1;
	uint8_t chEnMplPedometer :1;
	uint8_t chEnMplHeading :1;
	uint8_t chEnMplEuler_9Dof :1;
	uint8_t chEnMplEuler_6Dof :1;
	uint8_t chEnMplQuat9Dof :1;
	uint8_t chEnMplQuat6Dof :1;

	//nVSensors4
	uint8_t unusedIdx129Bit0:1;
	uint8_t unusedIdx129Bit1:1;
	uint8_t unusedIdx129Bit2:1;
	uint8_t unusedIdx129Bit3:1;
	uint8_t chEnMplQuat6DofRaw :1;
	uint8_t chEnMpu9150MagCal :1;
	uint8_t chEnMpu9150AccelCal :1;
	uint8_t chEnMpu9150GyroCal :1;

	//config setup byte4
	uint8_t chEnMpu9150MotCalCfg :3;
	uint8_t chEnMpu9150Lfp :3;
	uint8_t chEnMpu9150MplUseLsmMag :1;
	uint8_t chEnMpu9150Dmp :1;

	//config setup byte 5
	uint8_t chEnMpu9150MplMagMix :2;
	uint8_t chEnMpu9150MagSamplingRate :3;
	uint8_t chEnMpu9150MplSamplingRate :3;

	//config setup byte6
	uint8_t unusedIdx132Bit0:1;
	uint8_t unusedIdx132Bit1:1;
	uint8_t unusedIdx132Bit2:1;
	uint8_t chEnMplEnable :1;
	uint8_t chEnMplMagDistCal :1;
	uint8_t chEnMplVectCompCal :1;
	uint8_t chEnMplGyroCalTc :1;
	uint8_t chEnMplSensorFusion :1;
	uint8_t nVMPLAccelCalibration[21];
	uint8_t nVMPLMagCalibration[21];
	uint8_t nVMPLGyroCalibration[12];
	uint8_t nVSDShimmerName[12];
	uint8_t nVSdExPIDNAME[12];
	uint8_t nVSDConfigTime[4];
	uint8_t nVSDMyTrialID;
	uint8_t nVSDNShimmer;

	//nVSDTrialConfig0
	uint8_t sDErrorEnable:1;
	uint8_t master :1;
	uint8_t sync :1;
	uint8_t unusedIdx217Bit3:1;
	uint8_t rtcError :1;
	uint8_t userButton :1;
	uint8_t btPinSetup:1;
	uint8_t rtcSetByBt :1;

	//nVSDTrialConfig1
	uint8_t lowBatteryCutOut :1;
	uint8_t unusedIdx218Bit1:1;
	uint8_t unusedIdx218Bit2:1;
	uint8_t unusedIdx218Bit3:1;
	uint8_t tcxo :1;
	uint8_t unusedIdx218Bit5:1;
	uint8_t unusedIdx218Bit6:1;
	uint8_t singleTouchStart :1;

	uint8_t nVSDBTInterval;
	uint16_t nVEstExpLen;
	uint16_t nVMaxExPLen;
	uint8_t macAddr[6];

	//SDConfigDelayFlag;
	uint8_t info_sdcfg :1;
	uint8_t info_calib :1;
	uint8_t unusedIdx230Bit2:1;
	uint8_t unusedIdx230Bit3:1;
	uint8_t unusedIdx230Bit4:1;
	uint8_t unusedIdx230Bit5:1;
	uint8_t unusedIdx230Bit6:1;
	uint8_t sdCfgFlag:1;

	uint8_t nvBtSetPin;
	uint8_t unusedIdx232;
	uint8_t unusedIdx233;
	uint8_t unusedIdx234;
	uint8_t unusedIdx235;
	uint8_t unusedIdx236;
	uint8_t unusedIdx237;
	uint8_t unusedIdx238;
	uint8_t unusedIdx239;
	uint8_t unusedIdx240;
	uint8_t unusedIdx241;
	uint8_t unusedIdx242;
	uint8_t unusedIdx243;
	uint8_t unusedIdx244;
	uint8_t unusedIdx245;
	uint8_t unusedIdx246;
	uint8_t unusedIdx247;
	uint8_t unusedIdx248;
	uint8_t unusedIdx249;
	uint8_t unusedIdx250;
	uint8_t unusedIdx251;
	uint8_t unusedIdx252;
	uint8_t unusedIdx253;
	uint8_t unusedIdx254;
	uint8_t unusedIdx255;

	//cfg for sync //put it as individual bytes
	uint8_t syncNodeAddr1[6];
	uint8_t syncNodeAddr2[6];
	uint8_t syncNodeAddr3[6];
	uint8_t syncNodeAddr4[6];
	uint8_t syncNodeAddr5[6];
	uint8_t syncNodeAddr6[6];
	uint8_t syncNodeAddr7[6];
	uint8_t syncNodeAddr8[6];
	uint8_t syncNodeAddr9[6];
	uint8_t syncNodeAddr[6];
	uint8_t syncNodeAddr11[6];
	uint8_t syncNodeAddr12[6];
	uint8_t syncNodeAddr13[6];
	uint8_t syncNodeAddr14[6];
	uint8_t syncNodeAddr15[6];
	uint8_t syncNodeAddr16[6];
	uint8_t syncNodeAddr17[6];
	uint8_t syncNodeAddr18[6];
	uint8_t syncNodeAddr19[6];
	uint8_t syncNodeAddr20[6];
	uint8_t syncNodeAddr21[6];

	uint8_t unusedIdx382;
	uint8_t unusedIdx383;
} gConfigBytes;

void S4Ram_init(void);
uint8_t* S4Ram_getStoredConfig(void);
uint8_t* S4Ram_getSdHeadText(void);
void S4Ram_SetDefaultInfomem(void);

uint8_t S4Ram_sdHeadTextSet(const uint8_t* buf, uint16_t offset,uint16_t length);
uint8_t S4Ram_sdHeadTextGet(uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_sdHeadTextGetByte(uint16_t offset);
uint8_t S4Ram_sdHeadTextSetByte(uint16_t offset, uint8_t val);

uint8_t S4Ram_storedConfigSet(const uint8_t* buf, uint16_t offset,uint16_t length);
uint8_t S4Ram_storedConfigGet(uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_storedConfigGetByte(uint16_t offset);
uint8_t S4Ram_storedConfigSetByte(uint16_t offset, uint8_t val);

void S4Ram_btMacAsciiSet(const uint8_t* buf);
void S4Ram_btMacAsciiGet(uint8_t* buf);
void S4Ram_btMacHexSet(const uint8_t* buf);
void S4Ram_btMacHexGet(uint8_t* buf);

void S4Ram_config2SdHead(void);

#endif //S4Ram_H
