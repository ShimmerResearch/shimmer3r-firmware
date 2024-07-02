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
#include "gpdma.h"
//#include "gpio.h"
//#include "dma.h"


//static STATTypeDef * pStat;      
//static SENSINGTypeDef *pSensing;

ADCTypeDef adc;
#if defined(SHIMMER4_SDK)
ADC_HandleTypeDef hadcResv; //reserved for user use
ADC_HandleTypeDef hadcSens;
ADC_HandleTypeDef hadcBatt;
#endif

ADC_HandleTypeDef *hadcSensPtr;
ADC_HandleTypeDef *hadcBattPtr;

#if defined(SHIMMER3R)
uint16_t adc_battVal, adcBufSens[8];// max 8 channels, each of 16 bits
#elif defined(SHIMMER4_SDK)
uint32_t adc_battVal, adcBufSens[12], adcBufResv[12];// max 12 channels, each of 16 bits
#endif
//uint32_t adcBuf3[12];
uint8_t gsrActiveResistor;
uint8_t adcConfig;

#if defined(SHIMMER3R)
static uint16_t ADC_RANK_ARRAY[] = {
    ADC_REGULAR_RANK_1,
    ADC_REGULAR_RANK_2,
    ADC_REGULAR_RANK_3,
    ADC_REGULAR_RANK_4,
    ADC_REGULAR_RANK_5,
    ADC_REGULAR_RANK_6,
    ADC_REGULAR_RANK_7,
    ADC_REGULAR_RANK_8,
    ADC_REGULAR_RANK_9,
    ADC_REGULAR_RANK_10,
    ADC_REGULAR_RANK_11,
    ADC_REGULAR_RANK_12,
    ADC_REGULAR_RANK_13,
    ADC_REGULAR_RANK_14,
    ADC_REGULAR_RANK_15,
    ADC_REGULAR_RANK_16};
#endif


void S4_NORM_ADC_init(void){   
   
#if defined(SHIMMER3R)
  adc.chanCntSens = adc.chanCntBatt = 0;
#elif defined(SHIMMER4_SDK)
   adc.chanCntResv = adc.chanCntSens = adc.chanCntBatt = 0;
   memset(&hadcResv, 0, sizeof(ADC_HandleTypeDef));// = 0;//&hadc1;
#endif
   adcConfig = ADC_CONFIG_NONE;
#if defined(SHIMMER3R)
   hadcSensPtr = getHadc1();
   hadcBattPtr = getHadc2();
#elif defined(SHIMMER4_SDK)
   hadcResv.Instance = ADC1;
   //hadcSens = &hadc2;
   hadcSens.Instance = ADC2;
   //hadcBatt = &hadc3;
   hadcBatt.Instance = ADC3;
   
   hadcSensPtr = &hadcSens;
   hadcBattPtr = &hadcBatt;
 #endif

//   pStat = GetStatus();
//   pSensing = S4Sens_getSensing();
   
#if defined(SHIMMER4_SDK)
   S4_ADC_initBatt();
#endif
}

#if defined(SHIMMER4_SDK)
void S4_NORM_ADC_initBatt(void){
   
   // init battery adc
   ADC_ChannelConfTypeDef sConfig;
   /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
   */
   HAL_ADC_DeInit(hadcBattPtr);
#if defined(SHIMMER3R)
   hadcBattPtr->Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
#elif defined(SHIMMER4_SDK)
   hadcBattPtr->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
#endif
   hadcBattPtr->Init.Resolution = ADC_RESOLUTION_12B;
   hadcBattPtr->Init.ScanConvMode = DISABLE;
   hadcBattPtr->Init.ContinuousConvMode = DISABLE;
   hadcBattPtr->Init.DiscontinuousConvMode = DISABLE;
   hadcBattPtr->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
   hadcBattPtr->Init.DataAlign = ADC_DATAALIGN_RIGHT;
   hadcBattPtr->Init.NbrOfConversion = 1;
   hadcBattPtr->Init.DMAContinuousRequests = ENABLE;
   hadcBattPtr->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
   HAL_ADC_Init(hadcBattPtr);
    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
    */
   sConfig.Channel = ADC_CHANNEL_VBATT;
#if defined(SHIMMER3R)
   // Copied from MX_ADC1_Init function
   sConfig.Rank = ADC_REGULAR_RANK_1;
   sConfig.SamplingTime = ADC_SAMPLETIME_391CYCLES;
   sConfig.SingleDiff = ADC_SINGLE_ENDED;
   sConfig.OffsetNumber = ADC_OFFSET_NONE;
   sConfig.Offset = 0;
#else
   sConfig.Rank = 1;
   sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;
#endif
   HAL_ADC_ConfigChannel(hadcBattPtr, &sConfig);
}
#endif

