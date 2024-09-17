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

#include "s4_sensing.h"

SENSINGTypeDef sensing;
//static STATTypeDef * pStat;

ADCTypeDef *sensing_adc;
I2C_TypeDef *sensing_i2c;
I2C_TypeDef *sensing_i2c_batt;

#if defined(SHIMMER3R)
uint8_t expectedCbFlags = 0, currentCbFlags = 0;
#elif defined(SHIMMER4_SDK)
uint32_t temp_cnt1, temp_cnt2, temp_cnt3, temp_cnt4;
#endif

//I2CBatteryTypeDef *sensing_i2c_batt;

//uint8_t cc[MAX_NUM_CHANNELS], nbrAdcChans, nbrDigiChans;

void S4Sens_init(void)
{

  //sensing.en = 0;
  //sensing.nbrAdcChans = 0;
  //sensing.nbrDigiChans = 0;
  //sensing.ccLen = 0;
  memset((uint8_t *) &sensing, 0, sizeof(sensing));
}

//SENSINGTypeDef* S4Sens_getSensing(void){
//   return &sensing;
//}

void S4Sens_configureChannels(void)
{
  sensing.nbrAdcChans = sensing.nbrDigiChans = 0;
  sensing.ccLen = 0;
  sensing.ptr.ts = 1;
  sensing.dataLen = 1 + 3; //0x00 + timestamp

  S4_ADC_configureChannels();
  I2C_configureChannels();
  SPI_configureChannels();

#if defined(SHIMMER3R)
  expectedCbFlags = 0;
  if (areAdcChannelsEnabled())
  {
    expectedCbFlags |= STAT_PERI_ADC;
  }
  if (areI2cChannelsEnabled())
  {
    expectedCbFlags |= STAT_PERI_I2C_SENS;
  }
  if (areSpiChannelsEnabled())
  {
    expectedCbFlags |= STAT_PERI_SPI_SENS;
  }
#endif
}

uint8_t S4Sens_checkStartSensorConditions(void)
{
  if (stat.isSensing)
  {
    return 0;
  }
  if (!((stat.sdlogCmd == 1 && stat.isSdInserted && !stat.badFile)
          || (stat.btstreamCmd == BT_STREAM_CMD_STATE_START && stat.isBtConnected)))
  {
    return 0;
  }
  return 1;
}

uint8_t S4Sens_checkStartLoggingConditions(void)
{
  if (stat.isSdInserted)
  {
    if ((!stat.isLogging) && (stat.sdlogCmd == 1))
    {
      stat.isLogging = 1;
      stat.sdlogCmd = 0;
      return 1; //good to go
    }
    if ((stat.isLogging) && (stat.sdlogCmd == 2))
    {
      stat.isLogging = 0;
      stat.sdlogCmd = 0;
      return 0;
    }
  }
  else
  {
    return 0;
  }
  return 0;
}

uint8_t S4Sens_checkStartStreamingConditions(void)
{
  if (stat.isBtConnected)
  {
    if (!stat.isStreaming && (stat.btstreamCmd == BT_STREAM_CMD_STATE_START))
    {
      stat.isStreaming = 1;
      stat.btstreamCmd = BT_STREAM_CMD_STATE_IDLE;
    }
    if (stat.isStreaming && (stat.btstreamCmd == BT_STREAM_CMD_STATE_STOP))
    {
      stat.isStreaming = 0;
      setBtCrcMode(CRC_OFF);
      stat.btstreamCmd = BT_STREAM_CMD_STATE_IDLE;
    }
  }
  else
  {
    stat.isStreaming = 0;
    setBtCrcMode(CRC_OFF);
  }
  return 0;
}

void S4Sens_startSensing(void)
{
  //if(stat.isDocked){
  //   return;
  //}

  stat.isConfiguring = 1;
  if (S4Sens_checkStartSensorConditions())
  {
    stat.isSensing = 1;
    sensing.isFileCreated = 0;
    S4Sens_configureChannels();

    if (areAnyChannelsEnabled())
    {
      Board_enableSensingPower(1);
    }
    else
    {
      stat.isConfiguring = 0;
      stat.isSensing = 0;
      return;
    }

    uint16_t samplingRateTicks = S4Ram_getStoredConfig()->samplingRateTicks;
    sensing.freq = get_shimmer_sampling_freq();
    if (sensing.freq > 4096.0)
    { //Please don't go too fast, Thx, Best Regards.
      stat.isConfiguring = 0;
      stat.isSensing = 0;
      return;
    }
    sensing.clkInterval4096 = (uint16_t) 4096
        / sensing.freq; //216000000 = 8192*26367 or 108000000 = 4096*26367
    sensing.clkInterval16k = samplingRateTicks / 2;

    DockUart_disable();
    S4Sens_stepInit();

    if (areAdcChannelsEnabled())
    {
      S4_ADC_startSensing();
    }
    I2C_startSensing();
    SPI_startSensing();

    if (S4Ram_getStoredConfig()->chEnMicrophone)
    {
      MX_MDF1_Init();
    }

    //I2cSensing(1);// gather the first set of sample?
    //I2cBattMonitor(1);

//TIM_startSensing();
//sensing.clkInterval4096
//if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, sensing.clkInterval16k-1, RTC_WAKEUPCLOCK_RTCCLK_DIV2) != HAL_OK)
//{
//   Error_Handler();
//}
#if SENS_CLK_RTC0TIM1
    TIM_startSensing();
#else
    S4_RTC_WakeUpSet(samplingRateTicks);
#endif

    sensing.startTs = RTC_get64();

    if (S4Sens_checkStartLoggingConditions())
    {
      if(!isSdPowerOn())
      {
        SdPowerOn();
      }
      SD_fileInit();
    }
  }

  stat.isConfiguring = 0;
}

