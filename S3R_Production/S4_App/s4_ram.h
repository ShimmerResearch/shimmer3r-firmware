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

// Infomem contents
//#define NV_NUM_CONFIG_BYTES             100
// Infomem contents
#define NV_NUM_SETTINGS_BYTES             34
#define NV_NUM_CALIBRATION_BYTES          84
#define NV_NUM_SD_BYTES                   37
#define NV_TOTAL_NUM_CONFIG_BYTES         384//NV_NUM_SETTINGS_BYTES + NV_NUM_CALIBRATION_BYTES + NV_NUM_SD_BYTES
#define NV_NUM_RWMEM_BYTES                512

#define NV_SAMPLING_RATE                  0
#define NV_BUFFER_SIZE                    2
#define NV_SENSORS0                       3
#define NV_SENSORS1                       4
#define NV_SENSORS2                       5
#define NV_CONFIG_SETUP_BYTE0             6 //sensors setting bytes
#define NV_CONFIG_SETUP_BYTE1             7
#define NV_CONFIG_SETUP_BYTE2             8
#define NV_CONFIG_SETUP_BYTE3             9
#define NV_EXG_ADS1292R_1_CONFIG1         10// exg bytes, not implemented yet
#define NV_EXG_ADS1292R_1_CONFIG2         11
#define NV_EXG_ADS1292R_1_LOFF            12
#define NV_EXG_ADS1292R_1_CH1SET          13
#define NV_EXG_ADS1292R_1_CH2SET          14
#define NV_EXG_ADS1292R_1_RLD_SENS        15
#define NV_EXG_ADS1292R_1_LOFF_SENS       16
#define NV_EXG_ADS1292R_1_LOFF_STAT       17
#define NV_EXG_ADS1292R_1_RESP1           18
#define NV_EXG_ADS1292R_1_RESP2           19
#define NV_EXG_ADS1292R_2_CONFIG1         20
#define NV_EXG_ADS1292R_2_CONFIG2         21
#define NV_EXG_ADS1292R_2_LOFF            22
#define NV_EXG_ADS1292R_2_CH1SET          23
#define NV_EXG_ADS1292R_2_CH2SET          24
#define NV_EXG_ADS1292R_2_RLD_SENS        25
#define NV_EXG_ADS1292R_2_LOFF_SENS       26
#define NV_EXG_ADS1292R_2_LOFF_STAT       27
#define NV_EXG_ADS1292R_2_RESP1           28
#define NV_EXG_ADS1292R_2_RESP2           29
#define NV_BT_COMMS_BAUD_RATE             30
#define NV_DERIVED_CHANNELS_0             31
#define NV_DERIVED_CHANNELS_1             32
#define NV_DERIVED_CHANNELS_2             33
#define NV_A_ACCEL_CALIBRATION            34
#define NV_MPU9250_GYRO_CALIBRATION       55
#define NV_LSM303DLHC_MAG_CALIBRATION     76
#define NV_LSM303DLHC_ACCEL_CALIBRATION   97       //97->117
#define NV_CALIBRATION_END                117
#define NV_DERIVED_CHANNELS_3             118
#define NV_DERIVED_CHANNELS_4             119
#define NV_DERIVED_CHANNELS_5             120
#define NV_DERIVED_CHANNELS_6             121
#define NV_DERIVED_CHANNELS_7             122

