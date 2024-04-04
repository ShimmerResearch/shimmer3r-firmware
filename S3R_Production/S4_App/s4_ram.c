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
 
#include "s4_ram.h"


uint8_t storedConfig[STOREDCONFIG_SIZE], sdHeadText[SD_HEAD_SIZE], btMacAscii[14], btMacHex[6];


void S4Ram_init(void){     
   // init RAM (storedConfig) 
#if USE_DEFAULT_SENSOR
   S4Ram_SetDefaultInfomem();
#else
   uint8_t temp_storedConfig[STOREDCONFIG_SIZE];
   InfoMem_readRam(temp_storedConfig, 0, STOREDCONFIG_SIZE);
   if ((temp_storedConfig[NV_SENSORS1] == 0xFF)|| (*(uint16_t*)temp_storedConfig == 0)){
      //if config was never written to Infomem, write default
      //assuming some other app didn't make use of InfoMem, or else InfoMem was erased
      S4Ram_SetDefaultInfomem();  
   }
   else{
      //memcpy(temp_storedConfig+NV_MAC_ADDRESS, btMacHex, 6);
      S4Ram_storedConfigSet(temp_storedConfig, 0, STOREDCONFIG_SIZE);
   }
#endif //USE_DEFAULT_SENSOR
   S4Ram_storedConfigSet(btMacHex, NV_MAC_ADDRESS, 6);     
}
uint8_t* S4Ram_getStoredConfig(void){
   return storedConfig;
}
uint8_t* S4Ram_getSdHeadText(void){
   return sdHeadText;
}


/*
 * sdHeadText: Set(), Get() and GetByte(), S4Ram_sdHeadTextSetByte()
 */
uint8_t S4Ram_sdHeadTextSet(const uint8_t* buf, uint16_t offset, uint16_t length){
   if((offset>SD_HEAD_SIZE-1) || (offset+length>SD_HEAD_SIZE) || (length==0)){
      return 1;
   }
   memcpy(sdHeadText+offset, buf, length);
   return 0;
}

uint8_t S4Ram_sdHeadTextGet(uint8_t* buf, uint16_t offset, uint16_t length){
   if((offset>SD_HEAD_SIZE-1) || (offset+length>SD_HEAD_SIZE) || (length==0)){
      return 1;
   }
   memcpy(buf, sdHeadText+offset, length);
   return 0;
}

uint8_t S4Ram_sdHeadTextGetByte(uint16_t offset){
   if(offset>SD_HEAD_SIZE-1){
      return 0;
   }   
   return sdHeadText[offset];
}

uint8_t S4Ram_sdHeadTextSetByte(uint16_t offset, uint8_t val){
   if(offset>SD_HEAD_SIZE-1){
      return 1;
   }   
   sdHeadText[offset] = val;
   return 0;
}


/*
 * storedConfig: Set(), Get() and GetByte(), S4Ram_sdHeadTextSetByte()
 */

uint8_t S4Ram_storedConfigSet(const uint8_t* buf, uint16_t offset, uint16_t length){
   if((offset>STOREDCONFIG_SIZE-1) || (offset+length>STOREDCONFIG_SIZE) || (length==0)){
      return 1;
   }
   memcpy(storedConfig+offset, buf, length);
   return 0;
}

uint8_t S4Ram_storedConfigGet(uint8_t* buf, uint16_t offset, uint16_t length){
   if((offset>STOREDCONFIG_SIZE-1) || (offset+length>STOREDCONFIG_SIZE) || (length==0)){
      return 1;
   }
   memcpy(buf, storedConfig+offset, length);
   return 0;
}

uint8_t S4Ram_storedConfigGetByte(uint16_t offset){
   if(offset>STOREDCONFIG_SIZE-1){
      return 0;// or not?
   }   
   return storedConfig[offset];
}

uint8_t S4Ram_storedConfigSetByte(uint16_t offset, uint8_t val){
   if(offset>STOREDCONFIG_SIZE-1){
      return 1;
   }   
   storedConfig[offset] = val;
   return 0;
}

/*
 * btMacAscii: Set(), Get()
 */