uint8_t S4Sens_checkStopSensorConditions(void)
{
  if (!stat.isLogging && stat.isStreaming)
  { //streaming only case
    if (stat.btstreamCmd != BT_STREAM_CMD_STATE_STOP)
    {
      return 0;
    }
  }
  else if (stat.isLogging && !stat.isStreaming)
  { //logging only case
    if (stat.sdlogCmd != 2)
    {
      return 0;
    }
  }
  else if (stat.isLogging && stat.isStreaming)
  {
    if ((stat.btstreamCmd != BT_STREAM_CMD_STATE_STOP) || (stat.sdlogCmd != 2))
    {
      return 0;
    }
  }
  else
  {
  }

  return 1;
}

uint8_t S4Sens_checkStopLoggingConditions(void)
{
  if (stat.sdlogCmd != 2)
  {
    return 0;
  }
  else
  {
    stat.isLogging = 0;
    stat.sdlogCmd = 0;
    return 1;
  }
}

void S4Sens_stopSensing(void)
{
  if (!stat.isSensing)
  {
    return;
  }
  if (S4Sens_checkStopLoggingConditions())
  {
    SD_close();
    HAL_Delay(10);
  }
  if (S4Sens_checkStopSensorConditions())
  {
    stat.isConfiguring = 1;
    stat.isSensing = 0;
    stat.isStreaming = 0;
    setBtCrcMode(CRC_OFF);
    sensing.startTs = 0;
    //sensing.isSampling = 0;
    S4Sens_stopPeripherals();
    DockUart_enable();
  }

  //stat.sdlogCmd = 0;
  stat.btstreamCmd = BT_STREAM_CMD_STATE_IDLE;

  if (isSdInfoSyncDelayed())
  {
    SdInfoSync();
  }
 if (isSdPowerOn())
 {
   SdPowerOff();
 }
  stat.isConfiguring = 0;
}

void S4Sens_stopPeripherals(void)
{

#if SENS_CLK_RTC0TIM1
  TIM_stopSensing();
#else

#if defined(SHIMMER4_SDK)
  S4_RTC_WakeUpSetSlow();
#elif defined(SHIMMER3R)
  S4_RTC_WakeUpOff();
#endif

#endif

  if (areAdcChannelsEnabled())
  {
    S4_ADC_stopSensing();
  }
  //HAL_Delay(10); //Send ACK command needs delay here...
  //BtUart_sendRsp();
  I2C_stopSensing();
  SPI_stopSensing();

  Board_enableSensingPower(0);

  if (S4Ram_getStoredConfig()->chEnMicrophone)
  {
    MDF1_DeInit();
  }
}

void S4Sens_streamData(void)
{
#if SKIP_50MS
  if (sensing.startTs == 0xffffffffffffffff)
  {
  }
  else if ((sensing.startTs == 0) || (sensing.latestTs - sensing.startTs < 1638))
  {
    sensing.startTs = 0xffffffffffffffff;
    return;
  }
  else
  {
    sensing.startTs = 0xffffffffffffffff;
  }
#endif

  S4Sens_bufPoll();

  //ExpUart_TxIT(sensing.dataBuf, sensing.dataLen);

#if defined(SHIMMER4_SDK)
  //HAL_Delay(500);
  saveData();
#endif

  //sensing.isSampling = 0;
}

void S4Sens_bufPoll()
{
  if (areAdcChannelsEnabled())
  {
    S4_ADC_gatherDataStart();
  }

  I2C_pollSensors();

  SPI_pollSensors();
}

//this is to be called in the ISR
void S4Sens_gatherData(void)
{
  if (stat.isSensing)
  {
    //sensing.latestTs += ((sensing.latestTs-RTC_get64())/sensing.clkInterval4096)*sensing.clkInterval4096;
    sensing.latestTs = RTC_get64();
    sensing.dataBuf[sensing.ptr.ts] = sensing.latestTs & 0xff;
    sensing.dataBuf[sensing.ptr.ts + 1] = (sensing.latestTs >> 8) & 0xff;
    sensing.dataBuf[sensing.ptr.ts + 2] = (sensing.latestTs >> 16) & 0xff;

    //Task_set(TASK_STREAMDATA);//
#if defined(SHIMMER3R)
    sensing_start();
#elif defined(SHIMMER4_SDK)
    S4Sens_step1Start();
#endif

    //S4Sens_streamData();

    //I2cSensing(1);
  }
}

