/**
  ******************************************************************************
  * File Name          : ADC.c
  * Description        : This file provides code for the configuration
  *                      of the ADC instances.
  ******************************************************************************
  *
  * Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "s4_adc.h"

//#include "gpio.h"
//#include "dma.h"


//static STATTypeDef * pStat;      
//static SENSINGTypeDef *pSensing;

ADCTypeDef adc;
#if !IS_SHIMMER3R
ADC_HandleTypeDef hadcResv; //reserved for user use
#endif
ADC_HandleTypeDef hadcSens;
ADC_HandleTypeDef hadcBatt;

uint32_t adc_battVal, adcBufSens[12], adcBufResv[12];// max 12 channels, each of 16 bits
//uint32_t adcBuf3[12];
uint8_t gsrActiveResistor;
uint8_t adcConfig;


void S4_NORM_ADC_init(void){   
   
#if IS_SHIMMER3R
  adc.chanCntSens = adc.chanCntBatt = 0;
#else
   adc.chanCntResv = adc.chanCntSens = adc.chanCntBatt = 0;
   memset(&hadcResv, 0, sizeof(ADC_HandleTypeDef));// = 0;//&hadc1;
#endif
   adcConfig = ADC_CONFIG_NONE;
#if IS_SHIMMER3R
   hadcSens.Instance = ADC1;
   hadcBatt.Instance = ADC2;
#else
   hadcResv.Instance = ADC1;
   //hadcSens = &hadc2;
   hadcSens.Instance = ADC2;
   //hadcBatt = &hadc3;
   hadcBatt.Instance = ADC3;
#endif
   
//   pStat = GetStatus();
//   pSensing = S4Sens_getSensing();
   
#if !IS_SHIMMER3R
   S4_ADC_initBatt();
#endif
}

void S4_NORM_ADC_initBatt(void){
   
   // init battery adc
   ADC_ChannelConfTypeDef sConfig;
   /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
   */
   HAL_ADC_DeInit(&hadcBatt);
#if IS_SHIMMER3R
   hadcBatt.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
#else
   hadcBatt.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
#endif
   hadcBatt.Init.Resolution = ADC_RESOLUTION_12B;
   hadcBatt.Init.ScanConvMode = DISABLE;
   hadcBatt.Init.ContinuousConvMode = DISABLE;
   hadcBatt.Init.DiscontinuousConvMode = DISABLE;
   hadcBatt.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
   hadcBatt.Init.DataAlign = ADC_DATAALIGN_RIGHT;
   hadcBatt.Init.NbrOfConversion = 1;
   hadcBatt.Init.DMAContinuousRequests = ENABLE;
   hadcBatt.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
   HAL_ADC_Init(&hadcBatt);
    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
    */
   sConfig.Channel = ADC_CHANNEL_VBATT;
#if IS_SHIMMER3R
   // Copied from MX_ADC1_Init function
   sConfig.Rank = ADC_REGULAR_RANK_1;
   sConfig.SamplingTime = ADC_SAMPLETIME_391CYCLES_5;
   sConfig.SingleDiff = ADC_SINGLE_ENDED;
   sConfig.OffsetNumber = ADC_OFFSET_NONE;
   sConfig.Offset = 0;
#else
   sConfig.Rank = 1;
   sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;
#endif
   HAL_ADC_ConfigChannel(&hadcBatt, &sConfig);
}