#define NV_SENSORS3                       (128+0)
#define NV_SENSORS4                       (128+1)
#define NV_CONFIG_SETUP_BYTE4             (128+2)
#define NV_CONFIG_SETUP_BYTE5             (128+3)
#define NV_CONFIG_SETUP_BYTE6             (128+4)
#define NV_MPL_ACCEL_CALIBRATION          (128+5)    //+21
#define NV_MPL_MAG_CALIBRATION            (128+26)   //+21
#define NV_MPL_GYRO_CALIBRATION           (128+47)   //+12
#define NV_SD_SHIMMER_NAME                (128+59)   // +12 bytes
#define NV_SD_EXP_ID_NAME                 (128+71)   // +12 bytes
#define NV_SD_CONFIG_TIME                 (128+83)   // +4 bytes
#define NV_SD_MYTRIAL_ID                  (128+87)   // 1 byte
#define NV_SD_NSHIMMER                    (128+88)   // 1 byte
#define NV_SD_TRIAL_CONFIG0               (128+89)
#define NV_SD_TRIAL_CONFIG1               (128+90)
#define NV_SD_BT_INTERVAL                 (128+91)
#define NV_EST_EXP_LEN_MSB                (128+92)  // 2bytes
#define NV_EST_EXP_LEN_LSB                (128+93)
#define NV_MAX_EXP_LEN_MSB                (128+94)  // 2bytes
#define NV_MAX_EXP_LEN_LSB                (128+95)
#define NV_MAC_ADDRESS                    (128+96)   // 6bytes
#define NV_SD_CONFIG_DELAY_FLAG           (128+102)
#define NV_BT_SET_PIN                     (128+103)
#define NV_TEMP_PRES_CALIBRATION          (128+104) // +22 bytes, till 128+125

#define NV_CENTER                         (128+128+0)
#define NV_NODE0                          (128+128+6)