void S4Ram_btMacAsciiSet(char *buf)
{
   memcpy(btMacAscii, buf, 12);
   btMacAscii[12] = 0;
}

void S4Ram_btMacAsciiGet(uint8_t *buf)
{
  memcpy(buf, btMacAscii, 12);
}


/*
 * btMacHex: Set(), Get()
 */

void S4Ram_btMacHexSet(uint8_t *buf)
{
  memcpy(btMacHex, buf, 6);
}

void S4Ram_btMacHexGet(uint8_t *buf)
{
  memcpy(buf, btMacHex, 6);
}

void S4Ram_SetDefaultInfomem(void) {
   //uint8_t temp_btMacAscii[14];
   
   //51.2Hz
   *(uint16_t *)(storedConfig + NV_SAMPLING_RATE) = 0x40;
   storedConfig[NV_BUFFER_SIZE] = 1;
   //core sensors enabled
   storedConfig[NV_SENSORS0] = SENSOR_A_ACCEL + SENSOR_EXG1_24BIT + SENSOR_EXG2_24BIT + SENSOR_MPU9250_GYRO;//SENSOR_LSM303DLHC_MAG + 
   storedConfig[NV_SENSORS1] = SENSOR_VBATT;//SENSOR_STRAIN + SENSOR_INT_ADC_10 + SENSOR_LSM303DLHC_ACCEL
   storedConfig[NV_SENSORS2] = 0;//SENSOR_INT_ADC_12 + SENSOR_BMP180_PRESSURE
   //storedConfig[NV_SENSORS5] = SENSOR_APP_PPG;
   
   //LSM303DLHC Accel 100Hz, +/-2G, Low Power and High Resolution modes off
   storedConfig[NV_CONFIG_SETUP_BYTE0] = (LSM303DLHC_ACCEL_1_344kHz << 4) + (ACCEL_2G << 2);
   //MPU9150 sampling rate of 8kHz/(155+1), i.e. 51.282Hz
   storedConfig[NV_CONFIG_SETUP_BYTE1] = 0x9B;
   //LSM303DLHC Mag 75Hz, +/-1.3 Gauss, MPU9150 Gyro +/-500 degrees per second
   storedConfig[NV_CONFIG_SETUP_BYTE2] = (LSM303DLHC_MAG_1_3G << 5) + (LSM303DLHC_MAG_75HZ << 2) + MPU9150_GYRO_500DPS;
   //MPU9150 Accel +/-2G, BMP pressure oversampling ratio 1, GSR auto range, EXP_RESET_N pin set low
   // todo: *** *** *** warning! *** *** ***  btStream for this here is not correct, this is mpu9150_accrange, not lsm303 acc range
   storedConfig[NV_CONFIG_SETUP_BYTE3] = (ACCEL_2G << 6) + (BMP180_OSS_8 << 4) + (GSR_AUTORANGE << 1); //HW_RES_40K
   //set all ExG registers to their reset values
   //square wave test
   storedConfig[NV_EXG_ADS1292R_1_CONFIG1] = 0x04;
   storedConfig[NV_EXG_ADS1292R_1_CONFIG2] = 0xab;//was 0xa3 for rev1
   storedConfig[NV_EXG_ADS1292R_1_LOFF] = 0x10;
   storedConfig[NV_EXG_ADS1292R_1_CH1SET] = 0x05;
   storedConfig[NV_EXG_ADS1292R_1_CH2SET] = 0x05;
   storedConfig[NV_EXG_ADS1292R_1_RLD_SENS] = 0x00;
   storedConfig[NV_EXG_ADS1292R_1_LOFF_SENS] = 0x00;
   storedConfig[NV_EXG_ADS1292R_1_LOFF_STAT] = 0x00;
   storedConfig[NV_EXG_ADS1292R_1_RESP1] = 0x02;
   storedConfig[NV_EXG_ADS1292R_1_RESP2] = 0x01;
   storedConfig[NV_EXG_ADS1292R_2_CONFIG1] = 0x04;
   storedConfig[NV_EXG_ADS1292R_2_CONFIG2] = 0xa3;
   storedConfig[NV_EXG_ADS1292R_2_LOFF] = 0x10;
   storedConfig[NV_EXG_ADS1292R_2_CH1SET] = 0x05;
   storedConfig[NV_EXG_ADS1292R_2_CH2SET] = 0x05;
   storedConfig[NV_EXG_ADS1292R_2_RLD_SENS] = 0x00;
   storedConfig[NV_EXG_ADS1292R_2_LOFF_SENS] = 0x00;
   storedConfig[NV_EXG_ADS1292R_2_LOFF_STAT] = 0x00;
   storedConfig[NV_EXG_ADS1292R_2_RESP1] = 0x02;
   storedConfig[NV_EXG_ADS1292R_2_RESP2] = 0x01;
   //ecg
//   storedConfig[NV_EXG_ADS1292R_1_CONFIG1] = 0x02;
//   storedConfig[NV_EXG_ADS1292R_1_CONFIG2] = 0x80;
//   storedConfig[NV_EXG_ADS1292R_1_LOFF] = 0x10;
//   storedConfig[NV_EXG_ADS1292R_1_CH1SET] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_1_CH2SET] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_1_RLD_SENS] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_1_LOFF_SENS] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_1_LOFF_STAT] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_1_RESP1] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_1_RESP2] = 0x02;
//   storedConfig[NV_EXG_ADS1292R_2_CONFIG1] = 0x02;
//   storedConfig[NV_EXG_ADS1292R_2_CONFIG2] = 0x80;
//   storedConfig[NV_EXG_ADS1292R_2_LOFF] = 0x10;
//   storedConfig[NV_EXG_ADS1292R_2_CH1SET] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_2_CH2SET] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_2_RLD_SENS] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_2_LOFF_SENS] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_2_LOFF_STAT] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_2_RESP1] = 0x00;
//   storedConfig[NV_EXG_ADS1292R_2_RESP2] = 0x02;

//   if(storedConfig[NV_BT_COMMS_BAUD_RATE] == 0xFF)
//      storedConfig[NV_BT_COMMS_BAUD_RATE] = 9;

   // sd config
   //shimmername
   strcpy((char*)(storedConfig+NV_SD_SHIMMER_NAME), "idXXXX");
   memcpy((storedConfig+NV_SD_SHIMMER_NAME)+2, btMacAscii+8, 4);
   //exp_id
   memcpy((char*)(storedConfig+NV_SD_EXP_ID_NAME), "DefaultTrial", 12);
   memset(&storedConfig[NV_SD_CONFIG_TIME], 0x00, 4);   
   
   storedConfig[NV_SD_MYTRIAL_ID] = 0x00;
   storedConfig[NV_SD_NSHIMMER] = 0x00;
   storedConfig[NV_SD_TRIAL_CONFIG0] = SDH_USER_BUTTON_ENABLE + SDH_RWCERROR_EN;
   storedConfig[NV_SD_TRIAL_CONFIG1] = 0;
   storedConfig[NV_SD_BT_INTERVAL] = 54;

   InfoMem_update();
}