void S4_NORM_ADC_configureChannels(void){
   uint8_t *channel_contents_ptr = sensing.cc+sensing.ccLen;
   uint8_t nbr_adc_chans = 0;
   
   adc.sensorLen = 0;//adc.sensorCnt = 0;
#if IS_SHIMMER3R
   adc.chanCntSens = adc.chanCntBatt = 0;
#else
   adc.chanCntResv = adc.chanCntSens = adc.chanCntBatt = 0;
#endif
   
   //shimmer3 adc channel seq:
   // a_accel
   // vbatt
   // ext_a7
   // ext_a6
   // ext_a15
   // int_a12  (ppg)
   // int_a13  (sg_high)
   // int_a14  (sg_low)
   // int_a1   (gsr)
   
   //shimmer4 adc channel seq:
   // a_accel
   // vbatt
   // ext_a9   (same dock connector pin as ext_a7 in Shimmer3)
   // ext_a8   (same dock connector pin as ext_a6 in Shimmer3)
   // ext_a1
   // int_a10  (ppg1)
   // int_a12  (ppg2)
   // int_a11  (sg_high)
   // int_a0   (sg_low)
   // int_a2   (gsr)
   
   //Analog Accel (KXRB5-2042)
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_A_ACCEL) {
      *channel_contents_ptr++ = X_A_ACCEL;
      *channel_contents_ptr++ = Y_A_ACCEL;
      *channel_contents_ptr++ = Z_A_ACCEL;
      nbr_adc_chans += 3;
      sensing.ptr.analogAccel = sensing.dataLen;
      sensing.dataLen += 6;
      adc.sensorList[adc.sensorLen++] = X_A_ACCEL;
      adc.sensorList[adc.sensorLen++] = Y_A_ACCEL;
      adc.sensorList[adc.sensorLen++] = Z_A_ACCEL;
//      adc.sensorCnt++;
//      adc.chanCntSens++;
   }  
   //Analog Battery Voltage
#if USE_VBATT_ALWAYS
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_VBATT) {      
      *channel_contents_ptr++ = VBATT;
      nbr_adc_chans += 1;
      sensing.ptr.batteryAnalog = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = VBATT;
   }else{
      adc.sensorList[adc.sensorLen++] = VBATT;
   }      
#else
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_VBATT) {      
      *channel_contents_ptr++ = VBATT;
      nbr_adc_chans += 1;
      sensing.ptr.batteryAnalog = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = VBATT;
   }     
#endif
   
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_APP_PPG) {
      //in shimmer3 this corresponds to adc12
      S4Ram_storedConfigSetByte(NV_SENSORS1, S4Ram_storedConfigGetByte(NV_SENSORS1) | SENSOR_INT_ADC_10);
      S4Ram_storedConfigSetByte(NV_SENSORS2, S4Ram_storedConfigGetByte(NV_SENSORS2) | SENSOR_INT_ADC_12); 
#if IS_SHIMMER3R
      HAL_GPIO_WritePin(GPIOF, SW_EXP_BRD_Pin, GPIO_PIN_SET);
#else
      HAL_GPIO_WritePin(GPIOF, PPG_EN_Pin, GPIO_PIN_SET);
#endif
   }
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_STRAIN) {
      //in shimmer3 this corresponds to adc13 and adc14
      S4Ram_storedConfigSetByte(NV_SENSORS1, S4Ram_storedConfigGetByte(NV_SENSORS1) | SENSOR_INT_ADC_11); 
      S4Ram_storedConfigSetByte(NV_SENSORS2, S4Ram_storedConfigGetByte(NV_SENSORS2) | SENSOR_INT_ADC_0);         
      HAL_GPIO_WritePin(GPIOB, SW_STRAIN_GAUGE_Pin, GPIO_PIN_SET);
   }
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_GSR) {
      //in shimmer3 this corresponds to adc1
      S4Ram_storedConfigSetByte(NV_SENSORS1, S4Ram_storedConfigGetByte(NV_SENSORS1) | SENSOR_INT_ADC_2);
         
      uint16_t temp_samplingrate;
      S4Ram_storedConfigGet((uint8_t*)&temp_samplingrate, NV_SAMPLING_RATE, 2);
      GSR_init(S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE3), temp_samplingrate, GSR_AUTORANGE);
      if (((S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE3) & 0x0E) >> 1) <= HW_RES_3M3) {
         GSR_setRange((S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE3) & 0x0E) >> 1);
         gsrActiveResistor = (S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE3) & 0x0E) >> 1;
      } else {
         GSR_setRange(HW_RES_40K);
         gsrActiveResistor = HW_RES_40K;
      }  
   }
     
   //External ADC 9
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXT_ADC_9) {
      *channel_contents_ptr++ = EXT_ADC_9;
      nbr_adc_chans += 1;
      sensing.ptr.extADC_9 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = EXT_ADC_9;
   }
   
   //External ADC 8
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXT_ADC_8) {
      *channel_contents_ptr++ = EXT_ADC_8;
      nbr_adc_chans += 1;
      sensing.ptr.extADC_8 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = EXT_ADC_8;
   }
   
   //External ADC 1
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_EXT_ADC_1) {
      *channel_contents_ptr++ = EXT_ADC_1;
      nbr_adc_chans += 1;
      sensing.ptr.extADC_1 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = EXT_ADC_1;
   }   
   
   //Internal ADC 10
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_10) {
      *channel_contents_ptr++ = INT_ADC_10;
      nbr_adc_chans += 1;
      sensing.ptr.intADC_10 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_10;
   }
   
   //Internal ADC 12
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_INT_ADC_12) {
      *channel_contents_ptr++ = INT_ADC_12;
      nbr_adc_chans += 1;
      sensing.ptr.intADC_12 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_12;
   }
   
   //Internal ADC 11
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_11) {
      *channel_contents_ptr++ = INT_ADC_11;
      nbr_adc_chans += 1;
      sensing.ptr.intADC_11 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_11;
   }
   
   //Internal ADC 0
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_INT_ADC_0) {
      *channel_contents_ptr++ = INT_ADC_0;
      nbr_adc_chans += 1;
      sensing.ptr.intADC_0 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_0;
   }
   
   //Internal ADC 2
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_2) {
      *channel_contents_ptr++ = INT_ADC_2;
      nbr_adc_chans += 1;
      sensing.ptr.intADC_2 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_2;
   }   
   sensing.nbrAdcChans += nbr_adc_chans;
   sensing.ccLen += nbr_adc_chans;
}

