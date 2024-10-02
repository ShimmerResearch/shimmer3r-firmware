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
#include "shimmer_definitions.h"

#include "Bluetooth/sd_sync.h"
#include "bmp3_defs.h"

uint8_t sdHeadText[SD_HEAD_SIZE], btMacAscii[14], btMacHex[6];
gConfigBytes storedConfig;
uint8_t calibRamFlag = 0;

void S4Ram_init(void)
{
  //init RAM (storedConfig)
#if USE_DEFAULT_SENSOR
  S4Ram_SetDefaultInfomem();
#else
  gConfigBytes temp_storedConfig;
  InfoMem_readRam(temp_storedConfig.rawBytes, 0, STOREDCONFIG_SIZE);
  if ((temp_storedConfig.rawBytes[NV_SENSORS1] == 0xFF)
      || temp_storedConfig.samplingRateTicks == 0)
  {
    //if config was never written to Infomem, write default
    //assuming some other app didn't make use of InfoMem, or else InfoMem was erased
    S4Ram_SetDefaultInfomem();
  }
  else
  {
    //memcpy(temp_storedConfig+NV_MAC_ADDRESS, btMacHex, 6);
    S4Ram_storedConfigSet(temp_storedConfig.rawBytes, 0, STOREDCONFIG_SIZE);
  }
#endif //USE_DEFAULT_SENSOR
  S4Ram_storedConfigSet(btMacHex, NV_MAC_ADDRESS, 6);
}

gConfigBytes *S4Ram_getStoredConfig(void)
{
  return &storedConfig;
}

uint8_t *S4Ram_getSdHeadText(void)
{
  return sdHeadText;
}

/*
 * sdHeadText: Set(), Get() and GetByte(), S4Ram_sdHeadTextSetByte()
 */
uint8_t S4Ram_sdHeadTextSet(const uint8_t *buf, uint16_t offset, uint16_t length)
{
  if ((offset > SD_HEAD_SIZE - 1) || (offset + length > SD_HEAD_SIZE) || (length == 0))
  {
    return 1;
  }
  memcpy(sdHeadText + offset, buf, length);
  return 0;
}

uint8_t S4Ram_sdHeadTextGet(uint8_t *buf, uint16_t offset, uint16_t length)
{
  if ((offset > SD_HEAD_SIZE - 1) || (offset + length > SD_HEAD_SIZE) || (length == 0))
  {
    return 1;
  }
  memcpy(buf, sdHeadText + offset, length);
  return 0;
}

uint8_t S4Ram_sdHeadTextGetByte(uint16_t offset)
{
  if (offset > SD_HEAD_SIZE - 1)
  {
    return 0;
  }
  return sdHeadText[offset];
}

uint8_t S4Ram_sdHeadTextSetByte(uint16_t offset, uint8_t val)
{
  if (offset > SD_HEAD_SIZE - 1)
  {
    return 1;
  }
  sdHeadText[offset] = val;
  return 0;
}

/*
 * storedConfig: Set(), Get() and GetByte(), S4Ram_sdHeadTextSetByte()
 */

uint8_t S4Ram_storedConfigSet(const uint8_t *buf, uint16_t offset, uint16_t length)
{
  if ((offset > STOREDCONFIG_SIZE - 1) || (offset + length > STOREDCONFIG_SIZE)
      || (length == 0))
  {
    return 1;
  }
  memcpy(&storedConfig.rawBytes[offset], buf, length);
  return 0;
}

uint8_t S4Ram_storedConfigGet(uint8_t *buf, uint16_t offset, uint16_t length)
{
  if ((offset > STOREDCONFIG_SIZE - 1) || (offset + length > STOREDCONFIG_SIZE)
      || (length == 0))
  {
    return 1;
  }
  memcpy(buf, &storedConfig.rawBytes[offset], length);
  return 0;
}

uint8_t S4Ram_storedConfigGetByte(uint16_t offset)
{
  if (offset > STOREDCONFIG_SIZE - 1)
  {
    return 0; //or not?
  }
  return storedConfig.rawBytes[offset];
}