void S4_NORM_ADC_configureChannels(void){
   uint8_t *channel_contents_ptr = sensing.cc+sensing.ccLen;
   uint8_t nbr_adc_chans = 0;
   gConfigBytes *configBytes = S4Ram_getStoredConfig();
   
   adc.sensorLen = 0;//adc.sensorCnt = 0;
#if defined(SHIMMER3R)
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
   
#if defined(SHIMMER4_SDK)
   //Analog Accel (KXRB5-2042)
   if (configBytes->chEnLnAccel) {
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
#endif
   //Analog Battery Voltage
#if USE_VBATT_ALWAYS
  if (configBytes->chEnVBattery)
  {
      *channel_contents_ptr++ = VBATT;
      nbr_adc_chans += 1;
      sensing.ptr.batteryAnalog = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = VBATT;
   }else{
      adc.sensorList[adc.sensorLen++] = VBATT;
   }      
#else
   if (configBytes->chEnVBattery) {
      *channel_contents_ptr++ = VBATT;
      nbr_adc_chans += 1;
      sensing.ptr.batteryAnalog = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = VBATT;
   }     
#endif

   // TODO check what's needed below for Shimmer3r
#if defined(SHIMMER4_SDK)
   if (configBytes->chEnPpg) {
      //in shimmer3 this corresponds to adc12
      configBytes->chEnIntADC0 = 1;
      configBytes->chEnIntADC4 = 1;
      HAL_GPIO_WritePin(SW_PPG_EN_GPIO_Port, SW_PPG_EN_Pin, GPIO_PIN_SET);
   }
   if (configBytes->chEnBridgeAmp) {
      //in shimmer3 this corresponds to adc13 and adc14
      configBytes->chEnIntADC1 = 1;
      configBytes->chEnIntADC2 = 1;
      HAL_GPIO_WritePin(SW_STRAIN_GAUGE_GPIO_Port, SW_STRAIN_GAUGE_Pin, GPIO_PIN_SET);
   }
#endif

   if (configBytes->chEnGsr) {
     // TODO check what's needed below for Shimmer3r
#if defined(SHIMMER4_SDK)
     //in shimmer3 this corresponds to adc1
      configBytes->chEnIntADC3 = 1;
#endif
         
      GSR_init(configBytes->gsrRange, configBytes->samplingRateTicks, GSR_AUTORANGE);
      if (configBytes->gsrRange <= HW_RES_3M3) {
         GSR_setRange(configBytes->gsrRange);
         gsrActiveResistor = configBytes->gsrRange;
      } else {
         GSR_setRange(HW_RES_40K);
         gsrActiveResistor = HW_RES_40K;
      }
   }

   //External ADC 0
   if (configBytes->chEnExtADC0) {
      *channel_contents_ptr++ = EXT_ADC_0;
      nbr_adc_chans += 1;
      sensing.ptr.extADC0 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = EXT_ADC_0;
   }
   
   //External ADC 1
   if (configBytes->chEnExtADC1) {
      *channel_contents_ptr++ = EXT_ADC_1;
      nbr_adc_chans += 1;
      sensing.ptr.extADC1 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = EXT_ADC_1;
   }
   
   //External ADC 2
   if (configBytes->chEnExtADC2) {
      *channel_contents_ptr++ = EXT_ADC_2;
      nbr_adc_chans += 1;
      sensing.ptr.extADC2 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = EXT_ADC_2;
   }   
   
   //Internal ADC 0
   if (configBytes->chEnIntADC0) {
      *channel_contents_ptr++ = INT_ADC_0;
      nbr_adc_chans += 1;
      sensing.ptr.intADC0 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_0;
   }
   
#if defined(SHIMMER4_SDK)
   //Internal ADC 4
   if (configBytes->chEnIntADC4) {
      *channel_contents_ptr++ = INT_ADC_4;
      nbr_adc_chans += 1;
      sensing.ptr.intADC4 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_4;
   }
#endif
   
   //Internal ADC 1
   if (configBytes->chEnIntADC1) {
      *channel_contents_ptr++ = INT_ADC_1;
      nbr_adc_chans += 1;
      sensing.ptr.intADC1 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_1;
   }
   
   //Internal ADC 2
   if (configBytes->chEnIntADC2) {
      *channel_contents_ptr++ = INT_ADC_2;
      nbr_adc_chans += 1;
      sensing.ptr.intADC2 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_2;
   }
   
   //Internal ADC 3
   if (configBytes->chEnIntADC3) {
      *channel_contents_ptr++ = INT_ADC_3;
      nbr_adc_chans += 1;
      sensing.ptr.intADC3 = sensing.dataLen;
      sensing.dataLen += 2;
      adc.sensorList[adc.sensorLen++] = INT_ADC_3;
   }   

   sensing.nbrAdcChans += nbr_adc_chans;
   sensing.ccLen += nbr_adc_chans;
}

void S4_NORM_ADC_startSensing(){
  gConfigBytes *configBytes = S4Ram_getStoredConfig();
   ADC_ChannelConfTypeDef sConfig = {0};
#if defined(SHIMMER3R)
   uint8_t adc_counter_sens = 0; //adc channel rank counter
#elif defined(SHIMMER4_SDK)
   uint8_t adc_counter_sens = 1; //adc channel rank counter
#endif
   adcConfig = ADC_CONFIG_SENS;
   uint32_t adcGpioPinA = 0;
   uint32_t adcGpioPinB = 0;
      
   if(adc.sensorLen > 0){
      //memcpy((uint8_t*)hadcSensPtr.Init, (uint8_t*)&hadcBattPtr->Init, sizeof(ADC_InitTypeDef));
      //hadcSens.Instance = ADC2;
#if defined(SHIMMER3R)
     hadcSensPtr->Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
     hadcSensPtr->Init.Resolution = ADC_RESOLUTION_14B;
     hadcSensPtr->Init.GainCompensation = 0;
     hadcSensPtr->Init.DataAlign = ADC_DATAALIGN_RIGHT;
     hadcSensPtr->Init.ScanConvMode = ADC_SCAN_ENABLE;
     hadcSensPtr->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
     hadcSensPtr->Init.LowPowerAutoWait = DISABLE;
     hadcSensPtr->Init.ContinuousConvMode = ENABLE;
     hadcSensPtr->Init.NbrOfConversion = adc.sensorLen;
     hadcSensPtr->Init.DiscontinuousConvMode = DISABLE;
     hadcSensPtr->Init.ExternalTrigConv = ADC_SOFTWARE_START;
     hadcSensPtr->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
     hadcSensPtr->Init.DMAContinuousRequests = DISABLE;
     hadcSensPtr->Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_LOW;
     hadcSensPtr->Init.Overrun = ADC_OVR_DATA_PRESERVED;
     hadcSensPtr->Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
     hadcSensPtr->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_ONESHOT;
     hadcSensPtr->Init.OversamplingMode = DISABLE;


#elif defined(SHIMMER4_SDK)
      hadcSensPtr->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
      hadcSensPtr->Init.Resolution = ADC_RESOLUTION_12B;
      hadcSensPtr->Init.ScanConvMode = DISABLE;
      hadcSensPtr->Init.ContinuousConvMode = DISABLE;
      hadcSensPtr->Init.DiscontinuousConvMode = DISABLE;
      hadcSensPtr->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
      hadcSensPtr->Init.DataAlign = ADC_DATAALIGN_RIGHT;
      hadcSensPtr->Init.NbrOfConversion = adc.sensorLen;
      hadcSensPtr->Init.DMAContinuousRequests = ENABLE;
      //hadcSensPtr->Init.EOCSelection = ADC_EOC_SEQ_CONV;
#endif

      // Override EOCSelection depending on number of enabled channels
      if(adc.sensorLen > 1)
      {
         hadcSensPtr->Init.EOCSelection = ADC_EOC_SEQ_CONV;
      }
      else
      {
         hadcSensPtr->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
      }
      if (HAL_ADC_Init(hadcSensPtr) != HAL_OK)
      {
         Error_Handler();
      }
#if defined(SHIMMER3R)
      linkedListConfig(hadcSensPtr);

      sConfig.SamplingTime = ADC_SAMPLETIME_391CYCLES;
      sConfig.SingleDiff = ADC_SINGLE_ENDED;
      sConfig.OffsetNumber = ADC_OFFSET_NONE;
      sConfig.Offset = 0;
#elif defined(SHIMMER4_SDK)
      sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;
#endif
   }
   
#if defined(SHIMMER4_SDK)
   if(adc.chanCntResv > 0){  
      HAL_ADC_DeInit(&hadcResv);
      
      hadcResv.Init = hadcBattPtr->Init;
      
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
   
   //Analog Accel
   if (configBytes->chEnLnAccel) {
      sConfig.Channel = ADC_CHANNEL_ACCEL_X;//x
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig);

      sConfig.Channel = ADC_CHANNEL_ACCEL_Y;//y
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig);

      sConfig.Channel = ADC_CHANNEL_ACCEL_Z;//z
      sConfig.Rank = adc_counter_sens++;
      HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig);
      
      HAL_GPIO_WritePin(GPIOG, SW_ACCEL_Pin, GPIO_PIN_SET);
   }   