void S4_NORM_ADC_startSensing(){
   ADC_ChannelConfTypeDef sConfig;
   uint8_t adc_counter_sens = 1;//, adc_counter_resv = 0;   
   adcConfig = ADC_CONFIG_SENS;
#if IS_SHIMMER3R
   sConfig.SamplingTime = ADC_SAMPLETIME_391CYCLES_5;
#else
   sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;
#endif
      
   if(adc.sensorLen > 0){  
      HAL_ADC_DeInit(&hadcSens);
      
      //memcpy((uint8_t*)&hadcSens.Init, (uint8_t*)&hadcBatt.Init, sizeof(ADC_InitTypeDef));    
      //hadcSens.Instance = ADC2;
#if IS_SHIMMER3R
      hadcSens.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
#else
      hadcSens.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
#endif
      hadcSens.Init.Resolution = ADC_RESOLUTION_12B;
      //hadcSens.Init.ScanConvMode = DISABLE;
      hadcSens.Init.ContinuousConvMode = DISABLE;
      hadcSens.Init.DiscontinuousConvMode = DISABLE;
      hadcSens.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
      hadcSens.Init.DataAlign = ADC_DATAALIGN_RIGHT;
      //hadcSens.Init.NbrOfConversion = 3;
      hadcSens.Init.DMAContinuousRequests = ENABLE;
      //hadcSens.Init.EOCSelection = ADC_EOC_SEQ_CONV;
      
      if(adc.sensorLen > 1){
         hadcSens.Init.ScanConvMode = ENABLE;
         hadcSens.Init.EOCSelection = ADC_EOC_SEQ_CONV;
      } else {
         hadcSens.Init.ScanConvMode = DISABLE;
         hadcSens.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
      }
      
      hadcSens.Init.NbrOfConversion = adc.sensorLen;
      
      if (HAL_ADC_Init(&hadcSens) != HAL_OK)
      {
         Error_Handler();
      }
   }
   
#if !IS_SHIMMER3R
   if(adc.chanCntResv > 0){  
      HAL_ADC_DeInit(&hadcResv);
      
      hadcResv.Init = hadcBatt.Init;    
      
      if(adc.chanCntResv > 1){
         hadcResv.Init.ScanConvMode = ENABLE;
         hadcResv.Init.EOCSelection = ADC_EOC_SEQ_CONV;
      } else {
         hadcResv.Init.ScanConvMode = DISABLE;
         hadcResv.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
      }
      
      hadcResv.Init.NbrOfConversion = adc.chanCntResv;
      HAL_ADC_Init(&hadcResv);
   }
#endif
   
   //Analog Accel
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_A_ACCEL) {      
      sConfig.Channel = ADC_CHANNEL_ACCEL_X;//x
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);

      sConfig.Channel = ADC_CHANNEL_ACCEL_Y;//y
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);

      sConfig.Channel = ADC_CHANNEL_ACCEL_Z;//z
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
      
      HAL_GPIO_WritePin(GPIOG, SW_ACCEL_Pin, GPIO_PIN_SET);
   }   
   