void S4Sens_stepInit(void)
{
#if defined(SHIMMER3R)
  S4_ADC_gatherDataCb(sensing_adcCompleteCb);
  I2cSens_gatherDataCb(sensing_i2cCompleteCb);
  SPI_gatherDataCb(sensing_spiCompleteCb);
#elif defined(SHIMMER4_SDK)
  S4_ADC_gatherDataCb(S4Sens_step2Start);
  //I2C_gatherDataInit(S4Sens_step3Start);
  //I2C2_gatherDataInit(S4Sens_step4Start);
  I2cSens_gatherDataCb(S4Sens_step3Start);
  I2cBatt_gatherDataCb(S4Sens_step4Start);
  SPI_gatherDataCb(S4Sens_step5Start);
  temp_cnt1 = temp_cnt2 = temp_cnt3 = temp_cnt4 = 0;
#endif
}

#if defined(SHIMMER3R)
void sensing_start(void)
{
  currentCbFlags = 0;
  S4Sens_streamData();
}

void sensing_adcCompleteCb(void)
{
  sensing_stageCompleteCb(STAT_PERI_ADC);
}

void sensing_i2cCompleteCb(void)
{
  sensing_stageCompleteCb(STAT_PERI_I2C_SENS);
}

void sensing_spiCompleteCb(void)
{
  sensing_stageCompleteCb(STAT_PERI_SPI_SENS);
}

void sensing_stageCompleteCb(uint8_t stage)
{
  currentCbFlags |= stage;
  if (currentCbFlags == expectedCbFlags)
  {
    //saveData();
    S4_Task_set(TASK_SAVEDATA);
  }
}

#elif defined(SHIMMER4_SDK)
void S4Sens_step1Start(void)
{
  PeriStat_Set(STAT_PERI_ADC | STAT_PERI_I2C_SENS | STAT_PERI_I2C_BATT | STAT_PERI_SPI_SENS);
  S4Sens_streamData();
  S4_ADC_gatherDataStart();
  if (temp_cnt2 == 1000)
  {
    __NOP();
    __NOP();
    __NOP();
  }
}

void S4Sens_step2Start(void)
{
  PeriStat_Clr(STAT_PERI_ADC);
  temp_cnt2++;
  //I2C_gatherDataStart();
  I2cSens_gatherDataStart();
}

void S4Sens_step3Start(void)
{
  PeriStat_Clr(STAT_PERI_I2C_SENS);
  temp_cnt3++;
  I2cBatt_gatherDataStart();
}

void S4Sens_step4Start(void)
{
  PeriStat_Clr(STAT_PERI_I2C_BATT);
  //SPI_gatherDataStart();
  S4Sens_step5Start();
}

void S4Sens_step5Start(void)
{
  PeriStat_Clr(STAT_PERI_SPI_SENS);
  temp_cnt4++;
  //S4Sens_streamData();
  S4Sens_stepDone();
}
#endif
void S4Sens_stepDone(void)
{
#if defined(SHIMMER3R)

#endif
}

#if defined(SHIMMER4_SDK)
//void S4Sens_step1End(void){S4Sens_step2Start();}
//void S4Sens_step2End(void){S4Sens_step3Start();}
//void S4Sens_step3End(void){S4Sens_step4Start();}
//void S4Sens_step4End(void){S4Sens_step5Start();}
//void S4Sens_step5End(void){S4Sens_step6Start();}
//void S4Sens_step6End(void){}
#endif

void saveData(void)
{
#if USE_SD
  if (stat.isLogging)
  {
    PeriStat_Set(STAT_PERI_SDMMC);
    SD_writeToBuff(sensing.dataBuf + 1, sensing.dataLen - 1);
    PeriStat_Clr(STAT_PERI_SDMMC);
  }
#endif
#if USE_BT
  S4Sens_checkStartStreamingConditions();
  if (stat.isStreaming)
  {
    uint8_t crcMode = getBtCrcMode();
    if (crcMode != CRC_OFF)
    {
      calculateCrcAndInsert(crcMode, sensing.dataBuf, sensing.dataLen);
    }

    if (BT_write(sensing.dataBuf, sensing.dataLen + crcMode) == HAL_OK)
    {
    }
  }
#endif

  if ((!stat.isLogging) && (!stat.isStreaming))
  {
    S4_Task_set(TASK_STOPSENSING);
  }
}

uint8_t areAnyChannelsEnabled(void)
{
  if (sensing.nbrAdcChans > 0 || sensing.nbrDigiChans > 0)
  {
    return 1;
  }
  return 0;
}
