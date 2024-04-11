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

uint32_t temp_cnt1, temp_cnt2, temp_cnt3, temp_cnt4;

//I2CBatteryTypeDef *sensing_i2c_batt;

//uint8_t cc[MAX_NUM_CHANNELS], nbrAdcChans, nbrDigiChans;

void S4Sens_init(void){
   
   //sensing.en = 0;
//   sensing.nbrAdcChans = 0;
//   sensing.nbrDigiChans = 0;   
//   sensing.ccLen = 0;
   memset((uint8_t*)&sensing, 0, sizeof(sensing));
}

//SENSINGTypeDef* S4Sens_getSensing(void){
//   return &sensing;
//}

void S4Sens_configureChannels(void) {
   sensing.nbrAdcChans = sensing.nbrDigiChans = 0;
   sensing.ccLen = 0;
   sensing.ptr.ts = 1;
   sensing.dataLen = 1 + 3; // 0x00 + timestamp
   
   S4_ADC_configureChannels();
   I2C_configureChannels();
   SPI_configureChannels();
}

uint8_t S4Sens_checkStartSensorConditions(void){
   if(stat.isSensing){
      return 0;
   }
   if(!((stat.sdlogCmd == 1    && stat.isSdInserted) || 
        (stat.btstreamCmd == 1 && stat.isBtConnected))){      
      return 0;
   }        
   return 1;
}
uint8_t S4Sens_checkStartLoggingConditions(void){
   if(stat.isSdInserted){
      if((!stat.isLogging) && (stat.sdlogCmd == 1 )){
         stat.isLogging = 1;
         stat.sdlogCmd = 0;
         return 1;//good to go
      }
      if((stat.isLogging) && (stat.sdlogCmd == 2 )){
         stat.isLogging = 0;
         stat.sdlogCmd = 0;
         return 0;
      }
   }else{      
      return 0;
   }
   return 0;
}
uint8_t S4Sens_checkStartStreamingConditions(void){
   if(stat.isBtConnected){
      if((!stat.isStreaming) && (stat.btstreamCmd == 1 )){
         stat.isStreaming = 1;
         stat.btstreamCmd = 0;
      }
      if((stat.isStreaming) && (stat.btstreamCmd == 2 )){
         stat.isStreaming = 0;
         stat.btstreamCmd = 0;
      }
   } else{       
      stat.isStreaming = 0;
   }
   return 0;
}

void S4Sens_startSensing(void) {
//   if(stat.isDocked){
//      return;
//   }
   
   stat.isConfiguring = 1;
   if(S4Sens_checkStartSensorConditions()){   
      stat.isSensing = 1;
      sensing.isFileCreated = 0;
      DockUart_disable();
      S4Sens_stepInit();      
      S4Sens_configureChannels();

      uint16_t sampling_rate = S4Ram_getStoredConfig()->samplingRateTicks;
      sensing.freq = 32768.0 / sampling_rate;
      if(sensing.freq>4096){// Please don't go too fast, Thx, Best Regards.
         stat.isConfiguring = 0;
         stat.isSensing = 0;
         return;
      }
      sensing.clkInterval4096 = (uint16_t)4096 / sensing.freq;//216000000 = 8192*26367 or 108000000 = 4096*26367 
      sensing.clkInterval16k = sampling_rate/2;
      
      S4_ADC_startSensing();
      I2C_startSensing();
      SPI_startSensing();

      //I2cSensing(1);// gather the first set of sample?
      //I2cBattMonitor(1);
      
//      TIM_startSensing();
      //sensing.clkInterval4096
//      if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, sensing.clkInterval16k-1, RTC_WAKEUPCLOCK_RTCCLK_DIV2) != HAL_OK)
//      {
//         Error_Handler();
//      }
#if SENS_CLK_RTC0TIM1
      TIM_startSensing();
#else
      S4_RTC_WakeUpSet(sampling_rate);
#endif
      
      sensing.startTs = RTC_get64();
   }
   
   if(S4Sens_checkStartLoggingConditions()){  
      SD_fileInit();
   }
   stat.isConfiguring = 0;
}

uint8_t S4Sens_checkStopSensorConditions(void){
   if(!stat.isLogging && stat.isStreaming){// streaming only case
      if(stat.btstreamCmd != 2){
         return 0;
      }
   }else if (stat.isLogging && !stat.isStreaming){// logging only case
      if(stat.sdlogCmd != 2){
         return 0;
      }
   }else if (stat.isLogging && stat.isStreaming){
      if((stat.btstreamCmd != 2) || (stat.sdlogCmd != 2)){
         return 0;
      }
   }else{
   }
   
   return 1;
}
uint8_t S4Sens_checkStopLoggingConditions(void){
   if(stat.sdlogCmd != 2 ){
      return 0;
   }else{
      stat.isLogging = 0;
      stat.sdlogCmd = 0;
      return 1;   
   }
}
void S4Sens_stopSensing(void) {   
   if(!stat.isSensing){
      return;
   }   
   if(S4Sens_checkStopLoggingConditions()){
      SD_close();
      HAL_Delay(10);
   }   
   if(S4Sens_checkStopSensorConditions()){
      stat.isConfiguring = 1;
      stat.isSensing = 0;
      sensing.startTs = 0;
      //sensing.isSampling = 0;
      S4Sens_stopPeripherals();
      DockUart_enable();
   }
   
   //stat.sdlogCmd = 0;
   stat.btstreamCmd = 0;
   stat.isConfiguring = 0;
}