#if USE_VBATT_ALWAYS
   if (1) {
#else
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_VBATT) {   
#endif   
      sConfig.Channel = ADC_CHANNEL_VBATT;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }
   
   //External ADC A7 - ADC7_FLASHDAT1 - ADC1_IN9 as per SH_ARM.brd Allegro file
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXT_ADC_9) {
      sConfig.Channel = ADC_CHANNEL_EXT_A7;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }
   
   //External ADC A6 - ADC6_FLASHDAT2 - ADC1_IN8 as per SH_ARM.brd Allegro file
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXT_ADC_8) {
      sConfig.Channel = ADC_CHANNEL_EXT_A6;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }
   
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_EXT_ADC_1) {
      sConfig.Channel = ADC_CHANNEL_EXT_A1;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }

   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_2) {
      sConfig.Channel = ADC_CHANNEL_INT_A2;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }

   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_10) {
      sConfig.Channel = ADC_CHANNEL_INT_A10;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }

   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_11) {
      sConfig.Channel = ADC_CHANNEL_INT_A11;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }

   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_INT_ADC_0) {
      sConfig.Channel = ADC_CHANNEL_INT_A0;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }

#if !IS_SHIMMER3R
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_INT_ADC_12) {
      sConfig.Channel = ADC_CHANNEL_INT_A12;
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);
   }
#endif

}
   
void (*ADC_gatherDataDone_cb)(void);
void S4_NORM_ADC_gatherDataCb(void (*done_cb)(void)){
   ADC_gatherDataDone_cb = done_cb;   
}
   
void S4_NORM_ADC_gatherDataStart(void){   
#if !IS_SHIMMER3R
   if(adc.chanCntResv > 0){
      __NOP();
   }
#endif
   if(adc.sensorLen > 0){
      HAL_ADC_Start_DMA(&hadcSens, adcBufSens, (uint32_t)adc.sensorLen);
      for(uint16_t i = 0; i < 144/2; i++);
      
      ADC_gatherDataDone_cb();
      //HAL_ADC_Start(hadcSens);
   }
}
   
void S4_NORM_ADC_bufPoll(){
   uint8_t adc_offset_sens = 0;//, adc_offset_resv = 0;   
   //uint8_t adc_vbattery[2];
   
//   if(adc.chanCntBatt > 0){
//      ADC_readBatt();
//   }
   
   //Analog Accel
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_A_ACCEL) {      
      // X
      sensing.dataBuf[sensing.ptr.analogAccel + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.analogAccel + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);       
      // Y
      sensing.dataBuf[sensing.ptr.analogAccel + 2] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.analogAccel + 3] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      // Z
      sensing.dataBuf[sensing.ptr.analogAccel + 4] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.analogAccel + 5] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   
   //Analog Battery Voltage
#if USE_VBATT_ALWAYS    
   stat.battVal[0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   stat.battVal[1] = *((uint8_t*)adcBufSens + adc_offset_sens++);  
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_VBATT) { 
      sensing.dataBuf[sensing.ptr.batteryAnalog + 0] = stat.battVal[0];
      sensing.dataBuf[sensing.ptr.batteryAnalog + 1] = stat.battVal[1];
   }  
