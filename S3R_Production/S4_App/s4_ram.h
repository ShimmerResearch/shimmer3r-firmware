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

//#pragma pack(2)

typedef __packed struct
		{

	      //cfg in common

		  uint8_t samplingRateLSB;
		  uint8_t samplingRateMSB;
		  uint8_t bufferSizeLSB;
		  uint8_t bufferSizeMSB;

		  //sensors0 cfg
		  uint8_t lNAccelCfg:1;
		  uint8_t gyroCfg:1;
		  uint8_t magCfg:1;
		  uint8_t eXG1_24Bit:1;
		  uint8_t eXG2_24Bit:1;
		  uint8_t gsrCfg:1;
		  uint8_t eXGCh7:1;
		  uint8_t eXGCh6:1;
		  //sensors1 cfg
		  uint8_t brAmp:1; //confirm
		  uint8_t unusedbit:1;
	      uint8_t batteryCfg:1;
	      uint8_t wRAccelCfg:1;
		  uint8_t eXtCh15:1;
		  uint8_t intCh1:1;
		  uint8_t intCh12:1;
		  uint8_t intCh13:1;

		  //sensors2 cfg
		  uint8_t intCh14:1;
		  uint8_t mPU9150Accel:1;
		  uint8_t mPU9150Mag:1;
		  uint8_t exG1_16BitCfg:1;
		  uint8_t exG2_16BitCfg:2;
		  uint8_t pres:1;
		  uint8_t mPU9150temp:2;

		  //Config setup Byte0

		  uint8_t lSM303DigitalAccelRate:4;
		  uint8_t dAccelRange:2;
		  uint8_t accelLPM:1;
		  uint8_t accelHRM:1;
		  //Config setup Byte 1

		  uint8_t MPU9150GyroRate:8;

		  //Config setup Byte 2

		  uint8_t LSM303MagRange:3;
		  uint8_t LSM303MagRate:3;
		  uint8_t MPU9150GyroRange:2;

		  //Config setup Byte3

		  uint8_t MPUAccelRange:2;
		  uint8_t pressurePrecision:2;
		  uint8_t gsrRange:3;
		  uint8_t expPwr:1;


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
		  uint8_t ppgToHr2:1; //ppgtoHr1.14
		  uint8_t ppgToHr1:1; //ppgtoHr_12.13
		  uint8_t ppg2:1; //ppg2_1.14
		  uint8_t ppg1:1;  //ppg1
		  uint8_t ppg:1; //ppg_12.13
		  uint8_t skinTemp:1;
		  uint8_t res_amp:1;

		  //nvDerivedChannels1
		  uint8_t ecg2hrChp1Ch1:1;
		  uint8_t ecg2hrChp1Ch2:1;
		  uint8_t ecg2hrChp2Ch1:1;
		  uint8_t ecg2hrChp2Ch2:1;
		  uint8_t hrVTime:1;
		  uint8_t hrVFreq:1;
		  uint8_t activity:1;
		  uint8_t gsrMetricsGeneral:1;

		  //nvDerivedChannels2(msb)
		  uint8_t sixDofLnEuler:1;
		  uint8_t sixDofLnQuat:1;
		  uint8_t nineDofLnEuler:1;
		  uint8_t nineDofLnQuat:1;
		  uint8_t sixDofWrEuler:1;
		  uint8_t sixDofWrQuat:1;
		  uint8_t nineDofWrEuler:1;
		  uint8_t nineDofWrQuat:1;



		  uint8_t A_accelCalibration0Byte0; //size 21
		  uint8_t A_accelCalibration0Byte1;
		  uint8_t A_accelCalibration0Byte2;

		  uint8_t MPU9150GyroCalibrationByte0; //size 21
		  uint8_t MPU9150GyroCalibrationByte1;
		  uint8_t MPU9150GyroCalibrationByte2;

		  uint8_t LSM303DLHCMagCalibrationByte0;
		  uint8_t LSM303DLHCMagCalibrationByte1;
		  uint8_t LSM303DLHCMagCalibrationByte2;

		  uint8_t LSM303DLHCAccelCalibrationByte0;
		  uint8_t LSM303DLHCAccelCalibrationByte1;
		  uint8_t LSM303DLHCAccelCalibrationByte2;


		  //cfg in SDlog line InfoMem 118-122

		  //nVDerivedChannels3

		  uint8_t unusedBit:2;
		  uint8_t gyroOnTheFlyCalib:1;
		  uint8_t gaitModule:1;
		  uint8_t gsrMetricsTrendPeak:1;
		  uint8_t gsrBaseline:1;
		  uint8_t emgProcessingCh1:1;
		  uint8_t emgProcessingCh2:1;


		  uint8_t nvDerivedChannels4;
		  uint8_t nvDerivedChannels5MSB;
		  uint8_t nvDerivedChannels6;
		  uint8_t nvDerivedChannels7;

		  uint8_t unusedBytes0;
		  uint8_t unusedBytes1;
		  uint8_t unusedBytes2;
		  uint8_t unusedBytes3;
		  uint8_t unusedBytes4;

		  //cfg for sd

		  //nVSensors3
		  uint8_t mplQuat6Dof:1;
		  uint8_t mplQuat9Dof:1;
		  uint8_t mplEuler_6Dof:1;
		  uint8_t mplEuler_9Dof:1;
		  uint8_t mplHeading:1;
		  uint8_t mplPedometer:1;
		  uint8_t mplTap:1;
		  uint8_t mplMotionOrient:1;
		  //nVSensors4
		  uint8_t mpu9150GyroCal:1;
		  uint8_t mpu9150AccelCal:1;
		  uint8_t mpu9150MagCal:1;
		  uint8_t mplQuat6DofRaw:1;
		  uint8_t unusedbits:4;
		  //config setup byte4
		  uint8_t mpu9150Dmp:1;
		  uint8_t mpu9150MplUseLsmMag:1;
		  uint8_t mpu9150Lfp:3;
		  uint8_t mpu9150MotCalCfg:3;
		  //config setup byte 5
		  uint8_t mpu9150MplSamplingRate:3;
		  uint8_t mpu9150MagSamplingRate:3;
		  uint8_t mpu9150MplMagMix:2;
		  //config setup byte6
		  uint8_t mplSensorFusion:1;
		  uint8_t mplGyroCalTc:1;
		  uint8_t mplVectCompCal:1;
		  uint8_t mplMagDistCal:1;
		  uint8_t mplEnable:1;
		  uint8_t unusedbitS:3;


		  uint8_t nVMPLAccelCalibration0;
		  uint8_t nVMPLAccelCalibration1;
		  uint8_t nVMPLAccelCalibration2;
		  uint8_t nVMPLMagCalibration0;
		  uint8_t nVMPLMagCalibration1;
		  uint8_t nVMPLMagCalibration2;
		  uint8_t nVMPLGyroCalibration0;
		  uint8_t nVMPLGyroCalibration1;//should this be changed to just one byte u16 (size 12)
		  uint8_t nVSDShimmerName0;
		  uint8_t nVSDShimmerName1;//should this be changed to just one byte u16 (size 12)
		  uint16_t nVSDExPIDNAME:12;
		  uint8_t nVSDConfigTimeMSB:4;
		  uint8_t nVSDMyTrialID;
		  uint8_t nVSDNShimmer;
		  //nVSDTrialConfig0
		  uint8_t rtcSetByBt:1;
		  uint8_t unUsedbit:1; //cross check
		  uint8_t userButton:1;
		  uint8_t rtcError:1;
		  uint8_t un_Used_Bit:1; //cross check
		  uint8_t sync:1;
		  uint8_t master:1;
		  //nVSDTrialConfig1
		  uint8_t singleTouchStart:1; //is it 1 bit or 3bit ? line 92 in excel sheet
		  uint8_t unused_Bit:2; //cross check
		  uint8_t tcxo:1;
		  uint8_t unused_bit:3; //cross check
		  uint8_t lowBatteryCutOut:1;
		  uint8_t nVSDBTInterval;
		  uint8_t nVEstExpLenLSB;
		  uint8_t nVEstExpLenMSB;
		  uint8_t nVMaxExPLenlSB;
		  uint8_t nVMaxExPLenMSB;
		  uint8_t macAddr:6;

		  //SDConfigDelayFlag;
		  uint8_t unUsedBits:6;
		  uint8_t info_calib:1;
		  uint8_t info_sdcfg:1;
		  uint16_t unusedbytes0;
		  uint8_t  unusedbytes1;

		  //cfg for sync //put it as individual bytes

		  uint64_t syncNodeAddr1Byte0;
		  uint64_t syncNodeAddr1Byte1;
		  uint64_t syncNodeAddr1Byte2;
		  uint64_t syncNodeAddr1Byte3;
		  uint64_t syncNodeAddr1Byte4;
		  uint64_t syncNodeAddr1Byte5;


		  uint64_t syncNodeAddr2Byte0;
		  uint64_t syncNodeAddr2Byte1;
		  uint64_t syncNodeAddr2Byte2;
		  uint64_t syncNodeAddr2Byte3;
		  uint64_t syncNodeAddr2Byte4;
		  uint64_t syncNodeAddr2Byte5;


		  uint64_t syncNodeAddr3Byte0;
		  uint64_t syncNodeAddr3Byte1;
		  uint64_t syncNodeAddr3Byte2;
		  uint64_t syncNodeAddr3Byte3;
		  uint64_t syncNodeAddr3Byte4;
		  uint64_t syncNodeAddr3Byte5;

		  uint64_t syncNodeAddr4Byte0;
		  uint64_t syncNodeAddr4Byte1;
		  uint64_t syncNodeAddr4Byte2;
		  uint64_t syncNodeAddr4Byte3;
		  uint64_t syncNodeAddr4Byte4;
		  uint64_t syncNodeAddr4Byte5;

		  uint64_t syncNodeAddr5Byte0;
		  uint64_t syncNodeAddr5Byte1;
		  uint64_t syncNodeAddr5Byte2;
		  uint64_t syncNodeAddr5Byte3;
		  uint64_t syncNodeAddr5Byte4;
		  uint64_t syncNodeAddr5Byte5;

		  uint64_t syncNodeAddr6Byte0;
		  uint64_t syncNodeAddr6Byte1;
		  uint64_t syncNodeAddr6Byte2;
		  uint64_t syncNodeAddr6Byte3;
		  uint64_t syncNodeAddr6Byte4;
		  uint64_t syncNodeAddr6Byte5;

		  uint64_t syncNodeAddr7Byte0;
		  uint64_t syncNodeAddr7Byte1;
		  uint64_t syncNodeAddr7Byte2;
		  uint64_t syncNodeAddr7Byte3;
		  uint64_t syncNodeAddr7Byte4;
		  uint64_t syncNodeAddr7Byte5;
		  uint64_t syncNodeAddr7Byte6;

		  uint64_t syncNodeAddr8Byte0;
		  uint64_t syncNodeAddr8Byte1;
		  uint64_t syncNodeAddr8Byte2;
		  uint64_t syncNodeAddr8Byte3;
		  uint64_t syncNodeAddr8Byte4;
		  uint64_t syncNodeAddr8Byte5;

		  uint64_t syncNodeAddr9Byte1;
		  uint64_t syncNodeAddr9Byte2;
		  uint64_t syncNodeAddr9Byte3;
		  uint64_t syncNodeAddr9Byte4;
		  uint64_t syncNodeAddr9Byte5;

		  uint64_t syncNodeAddr10Byte0;
		  uint64_t syncNodeAddr10Byte1;
		  uint64_t syncNodeAddr10Byte2;
		  uint64_t syncNodeAddr10Byte3;
		  uint64_t syncNodeAddr10Byte4;
		  uint64_t syncNodeAddr10Byte5;

		  uint64_t syncNodeAddr11Byte0;
		  uint64_t syncNodeAddr11Byte1;
		  uint64_t syncNodeAddr11Byte2;
		  uint64_t syncNodeAddr11Byte3;
		  uint64_t syncNodeAddr11Byte4;
		  uint64_t syncNodeAddr11Byte5;

		  uint64_t syncNodeAddr12Byte0;
		  uint64_t syncNodeAddr12Byte1;
		  uint64_t syncNodeAddr12Byte2;
		  uint64_t syncNodeAddr12Byte3;
		  uint64_t syncNodeAddr12Byte4;
		  uint64_t syncNodeAddr12Byte5;


		  uint64_t syncNodeAddr13Byte0;
		  uint64_t syncNodeAddr13Byte1;
		  uint64_t syncNodeAddr13Byte2;
		  uint64_t syncNodeAddr13Byte3;
		  uint64_t syncNodeAddr13Byte4;
		  uint64_t syncNodeAddr13Byte5;

		  uint64_t syncNodeAddr14Byte0;
		  uint64_t syncNodeAddr14Byte1;
		  uint64_t syncNodeAddr14Byte2;
		  uint64_t syncNodeAddr14Byte3;
		  uint64_t syncNodeAddr14Byte4;
		  uint64_t syncNodeAddr14Byte5;

		  uint64_t syncNodeAddr15Byte0;
		  uint64_t syncNodeAddr15Byte1;
		  uint64_t syncNodeAddr15Byte2;
		  uint64_t syncNodeAddr15Byte3;
		  uint64_t syncNodeAddr15Byte4;
		  uint64_t syncNodeAddr15Byte5;


		  uint64_t syncNodeAddr16Byte0;
		  uint64_t syncNodeAddr16Byte1;
		  uint64_t syncNodeAddr16Byte2;
		  uint64_t syncNodeAddr16Byte3;
		  uint64_t syncNodeAddr16Byte4;
		  uint64_t syncNodeAddr16Byte5;

		  uint64_t syncNodeAddr17Byte0;
		  uint64_t syncNodeAddr17Byte1;
		  uint64_t syncNodeAddr17Byte2;
		  uint64_t syncNodeAddr17Byte3;
		  uint64_t syncNodeAddr17Byte4;
		  uint64_t syncNodeAddr17Byte5;

		  uint64_t syncNodeAddr18Byte0;
		  uint64_t syncNodeAddr18Byte1;
		  uint64_t syncNodeAddr18Byte2;
		  uint64_t syncNodeAddr18Byte3;
		  uint64_t syncNodeAddr18Byte4;
		  uint64_t syncNodeAddr18Byte5;

		  uint64_t syncNodeAddr19Byte0;
		  uint64_t syncNodeAddr19Byte1;
		  uint64_t syncNodeAddr19Byte2;
		  uint64_t syncNodeAddr19Byte3;
		  uint64_t syncNodeAddr19Byte4;
		  uint64_t syncNodeAddr19Byte5;

		  uint64_t syncNodeAddr20Byte0;
		  uint64_t syncNodeAddr20Byte1;
		  uint64_t syncNodeAddr20Byte2;
		  uint64_t syncNodeAddr20Byte3;
		  uint64_t syncNodeAddr20Byte4;
		  uint64_t syncNodeAddr20Byte5;

		  uint64_t syncNodeAddr21Byte0;
		  uint64_t syncNodeAddr21Byte1;
		  uint64_t syncNodeAddr21Byte2;
		  uint64_t syncNodeAddr21Byte3;
		  uint64_t syncNodeAddr21Byte4;
		  uint64_t syncNodeAddr21Byte5;

		};

void S4Ram_init(void);   
uint8_t* S4Ram_getStoredConfig(void);
uint8_t* S4Ram_getSdHeadText(void);
void S4Ram_SetDefaultInfomem(void);

uint8_t S4Ram_sdHeadTextSet(const uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_sdHeadTextGet(uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_sdHeadTextGetByte(uint16_t offset);
uint8_t S4Ram_sdHeadTextSetByte(uint16_t offset, uint8_t val);

uint8_t S4Ram_storedConfigSet(const uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_storedConfigGet(uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_storedConfigGetByte(uint16_t offset);
uint8_t S4Ram_storedConfigSetByte(uint16_t offset, uint8_t val);

void S4Ram_btMacAsciiSet(const uint8_t* buf);
void S4Ram_btMacAsciiGet(uint8_t* buf);
void S4Ram_btMacHexSet(const uint8_t* buf);
void S4Ram_btMacHexGet(uint8_t* buf);

void S4Ram_config2SdHead(void);

#endif //S4Ram_H