#endif
   
#if USE_VBATT_ALWAYS
   if (1)
#else
   if (configBytes->chEnVBattery)
#endif
   {
      sConfig.Channel = ADC_CHANNEL_VBATT;
      sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
      adcGpioInit(VBAT_SENSE_Pin, VBAT_SENSE_GPIO_Port);
   }
   
   //External ADC A7 - ADC7_FLASHDAT1 - ADC1_IN9 as per SH_ARM.brd Allegro file
   if (configBytes->chEnExtADC0) {
      sConfig.Channel = ADC_CHANNEL_EXT_A0;
      sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
      adcGpioPinA |= GPIO_ADC_EXT_EXP0_Pin;
   }
   
   //External ADC A6 - ADC6_FLASHDAT2 - ADC1_IN8 as per SH_ARM.brd Allegro file
   if (configBytes->chEnExtADC1) {
      sConfig.Channel = ADC_CHANNEL_EXT_A1;
      sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
      adcGpioPinA |= GPIO_ADC_EXT_EXP1_Pin;
   }
   
   if (configBytes->chEnExtADC2) {
      sConfig.Channel = ADC_CHANNEL_EXT_A2;
      sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
      adcGpioPinA |= GPIO_ADC_EXT_EXP2_Pin;
   }

   if (configBytes->chEnIntADC3) {
      sConfig.Channel = ADC_CHANNEL_INT_A3;
      sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
      adcGpioPinB |= GPIO_ADC_INT_EXP3_Pin;
   }

   if (configBytes->chEnIntADC0) {
      sConfig.Channel = ADC_CHANNEL_INT_A0;
      sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
      adcGpioPinA |= GPIO_ADC_INT_EXP0_Pin;
   }

   if (configBytes->chEnIntADC1) {
      sConfig.Channel = ADC_CHANNEL_INT_A1;
      sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
      adcGpioPinB |= GPIO_ADC_INT_EXP1_Pin;
   }

   if (configBytes->chEnIntADC2) {
      sConfig.Channel = ADC_CHANNEL_INT_A2;
      sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
      adcGpioPinB |= GPIO_ADC_INT_EXP2_Pin;
   }
   /*GPIO init as per configuration*/
   if(adcGpioPinA != 0)
   {
     adcGpioInit(adcGpioPinA, GPIOA);
   }
   if(adcGpioPinB != 0)
   {
     adcGpioInit(adcGpioPinB, GPIOB);
   }
   HAL_ADCEx_Calibration_Start(hadcSensPtr,ADC_CALIB_OFFSET,ADC_SINGLE_ENDED);//can be removed later