uint8_t S4Ram_storedConfigSetByte(uint16_t offset, uint8_t val)
{
  if (offset > STOREDCONFIG_SIZE - 1)
  {
    return 1;
  }
  storedConfig.rawBytes[offset] = val;
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

void S4Ram_SetDefaultInfomem(void)
{
  //uint8_t temp_btMacAscii[14];

  memset(storedConfig.rawBytes, 0x00, sizeof(storedConfig.rawBytes));

  S4Ram_btMacHexGet(storedConfig.macAddr);

  storedConfig.samplingRateTicks = 32768 / 51.2; //51.2Hz
  storedConfig.bufferSize = 1;
  //core sensors enabled
  storedConfig.chEnLnAccel = 1;
  storedConfig.chEnMag = 1;
  //storedConfig.chEnExg1_24Bit = 1;
  //storedConfig.chEnExg2_24Bit = 1;
  storedConfig.chEnGyro = 1;
  storedConfig.chEnVBattery = 1;

#if defined(SHIMMER3)
  //LSM303DLHC Accel 100Hz, +/-2G, Low Power and High Resolution modes off
  storedConfig.wrAccelRate = LSM303DLHC_ACCEL_100HZ;
  storedConfig.wrAccelRange = ACCEL_2G;
  //MPU9150 sampling rate of 8kHz/(155+1), i.e. 51.282Hz
  storedConfig.gyroRate = 0x9B;
  //LSM303DLHC Mag 75Hz, +/-1.3 Gauss, MPU9150 Gyro +/-500 degrees per second
  storedConfig.magRange = LSM303DLHC_MAG_1_3G;
  storedConfig.magRateLsb = LSM303DLHC_MAG_75HZ;
  set_config_byte_gyro_range(&storedConfig, MPU9150_GYRO_500DPS);
  //MPU9150 Accel +/-2G, BMP pressure oversampling ratio 1, GSR auto range, EXP_RESET_N pin set low
  // todo: *** *** *** warning! *** *** ***  btStream for this here is not correct, this is mpu9150_accrange, not lsm303 acc range
  storedConfig.altAccelRange = ACCEL_2G;
  set_config_byte_pressure_oversampling_ratio(&storedConfig, BMP180_OSS_8);
#elif defined(SHIMMER3R)
  //LIS2DW12 Accel 100Hz, +/-2G, Low Power and High Resolution modes off
  storedConfig.wrAccelRate = LIS2DW12_XL_ODR_100Hz;
  storedConfig.wrAccelRange = LIS2DW12_2g;
  //LSM6DSV Gyro sampling rate, next highest to 51.2Hz
  set_config_byte_gyro_rate(&storedConfig, LSM6DSV_ODR_AT_60Hz);
  //LIS3MDL Mag 75Hz, +/-4 Gauss, MPU9150 Gyro +/-500 degrees per second
  storedConfig.magRange = LIS3MDL_4_GAUSS;
  set_config_byte_mag_rate(&storedConfig, LIS3MDL_UHP_80Hz);
  //LSM6DSV Gyro +/-500 degrees per second
  set_config_byte_gyro_range(&storedConfig, LSM6DSV_500dps);
  storedConfig.altAccelRange = LSM6DSV_2g;
  set_config_byte_pressure_oversampling_ratio(&storedConfig, BMP3_NO_OVERSAMPLING);
  set_config_byte_wr_accel_mode(&storedConfig, LIS2DW12_HIGH_PERFORMANCE);
  storedConfig.altMagRate = LIS2MDL_ODR_100Hz;
  storedConfig.pressureRate = BMP3_ODR_100_HZ;
#endif
  storedConfig.gsrRange = GSR_AUTORANGE;
  //set all ExG registers to their reset values
  //square wave test
  storedConfig.exgADS1292rRegsCh1.config1 = 0x04;
  storedConfig.exgADS1292rRegsCh1.config2 = 0xab; //was 0xa3 for rev1
  storedConfig.exgADS1292rRegsCh1.loff = 0x10;
  storedConfig.exgADS1292rRegsCh1.ch1set = 0x05;
  storedConfig.exgADS1292rRegsCh1.ch2set = 0x05;
  storedConfig.exgADS1292rRegsCh1.rldSens = 0x00;
  storedConfig.exgADS1292rRegsCh1.loffSens = 0x00;
  storedConfig.exgADS1292rRegsCh1.loffStat = 0x00;
  storedConfig.exgADS1292rRegsCh1.resp1 = 0x02;
  storedConfig.exgADS1292rRegsCh1.resp2 = 0x01;
  storedConfig.exgADS1292rRegsCh2.config1 = 0x04;
  storedConfig.exgADS1292rRegsCh2.config2 = 0xa3;
  storedConfig.exgADS1292rRegsCh2.loff = 0x10;
  storedConfig.exgADS1292rRegsCh2.ch1set = 0x05;
  storedConfig.exgADS1292rRegsCh2.ch2set = 0x05;
  storedConfig.exgADS1292rRegsCh2.rldSens = 0x00;
  storedConfig.exgADS1292rRegsCh2.loffSens = 0x00;
  storedConfig.exgADS1292rRegsCh2.loffStat = 0x00;
  storedConfig.exgADS1292rRegsCh2.resp1 = 0x02;
  storedConfig.exgADS1292rRegsCh2.resp2 = 0x01;

  //ecg
  //storedConfig.exgADS1292R_1_CONFIG1 = 0x02;
  //storedConfig.exgADS1292R_1_CONFIG2 = 0x80;
  //storedConfig.exgADS1292R_1_LOFF = 0x10;
  //storedConfig.exgADS1292R_1_CH1SET = 0x00;
  //storedConfig.exgADS1292R_1_CH2SET = 0x00;
  //storedConfig.exgADS1292R_1_RLD_SENS = 0x00;
  //storedConfig.exgADS1292R_1_LOFF_SENS = 0x00;
  //storedConfig.exgADS1292R_1_LOFF_STAT = 0x00;
  //storedConfig.exgADS1292R_1_RESP1 = 0x00;
  //storedConfig.exgADS1292R_1_RESP2 = 0x02;
  //storedConfig.exgADS1292R_2_CONFIG1 = 0x02;
  //storedConfig.exgADS1292R_2_CONFIG2 = 0x80;
  //storedConfig.exgADS1292R_2_LOFF = 0x10;
  //storedConfig.exgADS1292R_2_CH1SET = 0x00;
  //storedConfig.exgADS1292R_2_CH2SET = 0x00;
  //storedConfig.exgADS1292R_2_RLD_SENS = 0x00;
  //storedConfig.exgADS1292R_2_LOFF_SENS = 0x00;
  //storedConfig.exgADS1292R_2_LOFF_STAT = 0x00;
  //storedConfig.exgADS1292R_2_RESP1 = 0x00;
  //storedConfig.exgADS1292R_2_RESP2 = 0x02;

  //if(storedConfig.btCommsBaudRate == 0xFF)
  //{
  //  storedConfig.btCommsBaudRate = 9;
  //}

  //sd config
  //shimmername
  setDefaultShimmerName();
  //exp_id
  setDefaultTrialId();
  storedConfig.configTime = 0;

  storedConfig.myTrialID = 0;
  storedConfig.numberOfShimmers = 0;
  storedConfig.userButtonEnable = 1;
  storedConfig.rtcErrorEnable = 1;
  storedConfig.sdErrorEnable = 1;
  storedConfig.btInterval = 54;

  storedConfig.experimentLengthMaxInMinutes = 0;

  storedConfig.experimentLengthEstimatedInSec = 1;
  setSyncEstExpLen((uint32_t) storedConfig.experimentLengthEstimatedInSec);

  InfoMem_update();
}

void S4Ram_config2SdHead(void)
{
  memset(sdHeadText, 0xff, SD_HEAD_SIZE);

  sdHeadText[SDH_SAMPLE_RATE_0] = storedConfig.rawBytes[NV_SAMPLING_RATE];
  sdHeadText[SDH_SAMPLE_RATE_1] = storedConfig.rawBytes[NV_SAMPLING_RATE + 1];
  sdHeadText[SDH_BUFFER_SIZE] = storedConfig.rawBytes[NV_BUFFER_SIZE];
  sdHeadText[SDH_SENSORS0] = storedConfig.rawBytes[NV_SENSORS0];
  sdHeadText[SDH_SENSORS1] = storedConfig.rawBytes[NV_SENSORS1];
  sdHeadText[SDH_SENSORS2] = storedConfig.rawBytes[NV_SENSORS2];
  sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig.rawBytes[NV_CONFIG_SETUP_BYTE0];
  sdHeadText[SDH_CONFIG_SETUP_BYTE1] = storedConfig.rawBytes[NV_CONFIG_SETUP_BYTE1];
  sdHeadText[SDH_CONFIG_SETUP_BYTE2] = storedConfig.rawBytes[NV_CONFIG_SETUP_BYTE2];
  sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig.rawBytes[NV_CONFIG_SETUP_BYTE3];
  sdHeadText[SDH_SENSORS3] = storedConfig.rawBytes[NV_SENSORS3];
  sdHeadText[SDH_SENSORS4] = storedConfig.rawBytes[NV_SENSORS4];
  sdHeadText[SDH_CONFIG_SETUP_BYTE4] = storedConfig.rawBytes[NV_CONFIG_SETUP_BYTE4];
  sdHeadText[SDH_CONFIG_SETUP_BYTE5] = storedConfig.rawBytes[NV_CONFIG_SETUP_BYTE5];
  sdHeadText[SDH_CONFIG_SETUP_BYTE6] = storedConfig.rawBytes[NV_CONFIG_SETUP_BYTE6];
  //little endian in fw, but they want big endian in sw trivial
  sdHeadText[SDH_SHIMMERVERSION_BYTE_0] = DEVICE_VER >> 8;
  sdHeadText[SDH_SHIMMERVERSION_BYTE_1] = DEVICE_VER & 0xff;
  sdHeadText[SDH_FW_VERSION_TYPE_0] = FW_IDENTIFIER >> 8;
  sdHeadText[SDH_FW_VERSION_TYPE_1] = FW_IDENTIFIER & 0xff;
  sdHeadText[SDH_FW_VERSION_MAJOR_0] = FW_VER_MAJOR >> 8;
  sdHeadText[SDH_FW_VERSION_MAJOR_1] = FW_VER_MAJOR & 0xff;
  sdHeadText[SDH_FW_VERSION_MINOR] = FW_VER_MINOR;
  sdHeadText[SDH_FW_VERSION_INTERNAL] = FW_VER_REL;
  //exg
  sdHeadText[SDH_EXG_ADS1292R_1_CONFIG1] = storedConfig.rawBytes[NV_EXG_ADS1292R_1_CONFIG1];
  sdHeadText[SDH_EXG_ADS1292R_1_CONFIG2] = storedConfig.rawBytes[NV_EXG_ADS1292R_1_CONFIG2];
  sdHeadText[SDH_EXG_ADS1292R_1_LOFF] = storedConfig.rawBytes[NV_EXG_ADS1292R_1_LOFF];
  sdHeadText[SDH_EXG_ADS1292R_1_CH1SET] = storedConfig.rawBytes[NV_EXG_ADS1292R_1_CH1SET];
  sdHeadText[SDH_EXG_ADS1292R_1_CH2SET] = storedConfig.rawBytes[NV_EXG_ADS1292R_1_CH2SET];
  sdHeadText[SDH_EXG_ADS1292R_1_RLD_SENS]
      = storedConfig.rawBytes[NV_EXG_ADS1292R_1_RLD_SENS];
  sdHeadText[SDH_EXG_ADS1292R_1_LOFF_SENS]
      = storedConfig.rawBytes[NV_EXG_ADS1292R_1_LOFF_SENS];
  sdHeadText[SDH_EXG_ADS1292R_1_LOFF_STAT]
      = storedConfig.rawBytes[NV_EXG_ADS1292R_1_LOFF_STAT];
  sdHeadText[SDH_EXG_ADS1292R_1_RESP1] = storedConfig.rawBytes[NV_EXG_ADS1292R_1_RESP1];
  sdHeadText[SDH_EXG_ADS1292R_1_RESP2] = storedConfig.rawBytes[NV_EXG_ADS1292R_1_RESP2];
  sdHeadText[SDH_EXG_ADS1292R_2_CONFIG1] = storedConfig.rawBytes[NV_EXG_ADS1292R_2_CONFIG1];
  sdHeadText[SDH_EXG_ADS1292R_2_CONFIG2] = storedConfig.rawBytes[NV_EXG_ADS1292R_2_CONFIG2];
  sdHeadText[SDH_EXG_ADS1292R_2_LOFF] = storedConfig.rawBytes[NV_EXG_ADS1292R_2_LOFF];
  sdHeadText[SDH_EXG_ADS1292R_2_CH1SET] = storedConfig.rawBytes[NV_EXG_ADS1292R_2_CH1SET];
  sdHeadText[SDH_EXG_ADS1292R_2_CH2SET] = storedConfig.rawBytes[NV_EXG_ADS1292R_2_CH2SET];
  sdHeadText[SDH_EXG_ADS1292R_2_RLD_SENS]
      = storedConfig.rawBytes[NV_EXG_ADS1292R_2_RLD_SENS];
  sdHeadText[SDH_EXG_ADS1292R_2_LOFF_SENS]
      = storedConfig.rawBytes[NV_EXG_ADS1292R_2_LOFF_SENS];
  sdHeadText[SDH_EXG_ADS1292R_2_LOFF_STAT]
      = storedConfig.rawBytes[NV_EXG_ADS1292R_2_LOFF_STAT];
  sdHeadText[SDH_EXG_ADS1292R_2_RESP1] = storedConfig.rawBytes[NV_EXG_ADS1292R_2_RESP1];
  sdHeadText[SDH_EXG_ADS1292R_2_RESP2] = storedConfig.rawBytes[NV_EXG_ADS1292R_2_RESP2];

  sdHeadText[SDH_BT_COMMS_BAUD_RATE] = storedConfig.rawBytes[NV_BT_COMMS_BAUD_RATE];
  sdHeadText[SDH_DERIVED_CHANNELS_0] = storedConfig.rawBytes[NV_DERIVED_CHANNELS_0];
  sdHeadText[SDH_DERIVED_CHANNELS_1] = storedConfig.rawBytes[NV_DERIVED_CHANNELS_1];
  sdHeadText[SDH_DERIVED_CHANNELS_2] = storedConfig.rawBytes[NV_DERIVED_CHANNELS_2];
  sdHeadText[SDH_DERIVED_CHANNELS_3] = storedConfig.rawBytes[NV_DERIVED_CHANNELS_3];
  sdHeadText[SDH_DERIVED_CHANNELS_4] = storedConfig.rawBytes[NV_DERIVED_CHANNELS_4];
  sdHeadText[SDH_DERIVED_CHANNELS_5] = storedConfig.rawBytes[NV_DERIVED_CHANNELS_5];
  sdHeadText[SDH_DERIVED_CHANNELS_6] = storedConfig.rawBytes[NV_DERIVED_CHANNELS_6];
  sdHeadText[SDH_DERIVED_CHANNELS_7] = storedConfig.rawBytes[NV_DERIVED_CHANNELS_7];

  /* sd config */
  sdHeadText[SDH_MYTRIAL_ID] = storedConfig.rawBytes[NV_SD_MYTRIAL_ID];
  sdHeadText[SDH_NSHIMMER] = storedConfig.rawBytes[NV_SD_NSHIMMER];
  sdHeadText[SDH_EST_EXP_LEN_MSB] = storedConfig.rawBytes[NV_EST_EXP_LEN_MSB];
  sdHeadText[SDH_EST_EXP_LEN_LSB] = storedConfig.rawBytes[NV_EST_EXP_LEN_LSB];
  sdHeadText[SDH_MAX_EXP_LEN_MSB] = storedConfig.rawBytes[NV_MAX_EXP_LEN_MSB];
  sdHeadText[SDH_MAX_EXP_LEN_LSB] = storedConfig.rawBytes[NV_MAX_EXP_LEN_LSB];
  sdHeadText[SDH_TRIAL_CONFIG0] = storedConfig.rawBytes[NV_SD_TRIAL_CONFIG0];
  sdHeadText[SDH_TRIAL_CONFIG1] = storedConfig.rawBytes[NV_SD_TRIAL_CONFIG1] = 0;
  sdHeadText[SDH_BROADCAST_INTERVAL] = storedConfig.rawBytes[NV_SD_BT_INTERVAL];

  //sdHeadText[SDH_RTC_DIFF_7] = *((uint8_t*)&rwcTimeDiff64);
  //sdHeadText[SDH_RTC_DIFF_6] = *(((uint8_t*)&rwcTimeDiff64)+1);
  //sdHeadText[SDH_RTC_DIFF_5] = *(((uint8_t*)&rwcTimeDiff64)+2);
  //sdHeadText[SDH_RTC_DIFF_4] = *(((uint8_t*)&rwcTimeDiff64)+3);
  //sdHeadText[SDH_RTC_DIFF_3] = *(((uint8_t*)&rwcTimeDiff64)+4);
  //sdHeadText[SDH_RTC_DIFF_2] = *(((uint8_t*)&rwcTimeDiff64)+5);
  //sdHeadText[SDH_RTC_DIFF_1] = *(((uint8_t*)&rwcTimeDiff64)+6);
  //sdHeadText[SDH_RTC_DIFF_0] = *(((uint8_t*)&rwcTimeDiff64)+7);

  memcpy(&sdHeadText[SDH_MAC_ADDR], &storedConfig.rawBytes[NV_MAC_ADDRESS], 6);
  memcpy(&sdHeadText[SDH_CONFIG_TIME_0], &storedConfig.rawBytes[NV_SD_CONFIG_TIME], 4);

  /* BMP180 had 22 bytes stored in index SDH_TEMP_PRES_CALIBRATION. BMP280 had
   * 24 bytes spread accross the 22 available bytes in SDH_TEMP_PRES_CALIBRATION
   * and a further 2 bytes in BMP280_XTRA_CALIB_BYTES. BMP390 uses 21 bytes */
  memcpy(&sdHeadText[SDH_TEMP_PRES_CALIBRATION], get_bmp_calib_data_bytes(), BMP3_LEN_CALIB_DATA);

  //memcpy(&sdHeadText[SDH_MPU9150_GYRO_CALIBRATION],
  //&storedConfig.rawBytes[NV_MPU9150_GYRO_CALIBRATION], 21);
  //memcpy(&sdHeadText[SDH_LSM303DLHC_MAG_CALIBRATION], &storedConfig.rawBytes[NV_LSM303DLHC_MAG_CALIBRATION],
  //21); memcpy(&sdHeadText[SDH_LSM303DLHC_ACCEL_CALIBRATION], &storedConfig.rawBytes[NV_LSM303DLHC_ACCEL_CALIBRATION],
  //21); memcpy(&sdHeadText[SDH_A_ACCEL_CALIBRATION], &storedConfig.rawBytes[NV_A_ACCEL_CALIBRATION],
  //21);

  ShimmerCalibSyncFromDumpRamAll();
  memcpy(&sdHeadText[SDH_DAUGHTER_CARD_ID_BYTE0], &getDaughtCardId()->exp_brd_id, 3);
}

void setDefaultShimmerName(void)
{
  strcpy(&storedConfig.shimmerName[0], "Shimmer_XXXX");
  memcpy(&storedConfig.shimmerName[8], btMacAscii + 8, 4);
}

void setDefaultTrialId(void)
{
  memcpy(&storedConfig.expIdName[0], "DefaultTrial", 12);
}

uint8_t GetSdCfgFlag(void)
{
  uint8_t sd_config_delay_flag = 0;
  InfoMem_readRam(&sd_config_delay_flag, NV_SD_CONFIG_DELAY_FLAG, 1);
  if (!(sd_config_delay_flag & 0x80))
  {
    if (sd_config_delay_flag & 0x01)
      return 1;
  }
  return 0;
}

void SetSdCfgFlag(uint8_t flag)
{
  gConfigBytes temp_storedConfig;
  InfoMem_readRam(temp_storedConfig.rawBytes, 0, STOREDCONFIG_SIZE);
  if (flag)
  {
    if (!temp_storedConfig.sdCfgFlag)
    {
      temp_storedConfig.infoSdcfg |= 0x01;
    }
    else
    {
      temp_storedConfig.infoSdcfg = 0x01;
    }
  }
  else
  {
    temp_storedConfig.infoSdcfg &= ~0x01;
  }
  InfoMem_updateFrom(temp_storedConfig.rawBytes);
}

uint8_t GetRamCalibFlag(void)
{
  return calibRamFlag;
}

void SetRamCalibFlag(uint8_t flag)
{
  //flag == 1: Ram>File, ShimmerCalib_ram2File()
  //        0: File>Ram, ShimmerCalib_file2Ram()
  calibRamFlag = flag;
}

float get_shimmer_sampling_freq(void)
{
  return 32768.0 / (float) storedConfig.samplingRateTicks;
}

void set_config_byte_gyro_range(gConfigBytes *storedConfigPtr, uint8_t value)
{
#if defined(SHIMMER3)
  value = (value <= MPU9250_GYRO_2000DPS) ? value : MPU9250_GYRO_500DPS;
#elif defined(SHIMMER3R)
  value = (value <= (LSM6DSV_2000dps + 1)) ? value : LSM6DSV_500dps;
#endif
  storedConfigPtr->gyroRangeLsb = value & 0x03;
  storedConfigPtr->gyroRangeMsb = (value >> 2) & 0x01;
}

uint8_t get_config_byte_gyro_range(void)
{
  return (storedConfig.gyroRangeMsb << 2) | storedConfig.gyroRangeLsb;
}

void set_config_byte_gyro_rate(gConfigBytes *storedConfigPtr, uint8_t value)
{
#if defined(SHIMMER3)
#elif defined(SHIMMER3R)
  value = (value < LSM6DSV_ODR_AT_7680Hz) ? value : LSM6DSV_ODR_AT_60Hz;
#endif
  storedConfigPtr->gyroRate = value;
}

void set_config_byte_wr_accel_lp_mode(gConfigBytes *storedConfigPtr, uint8_t value)
{
#if defined(SHIMMER3)
  value = (value == 1) ? 1 : 0;
#elif defined(SHIMMER3R)
  value = (value <= 3) ? value : 0;
#endif
  storedConfigPtr->wrAccelLpModeLsb = value & 0x01;
  storedConfigPtr->wrAccelLpModeMsb = (value >> 1) & 0x01;
}

uint8_t get_config_byte_wr_accel_lp_mode(void)
{
  return (storedConfig.wrAccelLpModeMsb << 1) | storedConfig.wrAccelLpModeLsb;
}

void set_config_byte_wr_accel_mode(gConfigBytes *storedConfigPtr, lis2dw12_mode_t value)
{
  storedConfigPtr->wrAccelHrMode = (value >> 2) & 0x01;
  set_config_byte_wr_accel_lp_mode(storedConfigPtr, value & 0x03);
}

lis2dw12_mode_t get_config_byte_wr_accel_mode(void)
{
  lis2dw12_mode_t wrAccelMode = (lis2dw12_mode_t) ((storedConfig.wrAccelHrMode << 2)
      | get_config_byte_wr_accel_lp_mode());
  return wrAccelMode;
}

void set_config_byte_pressure_oversampling_ratio(gConfigBytes *storedConfigPtr, uint8_t value)
{
#if defined(SHIMMER3)
  value = (value < 4) ? (value & 0x03) : BMP180_OSS_1;
#elif defined(SHIMMER3R)
  value = (value <= BMP3_OVERSAMPLING_32X) ? value : BMP3_NO_OVERSAMPLING;
#endif
  storedConfigPtr->pressureOversamplingRatioLsb = value & 0x03;
  storedConfigPtr->pressureOversamplingRatioMsb = (value >> 2) & 0x01;
}

uint8_t get_config_byte_pressure_oversampling_ratio(void)
{
  return (storedConfig.pressureOversamplingRatioMsb << 2)
      | storedConfig.pressureOversamplingRatioLsb;
}

void set_config_byte_mag_rate(gConfigBytes *storedConfigPtr, uint8_t value)
{
#if defined(SHIMMER3)
  value = (value < 4) ? (value & 0x03) : BMP180_OSS_1;
#elif defined(SHIMMER3R)
  value = value <= (LIS3MDL_UHP_80Hz) ? value : LIS3MDL_UHP_80Hz;
#endif
  storedConfigPtr->magRateLsb = value & 0x07;
  storedConfigPtr->magRateMsb = (value >> 3) & 0x07;
}

uint8_t get_config_byte_mag_rate(void)
{
  return (storedConfig.magRateMsb << 3) | storedConfig.magRateLsb;
}