void S4Sens_stopPeripherals(void) {
   
#if SENS_CLK_RTC0TIM1
   TIM_stopSensing();
#else
   S4_RTC_WakeUpSetSlow();
#endif
   
   S4_ADC_stopSensing();     
   HAL_Delay(10); // Send ACK command needs delay here...
   BtUart_sendRsp();   
#if IS_CONNECTED_DIG_SENSORS
   I2C_stopSensing();
   SPI_stopSensing();
#endif
}

void S4Sens_streamData(void) {
#if SKIP_50MS
   if(sensing.startTs == 0xffffffffffffffff){      
   }else if((sensing.startTs == 0) ||
      (sensing.latestTs - sensing.startTs < 1638)){
      sensing.startTs = 0xffffffffffffffff;
      return;
   }else{
      sensing.startTs = 0xffffffffffffffff;
   }
#endif   

   S4Sens_bufPoll();
   
   //ExpUart_TxIT(sensing.dataBuf, sensing.dataLen);

//   HAL_Delay(500);
#if USE_SD
   if(stat.isLogging){
      PeriStat_Set(STAT_PERI_SDMMC);
      SD_writeToBuff(sensing.dataBuf + 1, sensing.dataLen - 1);
      PeriStat_Clr(STAT_PERI_SDMMC);
   }
#endif 
#if USE_BT
   S4Sens_checkStartStreamingConditions();
   if(stat.isStreaming){
      if(BT_write(sensing.dataBuf, sensing.dataLen) == HAL_OK){
      }
   }
#endif   
   
   if((!stat.isLogging) && (!stat.isStreaming)){
      S4_Task_set(TASK_STOPSENSING);
   }
      
   //sensing.isSampling = 0;
}

void S4Sens_bufPoll() {

   // adc channels
   S4_ADC_bufPoll();
   
   I2C_pollSensors();
   
   SPI_pollSensors();

}

// this is to be called in the ISR
void S4Sens_gatherData(void) {
   if (stat.isSensing) {      
      //sensing.latestTs += ((sensing.latestTs-RTC_get64())/sensing.clkInterval4096)*sensing.clkInterval4096;      
      sensing.latestTs = RTC_get64();
      sensing.dataBuf[sensing.ptr.ts    ] = sensing.latestTs & 0xff;
      sensing.dataBuf[sensing.ptr.ts + 1] = (sensing.latestTs >> 8) & 0xff;
      sensing.dataBuf[sensing.ptr.ts + 2] = (sensing.latestTs >> 16) & 0xff;

      //Task_set(TASK_STREAMDATA);//
      S4Sens_step1Start();
      
      //S4Sens_streamData();

      //I2cSensing(1);
   }
}

void S4Sens_stepInit(void){
   S4_ADC_gatherDataCb(S4Sens_step2Start);
//   I2C_gatherDataInit(S4Sens_step3Start);
//   I2C2_gatherDataInit(S4Sens_step4Start);
   I2cSens_gatherDataCb(S4Sens_step3Start);
#if defined(SHIMMER4_SDK)
   I2cBatt_gatherDataCb(S4Sens_step4Start);
#endif
   SPI_gatherDataCb(S4Sens_step5Start);
   temp_cnt1 = temp_cnt2 = temp_cnt3 = temp_cnt4 = 0;
}

void S4Sens_step1Start(void){  
   PeriStat_Set(STAT_PERI_ADC | STAT_PERI_I2C_SENS | STAT_PERI_I2C_BATT | STAT_PERI_SPI_SENS);
   S4Sens_streamData();
   S4_ADC_gatherDataStart();
   if(temp_cnt2 == 1000){
      __NOP();
      __NOP();
      __NOP();
   }
}
void S4Sens_step2Start(void){  
   PeriStat_Clr(STAT_PERI_ADC);
   temp_cnt2++;
   //I2C_gatherDataStart();
   I2cSens_gatherDataStart();
}
void S4Sens_step3Start(void){  
   PeriStat_Clr(STAT_PERI_I2C_SENS);
   temp_cnt3++;
#if defined(SHIMMER4_SDK)
   I2cBatt_gatherDataStart();
#endif
}
void S4Sens_step4Start(void){  
   PeriStat_Clr(STAT_PERI_I2C_BATT);
   //SPI_gatherDataStart();
   S4Sens_step5Start();
}
void S4Sens_step5Start(void){  
   PeriStat_Clr(STAT_PERI_SPI_SENS);
   temp_cnt4++;
   //S4Sens_streamData();
   S4Sens_stepDone();
}
void S4Sens_stepDone(void)  {  
}
   
//void S4Sens_step1End(void){S4Sens_step2Start();}
//void S4Sens_step2End(void){S4Sens_step3Start();}
//void S4Sens_step3End(void){S4Sens_step4Start();}
//void S4Sens_step4End(void){S4Sens_step5Start();}
//void S4Sens_step5End(void){S4Sens_step6Start();}
//void S4Sens_step6End(void){}
   