#if defined(SHIMMER4_SDK)
   if (configBytes->chEnIntADC4) {
     sConfig.Rank = ADC_RANK_ARRAY[adc_counter_sens++];
      adc_counter_sens += ADC_RANK_OFFSET;
      if (HAL_ADC_ConfigChannel(hadcSensPtr, &sConfig) != HAL_OK)
      {
        Error_Handler();
      }
   }
#endif

}
   
void (*ADC_gatherDataDone_cb)(void);
void S4_NORM_ADC_gatherDataCb(void (*done_cb)(void)){
   ADC_gatherDataDone_cb = done_cb;   
}
   
void S4_NORM_ADC_gatherDataStart(void){   
#if defined(SHIMMER4_SDK)
   if(adc.chanCntResv > 0){
      __NOP();
   }
#endif
   if(adc.sensorLen > 0){
      HAL_ADC_Start_DMA(hadcSensPtr, (uint32_t *)adcBufSens, (uint32_t)adc.sensorLen);
   }
}
   
void S4_NORM_ADC_bufPoll(){
   uint8_t adc_offset_sens = 0;//, adc_offset_resv = 0;   
   //uint8_t adc_vbattery[2];
   gConfigBytes *configBytes = S4Ram_getStoredConfig();

//   if(adc.chanCntBatt > 0){
//      ADC_readBatt();
//   }
   
#if defined(SHIMMER4_SDK)
   //Analog Accel
   if (configBytes->chEnLnAccel) {
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
#endif
   
   //Analog Battery Voltage
#if USE_VBATT_ALWAYS    
   stat.battVal[0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   stat.battVal[1] = *((uint8_t*)adcBufSens + adc_offset_sens++);  
   if (configBytes->chEnVBattery) {
      sensing.dataBuf[sensing.ptr.batteryAnalog + 0] = stat.battVal[0];
      sensing.dataBuf[sensing.ptr.batteryAnalog + 1] = stat.battVal[1];
   }  
#else
   if (configBytes->chEnVBattery) {
       sensing.dataBuf[sensing.ptr.batteryAnalog + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
       sensing.dataBuf[sensing.ptr.batteryAnalog + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }     
#endif
   
   //External ADC A7 - ADC7_FLASHDAT1 - ADC1_IN9 as per SH_ARM.brd Allegro file
   if (configBytes->chEnExtADC0) {
      sensing.dataBuf[sensing.ptr.extADC0 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.extADC0 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   //External ADC A6 - ADC6_FLASHDAT2 - ADC1_IN8 as per SH_ARM.brd Allegro file
   if (configBytes->chEnExtADC1) {
      sensing.dataBuf[sensing.ptr.extADC1 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.extADC1 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   if (configBytes->chEnExtADC2) {
      sensing.dataBuf[sensing.ptr.extADC2 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.extADC2 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }      
   if (configBytes->chEnIntADC0) {
      sensing.dataBuf[sensing.ptr.intADC0 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC0 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
#if defined(SHIMMER4_SDK)
   if (configBytes->chEnIntADC4) {
      sensing.dataBuf[sensing.ptr.intADC4 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC4 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
#endif
   if (configBytes->chEnIntADC1) {
      sensing.dataBuf[sensing.ptr.intADC1 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC1 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }   
   if (configBytes->chEnIntADC2) {
      sensing.dataBuf[sensing.ptr.intADC2 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC2 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }
   if (configBytes->chEnIntADC3) {
      sensing.dataBuf[sensing.ptr.intADC3 + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
      sensing.dataBuf[sensing.ptr.intADC3 + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
   }   
   
//   //PPG
   
//   //Strain Gauge
//   if (configBytes->chEnBrAmp) {
//      // SG_LOW channel         
//      sensing.dataBuf[sensing.ptr.strainGauge + 0] = *((uint8_t*)adcBufSens + adc_offset_sens++);
//      sensing.dataBuf[sensing.ptr.strainGauge + 1] = *((uint8_t*)adcBufSens + adc_offset_sens++);
//      // SG_HIGH channel
//      sensing.dataBuf[sensing.ptr.strainGauge + 2] = *((uint8_t*)adcBufSens + adc_offset_sens++);
//      sensing.dataBuf[sensing.ptr.strainGauge + 3] = *((uint8_t*)adcBufSens + adc_offset_sens++);
//   }
   // GSR
   if(configBytes->chEnGsr) {
      union {
         uint8_t u8[4];
         uint32_t u32;
      }gsr_buf;
      gsr_buf.u8[0] = sensing.dataBuf[sensing.ptr.intADC3 + 0];
      gsr_buf.u8[1] = sensing.dataBuf[sensing.ptr.intADC3 + 1];
      GSR_output(&gsr_buf.u32);
      
      sensing.dataBuf[sensing.ptr.intADC3 + 0] = gsr_buf.u8[0];
      sensing.dataBuf[sensing.ptr.intADC3 + 1] = gsr_buf.u8[1];
   }
   
   
}

//void ADC_stopSensing()
//{
//  gConfigBytes *configBytes = S4Ram_getStoredConfig();
//  if (adc.sensorLen > 0)
//  {
//    HAL_ADC_Stop_DMA(hadcSens);
//    HAL_ADC_DeInit(hadcSens);
//
//    //Analog Accel (KXRB5-2042)
//    if (configBytes->chEnLnAccel)
//    {
//      HAL_GPIO_WritePin(GPIOG, SW_ACCEL_Pin, GPIO_PIN_RESET);
//    }
//    //Analog Strain Gauge
//    if (configBytes->chEnBridgeAmp)
//    {
//      // Resets the PV_SG voltage to gauge op-amp
//      HAL_GPIO_WritePin(SW_STRAIN_GAUGE_GPIO_Port, SW_STRAIN_GAUGE_Pin,
//          GPIO_PIN_RESET);
//    }
//    if (configBytes->chEnPpgApp)
//    {
//      HAL_GPIO_WritePin(SW_PPG_EN_GPIO_Port, PPG_EN_Pin, GPIO_PIN_RESET);
//    }
//  }
//  if (adc.chanCntResv > 0)
//  {
//    HAL_ADC_Stop_DMA(hadcResv);
//    HAL_ADC_DeInit(hadcResv);
//  }
//  adcConfig = ADC_CONFIG_NONE;
//}

void S4_NORM_ADC_stopSensing(){      
   HAL_ADC_Stop_DMA(hadcSensPtr);
   HAL_ADC_DeInit(hadcSensPtr);
#if defined(SHIMMER4_SDK)
   //Analog Accel (KXRB5-2042)
   HAL_GPIO_WritePin(GPIOG, SW_ACCEL_Pin, GPIO_PIN_RESET);
#endif
   //Analog Strain Gauge,  esets the PV_SG voltage to gauge op-amp
   HAL_GPIO_WritePin(GPIOB, SW_STRAIN_GAUGE_Pin, GPIO_PIN_RESET);
   HAL_GPIO_WritePin(GPIOF, SW_PPG_EN_Pin, GPIO_PIN_RESET);
   
#if defined(SHIMMER4_SDK)
   if(adc.chanCntResv > 0){
      HAL_ADC_Stop_DMA(&hadcResv);
      HAL_ADC_DeInit(&hadcResv);
   } 
#endif
   adcConfig = ADC_CONFIG_NONE;
}

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
#if defined(SHIMMER3R)
   case BATT_LOW:    stat.battStatLed = LED_RGB_RED;       break;
   case BATT_MID:    stat.battStatLed = LED_RGB_YELLOW;    break;
   case BATT_HIGH:   stat.battStatLed = LED_RGB_GREEN;    break;
   default: stat.battStatLed = LED_RED;                break;
#elif defined(SHIMMER4_SDK)
      case BATT_LOW:    stat.battStatLed = LED_RED;       break;
      case BATT_MID:    stat.battStatLed = LED_YELLOW;    break;
      case BATT_HIGH:   stat.battStatLed = LED_GREEN;    break;
      default: stat.battStatLed = LED_RED_LWR;                break;
#endif
   }
}
  
void S4_NORM_ADC_readBatt(void) {
#if defined(SHIMMER4_SDK)
   uint8_t need_to_restore = 0;   
#endif

#if defined(SHIMMER4_SDK)
     if(adcConfig != ADC_CONFIG_BATT){
#endif
      ADC_ChannelConfTypeDef sConfig;
#if defined(SHIMMER4_SDK)
      if(adcConfig == ADC_CONFIG_SENS && stat.isSensing){
         need_to_restore = 1;
      }
      adcConfig = ADC_CONFIG_BATT;
#endif

      HAL_ADC_DeInit(hadcBattPtr);
      hadcBattPtr->Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
      hadcBattPtr->Init.Resolution = ADC_RESOLUTION_14B;
      hadcBattPtr->Init.GainCompensation = 0;
      hadcBattPtr->Init.DataAlign = ADC_DATAALIGN_RIGHT;
      hadcBattPtr->Init.ScanConvMode = ADC_SCAN_ENABLE;
      hadcBattPtr->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
      hadcBattPtr->Init.LowPowerAutoWait = DISABLE;
      hadcBattPtr->Init.ContinuousConvMode = DISABLE;
      hadcBattPtr->Init.NbrOfConversion = 1;
      hadcBattPtr->Init.DiscontinuousConvMode = DISABLE;
      hadcBattPtr->Init.ExternalTrigConv = ADC_SOFTWARE_START;
      hadcBattPtr->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
      hadcBattPtr->Init.DMAContinuousRequests = DISABLE;
      hadcBattPtr->Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
      hadcBattPtr->Init.Overrun = ADC_OVR_DATA_PRESERVED;
      hadcBattPtr->Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
      hadcBattPtr->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
      hadcBattPtr->Init.OversamplingMode = DISABLE;
      HAL_ADC_Init(hadcBattPtr);
#if defined(SHIMMER3R)
      // Copied from MX_ADC1_Init function
      sConfig.Channel = ADC_CHANNEL_VBATT;
      sConfig.Rank = ADC_REGULAR_RANK_1;
      sConfig.SamplingTime = ADC_SAMPLETIME_391CYCLES;
      sConfig.SingleDiff = ADC_SINGLE_ENDED;
      sConfig.OffsetNumber = ADC_OFFSET_NONE;
      sConfig.Offset = 0;
#elif defined(SHIMMER4_SDK)
      sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;
#endif
      HAL_ADC_ConfigChannel(hadcBattPtr, &sConfig);
#if defined(SHIMMER4_SDK)
   }
#endif
#if defined(SHIMMER3R)
   //TODO hadcSensPtr is configured above from Shimmer4 code but we're using hadcBattPtr below which I can't make sense of.
   //TODO A bit of cross-over here between the two ADCs (presumably to handle whether the device is streaming or not) but I think this can probably be cleaned up a lot.
   //TODO Shimmer4 implementation called DMA operation here and then delayed for a number of samples, I think calling HAL_ADC_Start may be better here but needs testing
   //HAL_ADC_Start(hadcBattPtr);
   //adc_battVal = HAL_ADC_GetValue(hadcBattPtr);
   HAL_ADC_Start_IT(hadcBattPtr);
#elif defined(SHIMMER4_SDK)
   HAL_ADC_Start_DMA(hadcBattPtr, &adc_battVal, 1);
   for(uint16_t i = 0; i < 144; i++);
   stat.battVal[0] = adc_battVal & 0xff;
   stat.battVal[1] = (adc_battVal>>8) & 0xff;
   stat.battVal[2] = 0;
   stat.battVal[2] |= HAL_GPIO_ReadPin(CHG_STAT2_GPIO_Port, CHG_STAT2_Pin)<<7;
   stat.battVal[2] |= HAL_GPIO_ReadPin(CHG_STAT1_GPIO_Port, CHG_STAT1_Pin)<<6;
   if(need_to_restore)
   {
      S4_ADC_startSensing();
   }
   //*(uint16_t*)(stat.battVal) = adc_battVal & 0xffff;
   S4_ADC_rankBatt();
#endif
}

#if defined(SHIMMER3R)
bool areAdcChannelsEnabled(void)
{
  return adc.sensorLen > 0 ? true : false;
}
#endif

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
#if defined(SHIMMER3R)
  if (hadc->Instance == hadcSensPtr->Instance)
  {
    S4_NORM_ADC_bufPoll();
    ADC_gatherDataDone_cb();
    HAL_ADC_Stop_DMA(hadcSensPtr);
  }
  if (hadc->Instance == hadcBattPtr->Instance)
  {
    adc_battVal = HAL_ADC_GetValue(hadcBattPtr);
    updateBatteryStatus(adc_battVal);
  }
#elif defined(SHIMMER4_SDK)
   if (hadc->Instance == hadcResv.Instance) {//adc1
      __NOP();
   }
   if (hadc->Instance == hadcSensPtr->Instance) {//adc2
      __NOP();
      //ADC_gatherDataDone_cb();
   }
   __NOP();
   __NOP();
   __NOP();
#endif
}

void adcGpioInit(uint32_t pin, GPIO_TypeDef* port)
{
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  if (port == VBAT_SENSE_GPIO_Port)
  {
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(VBAT_SENSE_GPIO_Port, &GPIO_InitStruct);
  }
  else if (port == GPIOA)
  {
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (port == GPIOB)
  {
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void manageReadBatt(void)
{
  if(!hadcBattPtr)
  {
    return;
  }

  gConfigBytes *configBytes = S4Ram_getStoredConfig();
  if(stat.isSensing && configBytes->chEnVBattery) //if sensing and if vbat enabled use previous reading
  {
    updateBatteryStatus(*(uint16_t*) &sensing.dataBuf[sensing.ptr.batteryAnalog]);
  }
  else
  {
    S4_ADC_readBatt();
  }
  setupNextRtcMinuteAlarm();
}

void updateBatteryStatus(uint16_t adc_battVal)
{
  stat.battVal[0] = adc_battVal & 0xff;
  stat.battVal[1] = (adc_battVal >> 8) & 0xff;
  stat.battVal[2] = 0;
  stat.battVal[2] |= HAL_GPIO_ReadPin(CHG_STAT2_GPIO_Port, CHG_STAT2_Pin) << 7;
  stat.battVal[2] |= HAL_GPIO_ReadPin(CHG_STAT1_GPIO_Port, CHG_STAT1_Pin) << 6;
  S4_ADC_rankBatt();
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