void S4Ram_config2SdHead(void) {   
   memset(sdHeadText, 0xff, SDHEAD_LEN);

   sdHeadText[SDH_SAMPLE_RATE_0] = storedConfig[NV_SAMPLING_RATE];
   sdHeadText[SDH_SAMPLE_RATE_1] = storedConfig[NV_SAMPLING_RATE + 1];
   sdHeadText[SDH_BUFFER_SIZE] = storedConfig[NV_BUFFER_SIZE];
   sdHeadText[SDH_SENSORS0] = storedConfig[NV_SENSORS0];
   sdHeadText[SDH_SENSORS1] = storedConfig[NV_SENSORS1];
   sdHeadText[SDH_SENSORS2] = storedConfig[NV_SENSORS2];
   sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
   sdHeadText[SDH_CONFIG_SETUP_BYTE1] = storedConfig[NV_CONFIG_SETUP_BYTE1];
   sdHeadText[SDH_CONFIG_SETUP_BYTE2] = storedConfig[NV_CONFIG_SETUP_BYTE2];
   sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
   // DMP related - start
   sdHeadText[SDH_SENSORS3] = storedConfig[NV_SENSORS3];
   sdHeadText[SDH_SENSORS4] = storedConfig[NV_SENSORS4];
   sdHeadText[SDH_CONFIG_SETUP_BYTE4] = storedConfig[NV_CONFIG_SETUP_BYTE4];
   sdHeadText[SDH_CONFIG_SETUP_BYTE5] = storedConfig[NV_CONFIG_SETUP_BYTE5];
   sdHeadText[SDH_CONFIG_SETUP_BYTE6] = storedConfig[NV_CONFIG_SETUP_BYTE6];
   // DMP related - end
   // little endian in fw, but they want big endian in sw
   // trivial
   sdHeadText[SDH_SHIMMERVERSION_BYTE_0] = DEVICE_VER >> 8;
   sdHeadText[SDH_SHIMMERVERSION_BYTE_1] = DEVICE_VER & 0xff;
   sdHeadText[SDH_FW_VERSION_TYPE_0] = FW_IDENTIFIER >> 8;
   sdHeadText[SDH_FW_VERSION_TYPE_1] = FW_IDENTIFIER & 0xff;
   sdHeadText[SDH_FW_VERSION_MAJOR_0] = FW_VER_MAJOR >> 8;
   sdHeadText[SDH_FW_VERSION_MAJOR_1] = FW_VER_MAJOR & 0xff;
   sdHeadText[SDH_FW_VERSION_MINOR] = FW_VER_MINOR;
   sdHeadText[SDH_FW_VERSION_INTERNAL] = FW_VER_REL;
   // exg
   sdHeadText[SDH_EXG_ADS1292R_1_CONFIG1] = storedConfig[NV_EXG_ADS1292R_1_CONFIG1];
   sdHeadText[SDH_EXG_ADS1292R_1_CONFIG2] = storedConfig[NV_EXG_ADS1292R_1_CONFIG2];
   sdHeadText[SDH_EXG_ADS1292R_1_LOFF] = storedConfig[NV_EXG_ADS1292R_1_LOFF];
   sdHeadText[SDH_EXG_ADS1292R_1_CH1SET] = storedConfig[NV_EXG_ADS1292R_1_CH1SET];
   sdHeadText[SDH_EXG_ADS1292R_1_CH2SET] = storedConfig[NV_EXG_ADS1292R_1_CH2SET];
   sdHeadText[SDH_EXG_ADS1292R_1_RLD_SENS] = storedConfig[NV_EXG_ADS1292R_1_RLD_SENS];
   sdHeadText[SDH_EXG_ADS1292R_1_LOFF_SENS] = storedConfig[NV_EXG_ADS1292R_1_LOFF_SENS];
   sdHeadText[SDH_EXG_ADS1292R_1_LOFF_STAT] = storedConfig[NV_EXG_ADS1292R_1_LOFF_STAT];
   sdHeadText[SDH_EXG_ADS1292R_1_RESP1] = storedConfig[NV_EXG_ADS1292R_1_RESP1];
   sdHeadText[SDH_EXG_ADS1292R_1_RESP2] = storedConfig[NV_EXG_ADS1292R_1_RESP2];
   sdHeadText[SDH_EXG_ADS1292R_2_CONFIG1] = storedConfig[NV_EXG_ADS1292R_2_CONFIG1];
   sdHeadText[SDH_EXG_ADS1292R_2_CONFIG2] = storedConfig[NV_EXG_ADS1292R_2_CONFIG2];
   sdHeadText[SDH_EXG_ADS1292R_2_LOFF] = storedConfig[NV_EXG_ADS1292R_2_LOFF];
   sdHeadText[SDH_EXG_ADS1292R_2_CH1SET] = storedConfig[NV_EXG_ADS1292R_2_CH1SET];
   sdHeadText[SDH_EXG_ADS1292R_2_CH2SET] = storedConfig[NV_EXG_ADS1292R_2_CH2SET];
   sdHeadText[SDH_EXG_ADS1292R_2_RLD_SENS] = storedConfig[NV_EXG_ADS1292R_2_RLD_SENS];
   sdHeadText[SDH_EXG_ADS1292R_2_LOFF_SENS] = storedConfig[NV_EXG_ADS1292R_2_LOFF_SENS];
   sdHeadText[SDH_EXG_ADS1292R_2_LOFF_STAT] = storedConfig[NV_EXG_ADS1292R_2_LOFF_STAT];
   sdHeadText[SDH_EXG_ADS1292R_2_RESP1] = storedConfig[NV_EXG_ADS1292R_2_RESP1];
   sdHeadText[SDH_EXG_ADS1292R_2_RESP2] = storedConfig[NV_EXG_ADS1292R_2_RESP2];

   sdHeadText[SDH_BT_COMMS_BAUD_RATE] = storedConfig[NV_BT_COMMS_BAUD_RATE];
   sdHeadText[SDH_DERIVED_CHANNELS_0] = storedConfig[NV_DERIVED_CHANNELS_0];
   sdHeadText[SDH_DERIVED_CHANNELS_1] = storedConfig[NV_DERIVED_CHANNELS_1];
   sdHeadText[SDH_DERIVED_CHANNELS_2] = storedConfig[NV_DERIVED_CHANNELS_2];
   // sd config
   sdHeadText[SDH_MYTRIAL_ID] = storedConfig[NV_SD_MYTRIAL_ID];
   sdHeadText[SDH_NSHIMMER] = storedConfig[NV_SD_NSHIMMER];
   sdHeadText[SDH_EST_EXP_LEN_MSB] = storedConfig[NV_EST_EXP_LEN_MSB];
   sdHeadText[SDH_EST_EXP_LEN_LSB] = storedConfig[NV_EST_EXP_LEN_LSB];
   sdHeadText[SDH_MAX_EXP_LEN_MSB] = storedConfig[NV_MAX_EXP_LEN_MSB];
   sdHeadText[SDH_MAX_EXP_LEN_LSB] = storedConfig[NV_MAX_EXP_LEN_LSB];
   sdHeadText[SDH_TRIAL_CONFIG0] = storedConfig[NV_SD_TRIAL_CONFIG0];
   sdHeadText[SDH_TRIAL_CONFIG1] = storedConfig[NV_SD_TRIAL_CONFIG1] = 0;
   sdHeadText[SDH_BROADCAST_INTERVAL] = storedConfig[NV_SD_BT_INTERVAL];

//   sdHeadText[SDH_RTC_DIFF_7] = *((uint8_t*)&rwcTimeDiff64);
//   sdHeadText[SDH_RTC_DIFF_6] = *(((uint8_t*)&rwcTimeDiff64)+1);
//   sdHeadText[SDH_RTC_DIFF_5] = *(((uint8_t*)&rwcTimeDiff64)+2);
//   sdHeadText[SDH_RTC_DIFF_4] = *(((uint8_t*)&rwcTimeDiff64)+3);
//   sdHeadText[SDH_RTC_DIFF_3] = *(((uint8_t*)&rwcTimeDiff64)+4);
//   sdHeadText[SDH_RTC_DIFF_2] = *(((uint8_t*)&rwcTimeDiff64)+5);
//   sdHeadText[SDH_RTC_DIFF_1] = *(((uint8_t*)&rwcTimeDiff64)+6);
//   sdHeadText[SDH_RTC_DIFF_0] = *(((uint8_t*)&rwcTimeDiff64)+7);

   memcpy(&sdHeadText[SDH_MAC_ADDR], &storedConfig[NV_MAC_ADDRESS], 6);
   memcpy(&sdHeadText[SDH_CONFIG_TIME_0], &storedConfig[NV_SD_CONFIG_TIME], 4);

//   memcpy(&sdHeadText[SDH_MPU9150_GYRO_CALIBRATION], &storedConfig[NV_MPU9150_GYRO_CALIBRATION], 21);
//   memcpy(&sdHeadText[SDH_LSM303DLHC_MAG_CALIBRATION], &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
//   memcpy(&sdHeadText[SDH_LSM303DLHC_ACCEL_CALIBRATION], &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
//   memcpy(&sdHeadText[SDH_A_ACCEL_CALIBRATION], &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
}





 
 