#else
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_VBATT) { 
      stat.battVal[0] = sensing.dataBuf[sensing.ptr.batteryAnalog + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      stat.battVal[1] = sensing.dataBuf[sensing.ptr.batteryAnalog + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }     
#endif   
   S4_ADC_rankBatt();
   
   //External ADC A7 - ADC7_FLASHDAT1 - ADC1_IN9 as per SH_ARM.brd Allegro file
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXT_ADC_9) {      
      sensing.dataBuf[sensing.ptr.extADC_9 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.extADC_9 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   //External ADC A6 - ADC6_FLASHDAT2 - ADC1_IN8 as per SH_ARM.brd Allegro file
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXT_ADC_8) {      
      sensing.dataBuf[sensing.ptr.extADC_8 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.extADC_8 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_EXT_ADC_1) {      
      sensing.dataBuf[sensing.ptr.extADC_1 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.extADC_1 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }      
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_10) {      
      sensing.dataBuf[sensing.ptr.intADC_10 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC_10 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_INT_ADC_12) {      
      sensing.dataBuf[sensing.ptr.intADC_12 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC_12 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_11) {      
      sensing.dataBuf[sensing.ptr.intADC_11 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC_11 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }   
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_INT_ADC_0) {      
      sensing.dataBuf[sensing.ptr.intADC_0 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC_0 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_INT_ADC_2) {      
      sensing.dataBuf[sensing.ptr.intADC_2 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC_2 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }   
   
//   //PPG
   
//   //Strain Gauge
//   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_STRAIN) {
//      // SG_LOW channel         
//      sensing.dataBuf[sensing.ptr.strainGauge + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
//      sensing.dataBuf[sensing.ptr.strainGauge + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
//      // SG_HIGH channel
//      sensing.dataBuf[sensing.ptr.strainGauge + 2] = *((uint8_t*)adcBufSens + adc_offset_sens++);
//      sensing.dataBuf[sensing.ptr.strainGauge + 3] = *((uint8_t*)adcBufSens + adc_offset_sens++);
//   }
   // GSR
   if(S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_GSR) {
      union {
         uint8_t u8[4];
         uint32_t u32;
      }gsr_buf;
      gsr_buf.u8[0] = sensing.dataBuf[sensing.ptr.intADC_2 + 0];
      gsr_buf.u8[1] = sensing.dataBuf[sensing.ptr.intADC_2 + 1];
      GSR_output(&gsr_buf.u32);
      
      sensing.dataBuf[sensing.ptr.intADC_2 + 0] = gsr_buf.u8[0];
      sensing.dataBuf[sensing.ptr.intADC_2 + 1] = gsr_buf.u8[1];
   }
   
   
}

//void ADC_stopSensing(){      
//   if(adc.sensorLen > 0){
//      HAL_ADC_Stop_DMA(hadcSens);
//      HAL_ADC_DeInit(hadcSens);
//      
//      //Analog Accel (KXRB5-2042)
//      if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_A_ACCEL) {
//         HAL_GPIO_WritePin(GPIOG, SW_ACCEL_Pin, GPIO_PIN_RESET);
//      }         
//      //Analog Strain Gauge
//      if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_STRAIN) {
//         HAL_GPIO_WritePin(GPIOB, GPIO_INTERNAL4_Pin, GPIO_PIN_RESET); // Resets the PV_SG voltage to gauge op-amp
//      }        
//      if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_APP_PPG) {
//         HAL_GPIO_WritePin(GPIOF, PPG_EN_Pin, GPIO_PIN_RESET);
//      }
//   }     
//   if(adc.chanCntResv > 0){
//      HAL_ADC_Stop_DMA(hadcResv);
//      HAL_ADC_DeInit(hadcResv);
//   } 
//   adcConfig = ADC_CONFIG_NONE;
//}


void S4_NORM_ADC_stopSensing(){      
   HAL_ADC_Stop_DMA(&hadcSens);
   HAL_ADC_DeInit(&hadcSens);      
   //Analog Accel (KXRB5-2042)
   HAL_GPIO_WritePin(GPIOG, SW_ACCEL_Pin, GPIO_PIN_RESET);
   //Analog Strain Gauge,  esets the PV_SG voltage to gauge op-amp
   HAL_GPIO_WritePin(GPIOB, SW_STRAIN_GAUGE_Pin, GPIO_PIN_RESET);
   HAL_GPIO_WritePin(GPIOF, SW_PPG_EN_Pin, GPIO_PIN_RESET);
   
#if !IS_SHIMMER3R
   if(adc.chanCntResv > 0){
      HAL_ADC_Stop_DMA(&hadcResv);
      HAL_ADC_DeInit(&hadcResv);
   } 
#endif
   adcConfig = ADC_CONFIG_NONE;
}

#define BATT_LOW_MAX    2618
#define BATT_MID_MIN    2568
#define BATT_MID_MAX    2767
#define BATT_HIGH_MIN   2717
void S4_NORM_ADC_rankBatt(void) {
   if (stat.battStat == BATT_MID) {
      if (*(uint16_t*)stat.battVal < BATT_MID_MIN) {
         stat.battStat = BATT_LOW;
      } else if (*(uint16_t*)stat.battVal < BATT_MID_MAX) {
         stat.battStat = BATT_MID;
      } else{
         stat.battStat = BATT_HIGH;
      }
   } else if (stat.battStat == BATT_LOW) {
      if (*(uint16_t*)stat.battVal < BATT_LOW_MAX) {
         stat.battStat = BATT_LOW;
      } else if (*(uint16_t*)stat.battVal < BATT_MID_MAX) {
         stat.battStat = BATT_MID;
      } else{
         stat.battStat = BATT_HIGH;
      }
   } else { //high
      if (*(uint16_t*)stat.battVal < BATT_MID_MIN) {
         stat.battStat = BATT_LOW;
      } else if (*(uint16_t*)stat.battVal < BATT_HIGH_MIN) {
         stat.battStat = BATT_MID;
      } else{
         stat.battStat = BATT_HIGH;
      }
   }
   switch(stat.battStat){
      case BATT_LOW:    stat.battStatLed = LED_RED;       break;
      case BATT_MID:    stat.battStatLed = LED_YELLOW;    break;
      case BATT_HIGH:   stat.battStatLed = LED_GREEN0;    break;
      default: stat.battStatLed = LED_RED;                break;
   }
}
  
void S4_NORM_ADC_readBatt(void) {
   uint8_t need_to_restore = 0;   
   static uint8_t cnt = 0;
   
   if(!(++cnt%2)){
      return;
   }
   
   if(adcConfig != ADC_CONFIG_BATT){
      ADC_ChannelConfTypeDef sConfig;
      if(adcConfig == ADC_CONFIG_SENS && stat.isSensing){
         need_to_restore = 1;
      }      
      adcConfig = ADC_CONFIG_BATT;
      
      HAL_ADC_DeInit(&hadcSens);   
      hadcSens.Init = hadcBatt.Init;      
      hadcSens.Init.ScanConvMode = DISABLE;
      hadcSens.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
      hadcSens.Init.NbrOfConversion = 1;
      HAL_ADC_Init(&hadcSens);
#if IS_SHIMMER3R
      sConfig.SamplingTime = ADC_SAMPLETIME_391CYCLES_5;
#else
      sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;
#endif
      sConfig.Channel = ADC_CHANNEL_VBATT;
      sConfig.Rank = 1;
      HAL_ADC_ConfigChannel(&hadcSens, &sConfig);      
   }   
   HAL_ADC_Start_DMA(&hadcBatt, &adc_battVal, 1);
   for(uint16_t i = 0; i < 144; i++);
   stat.battVal[0] = adc_battVal & 0xff;
   stat.battVal[1] = (adc_battVal>>8) & 0xff;
   stat.battVal[2] = 0;
   stat.battVal[2] |= HAL_GPIO_ReadPin(CHG_STAT2_GPIO_Port, CHG_STAT2_Pin)<<7;
   stat.battVal[2] |= HAL_GPIO_ReadPin(CHG_STAT1_GPIO_Port, CHG_STAT1_Pin)<<6;
   if(need_to_restore){
      S4_ADC_startSensing();
   }
   //*(uint16_t*)(stat.battVal) = adc_battVal & 0xffff;
   
   S4_ADC_rankBatt();
}



void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
#if !IS_SHIMMER3R
   if (hadc->Instance == hadcResv.Instance) {//adc1
      __NOP();
   }
#endif
   if (hadc->Instance == hadcSens.Instance) {//adc2
      __NOP();
      //ADC_gatherDataDone_cb();
   }
   __NOP();
   __NOP();
   __NOP();
}

//void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
//{
//   if (hadc->Instance == ADC1) {
//      __NOP();
//   }
//}


/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