typedef union
{
  uint8_t rawBytes[STOREDCONFIG_SIZE];
  struct __attribute__((packed))
  {
    //cfg in common
    uint16_t samplingRateTicks;
    uint8_t bufferSize;

    //sensors0 cfg
#if defined(SHIMMER3)
    uint8_t chEnExtADC6 :1;
    uint8_t chEnExtADC7 :1;
#else
    uint8_t chEnExtADC1 :1;   // S3 = ADC6, S3R = ADC11, S4_SDK = ADC8
    uint8_t chEnExtADC0 :1;   // S3 = ADC7, S3R = ADC9, S4_SDK = ADC9
#endif
    uint8_t chEnGsr :1;
    uint8_t chEnExg2_24Bit :1;
    uint8_t chEnExg1_24Bit :1;
    uint8_t chEnMag :1;     // S3/S4_SDK = LSM303DLHC/LSM303AH, S3R = LIS2MDL Mag
    uint8_t chEnGyro :1;    // S3/S4_SDK = MPU9x50/ICM20948, S3R = LSM6DSV Gyro
    uint8_t chEnLnAccel :1; // S3/S4_SDK = KXRB5-2042/KXTC9-2050, S3R = LSM6DSV Accel

    //sensors1 cfg
#if defined(SHIMMER3)
    uint8_t chEnIntADC13 :1;
    uint8_t chEnIntADC12 :1;
    uint8_t chEnIntADC1 :1;
    uint8_t chEnExtADC15 :1;
#else
    uint8_t chEnIntADC1 :1;  // S3 = ADC13, S3R = ADC15, S4_SDK = ADC11
    uint8_t chEnIntADC0 :1;  // S3 = ADC12, S3R = ADC10, S4_SDK = ADC10
    uint8_t chEnIntADC3 :1;  // S3 = ADC1, S3R = ADC17, S4_SDK = ADC2
    uint8_t chEnExtADC2 :1;  // S3 = ADC15, S3R = ADC12, S4_SDK = ADC1
#endif
    uint8_t chEnWrAccel :1; // S3/S4_SDK = LSM303DLHC/LSM303AH, S3R = LIS2DW12 Accel
    uint8_t chEnVBattery :1;
#if defined(SHIMMER4_SDK)
    uint8_t chEnPpgApp :1;
#else
    uint8_t chEnMicrophone :1;
#endif
    uint8_t chEnBridgeAmp :1;

    //sensors2 cfg
#if defined(SHIMMER3)
    uint8_t unusedIdx5Bit0 :1;
    uint8_t chEnMPU9x50temp :1; // Shimmer3 MPU9x50/ICM20948 temperature
#elif defined(SHIMMER3R)
    uint8_t altMagRange :2; // S3R = LIS3MDL Mag
#elif defined(SHIMMER4_SDK)
    uint8_t chEnStc3100 :1;
    uint8_t chEnIntADC4 :1;  // S4_SDK = ADC12
#endif
    uint8_t chEnPressureAndTemperature :1;
    uint8_t chEnExg2_16Bit :1;
    uint8_t chEnExg1_16Bit :1;
    uint8_t chEnAltMag :1; // S3/S4_SDK MPU9x50/ICM20948 Mag, S3R = LIS3MDL
    uint8_t chEnAltAccel :1; // S3/S4_SDK MPU9x50/ICM20948 Accel, S3R = ADXL371
#if defined(SHIMMER3)
    uint8_t chEnIntADC14 :1;
#else
    uint8_t chEnIntADC2 :1;  // S3R = ADC16, S4_SDK = ADC0
#endif

    //Config setup Byte0
    uint8_t wrAccelHRM :1;
    uint8_t wrAccelLPM :1;
    uint8_t wrAccelRange :2;
    uint8_t wrAccelRate :4;

    //Config setup Byte 1
    uint8_t gyroRate;

    //Config setup Byte 2
    uint8_t gyroRange :2;
    uint8_t magRate :3;
    uint8_t magRange :3;

    //Config setup Byte3
    uint8_t expPwr :1;
    uint8_t gsrRange :3;
    uint8_t pressurePrecision :2;
    uint8_t altAccelRange :2; // S3/S4_SDK MPU9x50/ICM20948 Accel, S3R = LSM6DSV Accel

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
    uint8_t chEnResAmp :1;
    uint8_t chEnSkinTemp :1;
    uint8_t chEnPpg :1; //ppg_12.13
    uint8_t chEnPpg1 :1; //ppg1_12.13
    uint8_t chEnPpg2 :1;  //ppg2_1.14
    uint8_t chEnPpgtoHr :1; //ppgtoHr_12.13
    uint8_t chEnPpgToHr1 :1; //ppgtoHr1_12.13
    uint8_t chEnPpgToHr2 :1; //ppgtoHr2_1.14

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

    int16_t gyroCalibOffset_B0;
    int16_t gyroCalibOffset_B1;
    int16_t gyroCalibOffset_B2;
    int16_t gyroCalibSensitivity_K0;
    int16_t gyroCalibSensitivity_K1;
    int16_t gyroCalibSensitivity_K2;
    int8_t gyroCalibAlignment_R00;
    int8_t gyroCalibAlignment_R01;
    int8_t gyroCalibAlignment_R02;
    int8_t gyroCalibAlignment_R10;
    int8_t gyroCalibAlignment_R11;
    int8_t gyroCalibAlignment_R12;
    int8_t gyroCalibAlignment_R20;
    int8_t gyroCalibAlignment_R21;
    int8_t gyroCalibAlignment_R22;

    int16_t magCalibOffset_B0;
    int16_t magCalibOffset_B1;
    int16_t magCalibOffset_B2;
    int16_t magCalibSensitivtiy_K0;
    int16_t magCalibSensitivity_K1;
    int16_t magCalibSensitivity_K2;
    int8_t magCalibAlignment_R00;
    int8_t magCalibAlignment_R01;
    int8_t magCalibAlignment_R02;
    int8_t magCalibAlignment_R10;
    int8_t magCalibAlignment_R11;
    int8_t magCalibAlignment_R12;
    int8_t magCalibAlignment_R20;
    int8_t magCalibAlignment_R21;
    int8_t magCalibAlignment_R22;

    int16_t wrAccelCalibOffset_B0;
    int16_t wrAccelCalibOffset_B1;
    int16_t wrAccelCalibOffset_B2;
    int16_t wrAccelCalibSensitivity_K0;
    int16_t wrAccelCalibSensitivity_K1;
    int16_t wrAccelCalibSensitivity_K2;
    int8_t wrAccelCalibAlignment_R00;
    int8_t wrAccelCalibAlignment_R01;
    int8_t wrAccelCalibAlignment_R02;
    int8_t wrAccelCalibAlignment_R10;
    int8_t wrAccelCalibAlignment_R11;
    int8_t wrAccelCalibAlignment_R12;
    int8_t wrAccelCalibAlignment_R20;
    int8_t wrAccelCalibAlignment_R21;
    int8_t wrAccelCalibAlignment_R22;

    //cfg in SDlog line InfoMem 118-122
    //nVDerivedChannels3
    uint8_t chEnEmgProcessingCh2 :1;
    uint8_t chEnEmgProcessingCh1 :1;
    uint8_t chEnGsrBaseline :1;
    uint8_t chEnGsrMetricsTrendPeak :1;
    uint8_t chEnGaitModule :1;
    uint8_t chEnGyroOnTheFlyCalib :1;
    uint8_t unusedByte118Bit6 :1;
    uint8_t unusedByte118Bit7 :1;
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
    uint8_t unusedIdx129Bit0 :1;
    uint8_t unusedIdx129Bit1 :1;
    uint8_t unusedIdx129Bit2 :1;
    uint8_t unusedIdx129Bit3 :1;
    uint8_t chEnMplQuat6DofRaw :1;
    uint8_t chEnMpu9x50MagCal :1;
    uint8_t chEnMpu9x50AccelCal :1;
    uint8_t chEnMpu9x50GyroCal :1;

    //config setup byte4
    uint8_t chEnMpu9x50MotCalCfg :3;
    uint8_t chEnMpu9x50Lfp :3;
    uint8_t chEnMpu9x50MplUseLsmMag :1;
    uint8_t chEnMpu9x50Dmp :1;

    //config setup byte 5
    uint8_t chEnMpu9x50MplMagMix :2;
    uint8_t chEnMpu9x50MagSamplingRate :3;
    uint8_t chEnMpu9x50MplSamplingRate :3;

    //config setup byte6
    uint8_t unusedIdx132Bit0 :1;
    uint8_t unusedIdx132Bit1 :1;
    uint8_t unusedIdx132Bit2 :1;
    uint8_t chEnMplEnable :1;
    uint8_t chEnMplMagDistCal :1;
    uint8_t chEnMplVectCompCal :1;
    uint8_t chEnMplGyroCalTc :1;
    uint8_t chEnMplSensorFusion :1;
    uint8_t mplAccelCalibration[21];
    uint8_t mplMagCalibration[21];
    uint8_t mplGyroCalibration[12];
    char shimmerName[12];
    char expIdName[12];
    uint32_t configTime;
    uint8_t myTrialID;
    uint8_t numberOfShimmers;

    //SDTrialConfig0
    uint8_t sdErrorEnable :1;
    uint8_t master :1;
    uint8_t sync :1;
    uint8_t unusedIdx217Bit3 :1;
    uint8_t rtcErrorEnable :1;
    uint8_t userButtonEnable :1;
    uint8_t btPinSetup :1;
    uint8_t rtcSetByBt :1;

    //SDTrialConfig1
    uint8_t lowBatteryCutOut :1;
    uint8_t unusedIdx218Bit1 :1;
    uint8_t unusedIdx218Bit2 :1;
    uint8_t unusedIdx218Bit3 :1;
    uint8_t tcxo :1;
    uint8_t unusedIdx218Bit5 :1;
    uint8_t unusedIdx218Bit6 :1;
    uint8_t singleTouchStart :1;

    uint8_t btInterval;
    uint16_t estExpLen;
    uint16_t maxExPLen;
    uint8_t macAddr[6];

    //SDConfigDelayFlag;
    uint8_t infoSdcfg :1;
    uint8_t infoCalib :1;
    uint8_t unusedIdx230Bit2 :1;
    uint8_t unusedIdx230Bit3 :1;
    uint8_t unusedIdx230Bit4 :1;
    uint8_t unusedIdx230Bit5 :1;
    uint8_t unusedIdx230Bit6 :1;
    uint8_t sdCfgFlag :1;

    uint8_t btSetPin;
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

    uint8_t padding[128];
  };
} gConfigBytes;

void S4Ram_init(void);
gConfigBytes* S4Ram_getStoredConfig(void);
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

void S4Ram_btMacAsciiSet(char *buf);
void S4Ram_btMacAsciiGet(uint8_t* buf);
void S4Ram_btMacHexSet(uint8_t* buf);
void S4Ram_btMacHexGet(uint8_t* buf);

void S4Ram_config2SdHead(void);

void setDefaultShimmerName(void);
void setDefaultTrialId(void);

#endif //S4Ram_H
