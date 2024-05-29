/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */

//static STATTypeDef *pStat;
//static SENSINGTypeDef *pSensing;

// DOCK_UART variables

UART_HandleTypeDef *huartBt;
UART_HandleTypeDef *huartDock;
#if defined(SHIMMER4_SDK)
UART_HandleTypeDef *huartExp;
#endif

#if defined(SHIMMER4_SDK)
// BT UART variables
uint8_t btArgs[MAX_COMMAND_ARG_SIZE], btWaitingForArgs, btWaitingForArgsLength, btArgsSize, btAction;
// BT command vars
uint8_t inquiryBtRsp, samplingRateBtRsp, toggleLedRed, //enableBtstream, enableSdlog,
        lsm303dlhcAccelRangeResponse, lsm303dlhcMagGainResponse, lsm303dlhcMagSamplingRateResponse, dockStatusBtRsp,
        vbattBtRsp, trialConfigResponse, centerResponse, shimmerNameResponse, expIDResponse, configTimeResponse,
        dirResponse, nshimmerResponse, myIDResponse, lsm303dlhcAccelSamplingRateResponse, i2cvBattBtRsp,
        lsm303dlhcAccelHRModeResponse, mpu9250GyroRangeResponse, bmp180CalibCoeffBtRsp, mpu9250SamplingRateResponse,
        mpu9250AccelRangeResponse, bmp180OversamplingRatioResponse, internalExpPowerEnableResponse,
        exgRegsResponse, configSetupBytesResponse, fwVersionBtRsp, blinkLedBtRsp, infomemBtRsp, dcIdBtRsp, dcMemBtRsp,
        mpu9250MagSensAdjValsResponse, lsm303dlhcAccelLPModeResponse, deviceVersionBtRsp, rwcResponse,
        calibRamResponse;//btIsConnected,
uint8_t btInfomemLength, btDcMemLength, btCalibRamLength;
uint16_t btInfomemOffset, btDcMemOffset, btCalibRamOffset;
#endif

uint8_t  uartSteps, uartArgSize, uartArg2Wait, uartCrc2Wait, uartAction;//uartProcessCmds, uartSendResponses,
uint8_t uartDockRxBuf[2], dockRxBuf[UART_DATA_LEN_MAX], uartRespBuf[UART_RSP_PACKET_SIZE];
uint8_t  uartSendRspMac, uartSendRspVer, uartSendRspBat, uartSendRspRtcConfigTime, uartSendRspCurrentTime,
         uartSendRspGdi, uartSendRspGdm, uartSendRspGim, uartSendRspCalibDump, uartSendRspBtVer,
         uartSendRspAck, uartSendRspBadCmd, uartSendRspBadArg, uartSendRspBadCrc;
uint8_t uartDcMemLength, uartInfoMemLength, uartCalibRamLength;
uint16_t uartDcMemOffset, uartInfoMemOffset, uartCalibRamOffset;
uint64_t uartTimeStart, uartTimeEnd;

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef handle_GPDMA1_Channel1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 1000000;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_EnableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = DOCK_TXD_Pin|DOCK_RXD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    PD11     ------> USART3_CTS
    PD12     ------> USART3_RTS
    */
    GPIO_InitStruct.Pin = BT_TXD_Pin|BT_RXD_Pin|BT_CTS_Pin|BT_RTS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USART3 DMA Init */
    /* GPDMA1_REQUEST_USART3_TX Init */
    handle_GPDMA1_Channel1.Instance = GPDMA1_Channel1;
    handle_GPDMA1_Channel1.Init.Request = GPDMA1_REQUEST_USART3_TX;
    handle_GPDMA1_Channel1.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel1.Init.Direction = DMA_MEMORY_TO_PERIPH;
    handle_GPDMA1_Channel1.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel1.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel1.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel1.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel1.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel1.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel1.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmatx, handle_GPDMA1_Channel1);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel1, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* GPDMA1_REQUEST_USART3_RX Init */
    handle_GPDMA1_Channel0.Instance = GPDMA1_Channel0;
    handle_GPDMA1_Channel0.Init.Request = GPDMA1_REQUEST_USART3_RX;
    handle_GPDMA1_Channel0.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel0.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel0.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel0.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel0.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel0.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel0.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel0.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel0.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel0.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel0.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel0.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmarx, handle_GPDMA1_Channel0);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOB, DOCK_TXD_Pin|DOCK_RXD_Pin);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    PD11     ------> USART3_CTS
    PD12     ------> USART3_RTS
    */
    HAL_GPIO_DeInit(GPIOD, BT_TXD_Pin|BT_RXD_Pin|BT_CTS_Pin|BT_RTS_Pin);

    /* USART3 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */


void Uart_init(void){
   huartBt = &huart3;
#if defined(SHIMMER3R)
   huartDock = &huart1;
#elif defined(SHIMMER4_SDK)
   huartDock = &huart6;
#endif
   //huartExp = &huart6;

//   pStat = GetStatus();
//   pSensing = S4Sens_getSensing();
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
void usartBtUpdate(uint32_t baudRate, uint32_t hwFlowCtrl)
{
  HAL_StatusTypeDef status = HAL_UART_Abort(huartBt);
  status = HAL_UART_DeInit(huartBt);

  huartBt->Instance = USART3;
  huartBt->Init.BaudRate = baudRate;
  huartBt->Init.WordLength = UART_WORDLENGTH_8B;
  huartBt->Init.StopBits = UART_STOPBITS_1;
  huartBt->Init.Parity = UART_PARITY_NONE;
  huartBt->Init.Mode = UART_MODE_TX_RX;
  huartBt->Init.HwFlowCtl = hwFlowCtrl? UART_HWCONTROL_RTS_CTS:UART_HWCONTROL_NONE;
  huartBt->Init.OverSampling = UART_OVERSAMPLING_16;
  huartBt->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huartBt->Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huartBt->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(huartBt) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(huartBt, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(huartBt, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_EnableFifoMode(huartBt) != HAL_OK)
  {
    Error_Handler();
  }

  setBtUartInstance(huartBt);
}

#if defined(SHIMMER4_SDK)
/*****************************************************
 *
 *  bt uart
 *
 *****************************************************/

void BtUart_init(void){
   Uart_init();

   btWaitingForArgs = 0;
   btWaitingForArgsLength = 0;
   btArgsSize = 0;
   btAction = 0;
   // BT commands
   deviceVersionBtRsp = 0;
   rwcResponse = 0;
   dcIdBtRsp = 0;
   dcMemBtRsp = 0;
   infomemBtRsp = 0;

   //btIsConnected = 0;
   i2cvBattBtRsp = 0;
   inquiryBtRsp = 0;
   samplingRateBtRsp = 0;
   toggleLedRed = 0;
   lsm303dlhcAccelRangeResponse = 0;
   lsm303dlhcMagGainResponse = 0;
   lsm303dlhcMagSamplingRateResponse = 0;
   dockStatusBtRsp = 0;
   vbattBtRsp = 0;
   trialConfigResponse = 0;
   centerResponse = 0;
   shimmerNameResponse = 0;
   expIDResponse = 0;
   configTimeResponse = 0;
   dirResponse = 0;
   nshimmerResponse = 0;
   myIDResponse = 0;
   lsm303dlhcAccelSamplingRateResponse = 0;
   lsm303dlhcAccelLPModeResponse = 0;
   lsm303dlhcAccelHRModeResponse = 0;
   mpu9250GyroRangeResponse = 0;
   bmp180CalibCoeffBtRsp = 0;
   mpu9250SamplingRateResponse = 0;
   mpu9250AccelRangeResponse = 0;
   bmp180OversamplingRatioResponse = 0;
   internalExpPowerEnableResponse = 0;
   mpu9250MagSensAdjValsResponse = 0;
   exgRegsResponse = 0;
   configSetupBytesResponse = 0;
   fwVersionBtRsp = 0;
   blinkLedBtRsp = 0;


   // init BT
   // this must wait till storedConfig is updated
   uint8_t temp_btMacAscii[14], temp_btMacHex[6];
#if USE_BT
   BT_init(huartBt, BtUart_callBack);
   BT_disableRemoteConfig(1);
   uint8_t btName[] = "Shimmer4";
   BT_setFriendlyName((char*)btName);
   BT_setRadioMode(SLAVE_MODE);
   BT_configure();
   if(!BT_getMacAddressHex(temp_btMacHex)){
      S4Ram_btMacHexSet(temp_btMacHex);
      BT_getMacAddressAscii(temp_btMacAscii);
      S4Ram_btMacAsciiSet(temp_btMacAscii);
      stat.isBtPoweredOn = 1;
   }else{
      stat.isBtPoweredOn = 0;
   }
   BT_setTempBaudRate("460K");
   HAL_Delay(100);
#else
   BT_disable(huartBt);
   memcpy(temp_btMacHex, "abcdef", 6);
   S4Ram_btMacHexSet(temp_btMacHex);
#endif //USE_BT

   BtUart_rtsIntCheck();
   //S4Ram_storedConfigSet(temp_btMacHex, NV_MAC_ADDRESS, 6);

   //InfoMem_update(); //InfoMem_write(NV_MAC_ADDRESS, btMacHex, 6);

}

//uint8_t BtUart_isConnected(void){ return btIsConnected;}


uint8_t BtUart_callBack(uint8_t* data_buf) {
   static uint8_t data;
   if (btWaitingForArgs) {
      if (!btWaitingForArgsLength) {
         if ((btAction == SET_EXG_REGS_COMMAND) && (btWaitingForArgs == 3)) {
            btArgs[0] = data_buf[0];
            btArgs[1] = data_buf[1];
            btArgs[2] = data_buf[2];
            BT_setRxLen(btArgs[2]);
            btWaitingForArgsLength = btArgs[2];
            return 0;
         } else if (((btAction == SET_DAUGHTER_CARD_MEM_COMMAND) && (btWaitingForArgs == 3)) ||
                    ((btAction == SET_INFOMEM_COMMAND)           && (btWaitingForArgs == 3)) ||
                    ((btAction == SET_CALIB_DUMP_COMMAND)        && (btWaitingForArgs == 3))) {
            btArgs[0] = data_buf[0];
            btArgs[1] = data_buf[1];
            btArgs[2] = data_buf[2];
            BT_setRxLen(btArgs[0]);
            btWaitingForArgsLength = btArgs[0];
            return 0;
         } else if (((btAction == SET_CENTER_COMMAND) && (btWaitingForArgs == 1)) ||
                    ((btAction == SET_CONFIGTIME_COMMAND)    && (btWaitingForArgs == 1)) ||
                    ((btAction == SET_EXPID_COMMAND)         && (btWaitingForArgs == 1)) ||
                    ((btAction == SET_SHIMMERNAME_COMMAND)   && (btWaitingForArgs == 1))) {
            btArgs[0] = data_buf[0];
            if (btArgs[0]) {
               BT_setRxLen(btArgs[0]);
               btWaitingForArgsLength = btArgs[0];
               return 0;
            }
         }
      }
      if (btWaitingForArgsLength)
         memcpy(btArgs + btWaitingForArgs, data_buf, btWaitingForArgsLength);
      else
         memcpy(btArgs, data_buf, btWaitingForArgs);
      BT_setRxLen(1);
      btWaitingForArgsLength = 0;
      btWaitingForArgs = 0;
      btArgsSize = 0;
      //btProcessCmds = 1;
      S4_Task_set(TASK_BTPROCESS);
      return 1;
   } else {
      data = data_buf[0];
      switch (data) {
      case INQUIRY_COMMAND:
      case DUMMY_COMMAND:
      case GET_SAMPLING_RATE_COMMAND:
      case TOGGLE_LED_COMMAND:
      case START_STREAMING_COMMAND:
      case GET_STATUS_COMMAND:
#if defined(SHIMMER4_SDK)
      case GET_I2C_BATT_STATUS_COMMAND:
#endif
      case GET_VBATT_COMMAND:
      case GET_TRIAL_CONFIG_COMMAND:
      case START_SDBT_COMMAND:
      case GET_CONFIG_SETUP_BYTES_COMMAND:
      case STOP_STREAMING_COMMAND:
      case STOP_SDBT_COMMAND:
      case START_LOGGING_COMMAND:
      case STOP_LOGGING_COMMAND:
      case GET_A_ACCEL_CALIBRATION_COMMAND:
      case GET_MPU9250_GYRO_CALIBRATION_COMMAND:
      case GET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
      case GET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
      case GET_GSR_RANGE_COMMAND:
      case GET_ALL_CALIBRATION_COMMAND:
      case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
      case GET_DEVICE_VERSION_COMMAND:
      case GET_FW_VERSION_COMMAND:
      case GET_CHARGE_STATUS_LED_COMMAND:
      case GET_BUFFER_SIZE_COMMAND:
      case GET_UNIQUE_SERIAL_COMMAND:
      case GET_LSM303DLHC_ACCEL_RANGE_COMMAND:
      case GET_LSM303DLHC_MAG_GAIN_COMMAND:
      case GET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
      case GET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
      case GET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
      case GET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
      case GET_MPU9250_GYRO_RANGE_COMMAND:
      case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
      case GET_MPU9250_SAMPLING_RATE_COMMAND:
      case GET_MPU9250_ACCEL_RANGE_COMMAND:
      case GET_BMP180_PRES_OVERSAMPLING_RATIO_COMMAND:
      case GET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
      case RESET_TO_DEFAULT_CONFIGURATION_COMMAND:
      case RESET_CALIBRATION_VALUE_COMMAND:
      case GET_MPU9250_MAG_SENS_ADJ_VALS_COMMAND:
      case GET_BT_COMMS_BAUD_RATE:
      case GET_CENTER_COMMAND:
      case GET_SHIMMERNAME_COMMAND:
      case GET_EXPID_COMMAND:
      case GET_MYID_COMMAND:
      case GET_NSHIMMER_COMMAND:
      case GET_CONFIGTIME_COMMAND:
      case GET_DIR_COMMAND:
      case GET_DERIVED_CHANNEL_BYTES:
      case GET_RWC_COMMAND:
      case UPD_FLASH_COMMAND:
         btAction = data;
         BT_setRxLen(1);
         //btProcessCmds = 1;
         S4_Task_set(TASK_BTPROCESS);
         return 1;
      case SET_LSM303DLHC_ACCEL_RANGE_COMMAND:
      case SET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
      case SET_LSM303DLHC_MAG_GAIN_COMMAND:
      case SET_CHARGE_STATUS_LED_COMMAND:
      case SET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
      case SET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
      case SET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
      case SET_MPU9250_GYRO_RANGE_COMMAND:
      case SET_MPU9250_SAMPLING_RATE_COMMAND:
      case SET_MPU9250_ACCEL_RANGE_COMMAND:
      case SET_BMP180_PRES_OVERSAMPLING_RATIO_COMMAND:
      case SET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
      case SET_GSR_RANGE_COMMAND:
      case SET_BT_COMMS_BAUD_RATE:
      case SET_CENTER_COMMAND:
      case SET_SHIMMERNAME_COMMAND:
      case SET_EXPID_COMMAND:
      case SET_MYID_COMMAND:
      case SET_NSHIMMER_COMMAND:
      case SET_CONFIGTIME_COMMAND:
      case SET_CRC_COMMAND:
         btWaitingForArgs = 1;
         BT_setRxLen(1);
         btAction = data;
         return 0;
      case SET_SAMPLING_RATE_COMMAND:
      case GET_DAUGHTER_CARD_ID_COMMAND:
#if defined(SHIMMER4_SDK)
      case SET_I2C_BATT_STATUS_FREQ_COMMAND:
#endif
         //case SET_DAUGHTER_CARD_ID_COMMAND:
         btWaitingForArgs = 2;
         BT_setRxLen(2);
         btAction = data;
         return 0;
      case SET_SENSORS_COMMAND:
      case GET_EXG_REGS_COMMAND:
      case SET_EXG_REGS_COMMAND:
      case GET_DAUGHTER_CARD_MEM_COMMAND:
      case SET_DAUGHTER_CARD_MEM_COMMAND:
      case SET_TRIAL_CONFIG_COMMAND:
      case SET_DERIVED_CHANNEL_BYTES:
      case GET_INFOMEM_COMMAND:
      case SET_INFOMEM_COMMAND:
      case GET_CALIB_DUMP_COMMAND:
      case SET_CALIB_DUMP_COMMAND:
         btWaitingForArgs = 3;
         BT_setRxLen(3);
         btAction = data;
         return 0;
      case SET_CONFIG_SETUP_BYTES_COMMAND:
         btWaitingForArgs = 4;
         BT_setRxLen(4);
         btAction = data;
         return 0;
      case SET_RWC_COMMAND:
         btWaitingForArgs = 8;
         BT_setRxLen(8);
         btAction = data;
         return 0;
      case SET_A_ACCEL_CALIBRATION_COMMAND:
      case SET_MPU9250_GYRO_CALIBRATION_COMMAND:
      case SET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
      case SET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
         btWaitingForArgs = 21;
         BT_setRxLen(21);
         btAction = data;
         return 0;
      default:
         BT_setRxLen(1);
         return 0;
      }
   }
   //return 0;
}


void BtUart_processCmd(void) {
   uint64_t temp64;
#if defined(SHIMMER4_SDK)
   uint16_t temp16;
#endif

   //HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_6);//green

   //uint32_t config_time;
   //uint8_t my_config_time[4];
   //uint8_t name_len;
   //uint8_t update_sdconfig = 0, calib_update = 0, calib_sensor = 0, calib_range = 0;
   switch (btAction) {
   case INQUIRY_COMMAND:
      inquiryBtRsp = 1;
      break;
   case DUMMY_COMMAND:
      break;
   case GET_SAMPLING_RATE_COMMAND:
      samplingRateBtRsp = 1;
      break;
   case TOGGLE_LED_COMMAND:
      toggleLedRed ^= 1;
      break;
   case START_STREAMING_COMMAND:
      stat.btstreamCmd = BT_STREAM_CMD_STATE_START;
      S4_Task_set(TASK_STARTSENSING);
      break;
   case START_LOGGING_COMMAND:
      stat.sdlogCmd = 1;
      S4_Task_set(TASK_STARTSENSING);
      break;
   case START_SDBT_COMMAND:
      stat.btstreamCmd = BT_STREAM_CMD_STATE_START;
      stat.sdlogCmd = 1;
      S4_Task_set(TASK_STARTSENSING);
      break;
//   case SET_CRC_COMMAND:
//      crcChecksum = args[0];
//      break;
   case STOP_STREAMING_COMMAND:
      stat.btstreamCmd = BT_STREAM_CMD_STATE_STOP;
      S4_Task_set(TASK_STOPSENSING);
      break;
   case STOP_LOGGING_COMMAND:
      stat.sdlogCmd = 2;
      S4_Task_set(TASK_STOPSENSING);
      break;
   case STOP_SDBT_COMMAND:
      stat.btstreamCmd = BT_STREAM_CMD_STATE_STOP;
      stat.sdlogCmd = 2;
      S4_Task_set(TASK_STOPSENSING);
      break;
   case SET_SENSORS_COMMAND:
      S4Ram_storedConfigSet(btArgs, NV_SENSORS0, 3);
      S4Ram_sdHeadTextSet(btArgs, NV_SENSORS0, 3);
      //InfoMem_update();
      break;
   case GET_LSM303DLHC_ACCEL_RANGE_COMMAND:
      lsm303dlhcAccelRangeResponse = 1;
      break;
   case GET_LSM303DLHC_MAG_GAIN_COMMAND:
      lsm303dlhcMagGainResponse = 1;
      break;
   case GET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
      lsm303dlhcMagSamplingRateResponse = 1;
      break;
   case GET_STATUS_COMMAND:
      dockStatusBtRsp = 1;
      break;
#if defined(SHIMMER4_SDK)
   case GET_I2C_BATT_STATUS_COMMAND:
      i2cvBattBtRsp = 1;
      break;
   case SET_I2C_BATT_STATUS_FREQ_COMMAND:
      temp16 = btArgs[0] + ((uint16_t)btArgs[1]<<8);
      I2C_readBattSetFreq(temp16);
      break;
#endif
   case GET_VBATT_COMMAND:
      vbattBtRsp = 1;
      break;
   case GET_TRIAL_CONFIG_COMMAND:
      trialConfigResponse = 1;
      break;
   case SET_TRIAL_CONFIG_COMMAND:
      S4Ram_storedConfigSetByte(NV_SD_TRIAL_CONFIG0, btArgs[0]);
      S4Ram_storedConfigSetByte(NV_SD_TRIAL_CONFIG1, 0);
      S4Ram_storedConfigSetByte(NV_SD_BT_INTERVAL, btArgs[2]);
      S4Ram_sdHeadTextSetByte(SDH_TRIAL_CONFIG0, btArgs[0]);
      S4Ram_sdHeadTextSetByte(SDH_TRIAL_CONFIG1, 0);
      S4Ram_sdHeadTextSetByte(SDH_BROADCAST_INTERVAL, btArgs[2]);
      InfoMem_update();
      break;
   /*case GET_CENTER_COMMAND:
    centerResponse = 1;
    break;
   case SET_CENTER_COMMAND:
    break;
   case GET_SHIMMERNAME_COMMAND:
    shimmerNameResponse = 1;
    break;
   case SET_SHIMMERNAME_COMMAND:
    name_len = btArgs[0]<(MAX_CHARS-1)?args[0]:(MAX_CHARS-1);
    memset((uint8_t*)(storedConfig+NV_SD_SHIMMER_NAME), 0, MAX_CHARS-1);
    memcpy((uint8_t*)(storedConfig+NV_SD_SHIMMER_NAME), &args[1], name_len);
    InfoMem_write((uint8_t*)NV_SD_SHIMMER_NAME, storedConfig+NV_SD_SHIMMER_NAME, MAX_CHARS-1);
    SetShimmerName();
    update_sdconfig = 1;
    break;
   case GET_EXPID_COMMAND:
    expIDResponse = 1;
    break;
   case SET_EXPID_COMMAND:
    name_len = args[0]<(MAX_CHARS-1)?args[0]:(MAX_CHARS-1);
    memset((uint8_t*)(storedConfig+NV_SD_EXP_ID_NAME), 0, MAX_CHARS-1);
    memcpy((uint8_t*)(storedConfig+NV_SD_EXP_ID_NAME), &args[1], name_len);
    InfoMem_write((uint8_t*)NV_SD_EXP_ID_NAME, storedConfig+NV_SD_EXP_ID_NAME, MAX_CHARS-1);
    SetExpIdName();
    update_sdconfig = 1;
    break;
   case GET_CONFIGTIME_COMMAND:
    configTimeResponse = 1;
    break;
   case GET_DIR_COMMAND:
    dirResponse = 1;
    break;
   case SET_CONFIGTIME_COMMAND:
    name_len = args[0]<(MAX_CHARS-1)?args[0]:(MAX_CHARS-1);
    memcpy(configTimeText, &args[1], name_len);
    configTimeText[args[0]]='\0';
    SetName();
    config_time = atol((char*)configTimeText);
    my_config_time[3] = *((uint8_t*)&config_time);
    my_config_time[2] = *(((uint8_t*)&config_time)+1);
    my_config_time[1] = *(((uint8_t*)&config_time)+2);
    my_config_time[0] = *(((uint8_t*)&config_time)+3);
    memcpy(&sdHeadText[SDH_CONFIG_TIME_0], my_config_time, 4);
    memcpy((uint8_t*)(storedConfig+NV_SD_CONFIG_TIME), my_config_time, 4);
    InfoMem_write((uint8_t*)NV_SD_CONFIG_TIME, storedConfig+NV_SD_CONFIG_TIME, 4);
    update_sdconfig = 1;
    break;
   case GET_NSHIMMER_COMMAND:
    nshimmerResponse = 1;
    break;
   case SET_NSHIMMER_COMMAND:
    storedConfig[NV_SD_NSHIMMER] = args[0];
    sdHeadText[SDH_NSHIMMER] = args[0];
    InfoMem_write((uint8_t*)NV_SD_NSHIMMER, storedConfig+NV_SD_NSHIMMER, 1);
    update_sdconfig = 1;
    break;
   case GET_MYID_COMMAND:
    myIDResponse = 1;
    break;
   case SET_MYID_COMMAND:
    storedConfig[NV_SD_MYTRIAL_ID] = args[0];
    sdHeadText[SDH_MYTRIAL_ID] = args[0];
    InfoMem_write((uint8_t*)NV_SD_MYTRIAL_ID, storedConfig+NV_SD_MYTRIAL_ID, 1);
    update_sdconfig = 1;
    break;
   case SET_LSM303DLHC_ACCEL_RANGE_COMMAND:
    if(args[0] < 4)
    storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0xF3) + ((args[0]&0x03)<<2);
    else
    storedConfig[NV_CONFIG_SETUP_BYTE0] &= 0xF3;
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE0, &storedConfig[NV_CONFIG_SETUP_BYTE0], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
    update_sdconfig = 1;
    if(isSensing) {
    stopSensing = 1;
    startSensing = 1;
    }
    break;
   case GET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
    lsm303dlhcAccelSamplingRateResponse = 1;
    break;
   case GET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
    lsm303dlhcAccelLPModeResponse = 1;
    break;
   case GET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
    lsm303dlhcAccelHRModeResponse = 1;
    break;
   case GET_MPU9250_GYRO_RANGE_COMMAND:
    mpu9250GyroRangeResponse = 1;
    break;*/
   case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
      bmp180CalibCoeffBtRsp = 1;
      break;/*
  case GET_MPU9250_SAMPLING_RATE_COMMAND:
    mpu9250SamplingRateResponse = 1;
    break;
  case GET_MPU9250_ACCEL_RANGE_COMMAND:
    mpu9250AccelRangeResponse = 1;
    break;
  case GET_BMP180_PRES_OVERSAMPLING_RATIO_COMMAND:
    bmp180OversamplingRatioResponse = 1;
    break;
  case GET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
    internalExpPowerEnableResponse = 1;
    break;
  case GET_MPU9250_MAG_SENS_ADJ_VALS_COMMAND:
    mpu9250MagSensAdjValsResponse = 1;
    break;
  case GET_EXG_REGS_COMMAND:
    if(args[0]<2 && args[1]<10 && args[2]<11) {
      exgChip = args[0];
      exgStartAddr = args[1];
      exgLength = args[2];
    } else
      exgLength = 0;
    exgRegsResponse = 1;
    break;
*/
   /*   case SET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
    if(args[0] < 10)
      storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0x0F) + ((args[0]&0x0F)<<4);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0x0F) + (LSM303DLHC_ACCEL_100HZ<<4);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE0, &storedConfig[NV_CONFIG_SETUP_BYTE0], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_LSM303DLHC_MAG_GAIN_COMMAND:
    if(args[0]>0 && args[0]<8)
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0x1F) + ((args[0]&0x07)<<5);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0x1F) + (LSM303DLHC_MAG_1_3G<<5);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE2, &storedConfig[NV_CONFIG_SETUP_BYTE2], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE2] = storedConfig[NV_CONFIG_SETUP_BYTE2];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
    if(args[0] < 8)
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0xE3) + ((args[0]&0x07)<<2);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0xE3) + (LSM303DLHC_MAG_75HZ<<2);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE2, &storedConfig[NV_CONFIG_SETUP_BYTE2], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE2] = storedConfig[NV_CONFIG_SETUP_BYTE2];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
    if(args[0] == 1){
      storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0xFD) + 0x02;
    }else{
      storedConfig[NV_CONFIG_SETUP_BYTE0] &= 0xFD;
    }
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE0, &storedConfig[NV_CONFIG_SETUP_BYTE0], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
    if(args[0] == 1){
      storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0xFE) + 0x01;
    }else{
      storedConfig[NV_CONFIG_SETUP_BYTE0] &= 0xFE;
    }
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE0, &storedConfig[NV_CONFIG_SETUP_BYTE0], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_MPU9250_GYRO_RANGE_COMMAND:
    if(args[0] < 4)
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0xFC) + (args[0]&0x03);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0xFC) + MPU9250_GYRO_500DPS;
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE2, &storedConfig[NV_CONFIG_SETUP_BYTE2], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE2] = storedConfig[NV_CONFIG_SETUP_BYTE2];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_MPU9250_SAMPLING_RATE_COMMAND:
    storedConfig[NV_CONFIG_SETUP_BYTE1] = args[0];
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE1, &storedConfig[NV_CONFIG_SETUP_BYTE1], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE1] = storedConfig[NV_CONFIG_SETUP_BYTE1];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_MPU9250_ACCEL_RANGE_COMMAND:
    if(args[0] < 4)
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0x3F) + ((args[0]&0x03)<<6);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0x3F) + (ACCEL_2G<<6);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE3, &storedConfig[NV_CONFIG_SETUP_BYTE3], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_BMP180_PRES_OVERSAMPLING_RATIO_COMMAND:
    if(args[0] < 4)
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0xCF) + ((args[0]&0x03)<<4);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0xCF) + (BMP180_OSS_1<<4);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE3, &storedConfig[NV_CONFIG_SETUP_BYTE3], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
    update_sdconfig = 1;
    break;
    case SET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
    if(args[0] == 1)
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0xFE) + (args[0]&0x01);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE3] &= 0xFE;
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE3, &storedConfig[NV_CONFIG_SETUP_BYTE3], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
   */
   case GET_CONFIG_SETUP_BYTES_COMMAND:
      configSetupBytesResponse = 1;
      break;
   case SET_CONFIG_SETUP_BYTES_COMMAND:
      S4Ram_storedConfigSet(btArgs, NV_CONFIG_SETUP_BYTE0, 4);
      //InfoMem_update();
      break;
   case SET_SAMPLING_RATE_COMMAND:
      S4Ram_storedConfigSet(btArgs, NV_SAMPLING_RATE, 2);
      S4Ram_sdHeadTextSet(btArgs, SDH_SAMPLE_RATE_0, 2);
      //InfoMem_update();
      break;
   case GET_CALIB_DUMP_COMMAND:
      // usage:
      // 0x98, offset, offset, length
      btCalibRamLength = btArgs[0];
      btCalibRamOffset = btArgs[1] + (btArgs[2]<<8);
      calibRamResponse = 1;
      break;
   case SET_CALIB_DUMP_COMMAND:
      // usage:
      // 0x98, offset, offset, length, data[0:127]
      // max length of this command = 132
      btCalibRamLength = btArgs[0];
      btCalibRamOffset = btArgs[1] + (btArgs[2]<<8);
      if(ShimmerCalib_ramWrite(btArgs+3, btCalibRamLength, btCalibRamOffset) == 1){
         //InfoMem_update();
//         ShimmerCalibSyncFromDumpRamAll();
//         update_calib_dump_file = 1;
      }
      break;
   case UPD_FLASH_COMMAND:
      InfoMem_update();
      //ShimmerCalibSyncFromDumpRamAll();
      //update_calib_dump_file = 1;
      break;
   /*   case SET_A_ACCEL_CALIBRATION_COMMAND:
    memcpy(&storedConfig[NV_A_ACCEL_CALIBRATION], args, 21);
    InfoMem_write((void*)NV_A_ACCEL_CALIBRATION, &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
    memcpy(&sdHeadText[SDH_A_ACCEL_CALIBRATION], &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
    calib_update = 1;
    calib_sensor = S_ACCEL_A;
    break;
    case GET_A_ACCEL_CALIBRATION_COMMAND:
    aAccelCalibrationResponse = 1;
    break;
    case SET_MPU9250_GYRO_CALIBRATION_COMMAND:
    memcpy(&storedConfig[NV_MPU9250_GYRO_CALIBRATION], args, 21);
    InfoMem_write((void*)NV_MPU9250_GYRO_CALIBRATION, &storedConfig[NV_MPU9250_GYRO_CALIBRATION], 21);
    memcpy(&sdHeadText[SDH_MPU9250_GYRO_CALIBRATION], &storedConfig[NV_MPU9250_GYRO_CALIBRATION], 21);
    calib_update = 1;
    calib_sensor = S_GYRO;
    calib_range = storedConfig[NV_CONFIG_SETUP_BYTE2] & 0x03;
    break;
    case GET_MPU9250_GYRO_CALIBRATION_COMMAND:
    gyroCalibrationResponse = 1;
    break;
    case SET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
    memcpy(&storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], args, 21);
    InfoMem_write((void*)NV_LSM303DLHC_MAG_CALIBRATION, &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
    memcpy(&sdHeadText[SDH_LSM303DLHC_MAG_CALIBRATION], &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
    calib_update = 1;
    calib_sensor = S_MAG;
    calib_range = (storedConfig[NV_CONFIG_SETUP_BYTE2]>>5) & 0x07;
    break;
    case GET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
    magCalibrationResponse = 1;
    break;
    case SET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
    memcpy(&storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], args, 21);
    InfoMem_write((void*)NV_LSM303DLHC_ACCEL_CALIBRATION, &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
    memcpy(&sdHeadText[SDH_LSM303DLHC_ACCEL_CALIBRATION], &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
    calib_update = 1;
    calib_sensor = S_ACCEL_D;
    calib_range = (storedConfig[NV_CONFIG_SETUP_BYTE0]>>2)&0x03;
    break;*/
   case SET_GSR_RANGE_COMMAND:
      if (btArgs[0] <= 4){
        S4Ram_getStoredConfig()->gsrRange = (btArgs[0] & 0x07);
      }else{
        S4Ram_getStoredConfig()->gsrRange = GSR_AUTORANGE;
      }
      //InfoMem_write((void*)NV_CONFIG_SETUP_BYTE3, &storedConfig[NV_CONFIG_SETUP_BYTE3], 1);
      //InfoMem_update();
      //sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
//    update_sdconfig = 1;
//    if(isSensing) {
//      stopSensing = 1;
//      startSensing = 1;
//    }
      break;
   case SET_EXG_REGS_COMMAND:
      if (btArgs[0] < 2 && btArgs[1] < 10 && btArgs[2] < 11) {
         if (btArgs[0]) {
            //memcpy((storedConfig + NV_EXG_ADS1292R_2_CONFIG1 + btArgs[1]), (btArgs + 3), btArgs[2]);
            S4Ram_storedConfigSet(btArgs + 3, NV_EXG_ADS1292R_2_CONFIG1 + btArgs[1], btArgs[2]);
            //InfoMem_update();
            //memcpy(sdHeadText + SDH_EXG_ADS1292R_2_CONFIG1, storedConfig + NV_EXG_ADS1292R_2_CONFIG1, btArgs[2]);
         } else {
            //memcpy((storedConfig + NV_EXG_ADS1292R_1_CONFIG1 + btArgs[1]), (btArgs + 3), btArgs[2]);
            S4Ram_storedConfigSet(btArgs + 3, NV_EXG_ADS1292R_1_CONFIG1 + btArgs[1], btArgs[2]);
            //InfoMem_update();
            //memcpy(sdHeadText + SDH_EXG_ADS1292R_1_CONFIG1, storedConfig + NV_EXG_ADS1292R_1_CONFIG1, btArgs[2]);
         }
         //update_sdconfig = 1;
      }
      break;
   /*     case RESET_TO_DEFAULT_CONFIGURATION_COMMAND:
     SetDefaultConfiguration();
     Config2SdHead();
     update_sdconfig = 1;
     configureAdcChannels = 1;
     if(isSensing) {
       stopSensing = 1;
       startSensing = 1;
     }
     break;
     case RESET_CALIBRATION_VALUE_COMMAND:
     memset(&storedConfig[NV_A_ACCEL_CALIBRATION], 0xFF, NV_NUM_CALIBRATION_BYTES);
     InfoMem_write((void*)NV_A_ACCEL_CALIBRATION, &storedConfig[NV_A_ACCEL_CALIBRATION], NV_NUM_CALIBRATION_BYTES);
     memcpy(&sdHeadText[SDH_LSM303DLHC_ACCEL_CALIBRATION], &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
     memcpy(&sdHeadText[SDH_MPU9250_GYRO_CALIBRATION], &storedConfig[NV_MPU9250_GYRO_CALIBRATION], 21);
     memcpy(&sdHeadText[SDH_LSM303DLHC_MAG_CALIBRATION], &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
     memcpy(&sdHeadText[SDH_A_ACCEL_CALIBRATION], &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
     calib_update = 1;
     break;
     case GET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
     dAccelCalibrationResponse = 1;
     break;
     case GET_GSR_RANGE_COMMAND:
     gsrRangeResponse = 1;
     break;
     case GET_ALL_CALIBRATION_COMMAND:
     allCalibrationResponse = 1;
     break;
   */
   case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
   case GET_DEVICE_VERSION_COMMAND:
      deviceVersionBtRsp = 1;
      break;
   case GET_FW_VERSION_COMMAND:
      fwVersionBtRsp = 1;
      break;
   case GET_CHARGE_STATUS_LED_COMMAND:
      blinkLedBtRsp = 1;
      break;/*
  case GET_BUFFER_SIZE_COMMAND:
    bufferSizeResponse = 1;
    break;
  case GET_UNIQUE_SERIAL_COMMAND:
    uniqueSerialResponse = 1;
    break;
*/
   case GET_DAUGHTER_CARD_ID_COMMAND:
      btDcMemLength = btArgs[0];
      btDcMemOffset = btArgs[1];
      if ((btDcMemLength <= 16) && (btDcMemOffset <= 15) && (btDcMemLength + btDcMemOffset <= 16))
         dcIdBtRsp = 1;
      break;
//   case SET_DAUGHTER_CARD_ID_COMMAND:
//      btDcMemLength = args[0];
//      btDcMemOffset = args[1];
//      if((btDcMemLength<=16) && (btDcMemOffset<=15) && (btDcMemLength+btDcMemOffset<=16)) {
//         CAT24C16_init();
//         CAT24C16_write(btDcMemOffset, btDcMemLength, args+2);
//         CAT24C16_powerOff();
//      }
//      break;
   case GET_DAUGHTER_CARD_MEM_COMMAND:
      btDcMemLength = btArgs[0];
      btDcMemOffset = btArgs[1] + (btArgs[2] << 8);
      if ((btDcMemLength <= 128) && (btDcMemOffset <= 2031) && (btDcMemLength + btDcMemOffset <= 2032))
         dcMemBtRsp = 1;
      break;
   case SET_DAUGHTER_CARD_MEM_COMMAND:
      btDcMemLength = btArgs[0];
      btDcMemOffset = btArgs[1] + (btArgs[2] << 8);
      if ((btDcMemLength <= 128) && (btDcMemOffset <= 2031) && (btDcMemLength + btDcMemOffset <= 2032)) {
         CAT24C16_write(btDcMemOffset + 16, btArgs + 3, btDcMemLength);
      }
      break;
   /*   case GET_BT_COMMS_BAUD_RATE:
    btCommsBaudRateResponse = 1;
    break;
    case SET_BT_COMMS_BAUD_RATE:
    if(args[0] != storedConfig[NV_BT_COMMS_BAUD_RATE]) {
      if(args[0]<11) {
        changeBtBaudRate = args[0];
      } else {
        changeBtBaudRate = 9;
      }
    }
    break;
    case GET_DERIVED_CHANNEL_BYTES:
    derivedChannelResponse = 1;
    break;
    case SET_DERIVED_CHANNEL_BYTES:
    memcpy(&storedConfig[NV_DERIVED_CHANNELS_0], &args[0], 3);
    InfoMem_write((void*)NV_DERIVED_CHANNELS_0, &storedConfig[NV_DERIVED_CHANNELS_0], 3);
    memcpy(&sdHeadText[SDH_DERIVED_CHANNELS_0], &storedConfig[NV_DERIVED_CHANNELS_0], 3);
    update_sdconfig = 1;
    break;
   */
   case GET_INFOMEM_COMMAND:
      btInfomemLength = btArgs[0];
      btInfomemOffset = btArgs[1] + (btArgs[2] << 8);
      if ((btInfomemLength <= 128) && (btInfomemOffset <= (NV_NUM_RWMEM_BYTES - 1)) && (btInfomemLength + btInfomemOffset <= NV_NUM_RWMEM_BYTES))
         infomemBtRsp = 1;
      break;
   case SET_INFOMEM_COMMAND:
      btInfomemLength = btArgs[0];
      btInfomemOffset = btArgs[1] + (btArgs[2] << 8);
      if ((btInfomemLength <= 128) && (btInfomemOffset <= (NV_NUM_RWMEM_BYTES - 1)) && (btInfomemLength + btInfomemOffset <= NV_NUM_RWMEM_BYTES)) {
//         memcpy(btMacHex, storedConfig + NV_MAC_ADDRESS, 6);
//         memcpy(storedConfig + btInfomemOffset, btArgs + 3, btInfomemLength);
//         memcpy(storedConfig + NV_MAC_ADDRESS, btMacHex, 6);
         uint8_t temp_btMacHex[6];
         S4Ram_storedConfigGet(temp_btMacHex, NV_MAC_ADDRESS, 6);
         S4Ram_storedConfigSet(btArgs + 3, btInfomemOffset, btInfomemLength);
         S4Ram_storedConfigSet(temp_btMacHex, NV_MAC_ADDRESS, 6);
         //InfoMem_update();
         //Infomem2Names();
         //update_sdconfig = 1;
         //if(((btInfomemOffset>=NV_A_ACCEL_CALIBRATION) && (btInfomemOffset<=NV_CALIBRATION_END)) ||
         //   (((btInfomemLength+btInfomemOffset)>=NV_A_ACCEL_CALIBRATION) && ((btInfomemLength+btInfomemOffset)<=NV_CALIBRATION_END)) ||
         //   ((btInfomemOffset<=NV_A_ACCEL_CALIBRATION) && ((btInfomemLength+btInfomemOffset)>=NV_CALIBRATION_END)))
         //   calib_update = 1;
      }
      else
         return;
      break;
   case GET_RWC_COMMAND:
      rwcResponse = 1;
      break;
   case SET_RWC_COMMAND:
      memcpy((uint8_t*)(&temp64), btArgs, 8);// 64bits = 8bytes
      RTC_init(temp64);
      //storedConfig[NV_SD_TRIAL_CONFIG0] |= SDH_RTC_SET_BY_BT;
      S4Ram_storedConfigSetByte(NV_SD_TRIAL_CONFIG0, S4Ram_storedConfigGetByte(NV_SD_TRIAL_CONFIG0) | SDH_RTC_SET_BY_BT);
      //InfoMem_update();
      break;
   default:;
   }
   //sendAck = 1;
   //btSendRsps = 1;
   S4_Task_set(TASK_BTRESPONSE);
//   if(update_sdconfig && CheckSdInslot()){
//      if(!docked)
//         UpdateSdConfig();
//      else
//         SetSdCfgFlag(1);
//   }
//   if(calib_update && CheckSdInslot()){
//      if(!docked)
//         CalibNewFile(calib_sensor, calib_range);
//      else
//         SetCalibFlag(1);
//      //   CalibAll();
//      //else
//      //   SetCalibFlag(1);
//   }
}
void BtUart_sendRsp(void) {
#if USE_BT
   uint16_t packet_length = 0;
   //STATTypeDef * stat = GetStatus();
   uint8_t bt_tx_data[RESPONSE_PACKET_SIZE];
   packet_length = 0;
   if (stat.isBtConnected) {
      //if(sendAck) {
      *(bt_tx_data + packet_length++) = ACK_COMMAND_PROCESSED;
      //   sendAck = 0;
      //}
      if (inquiryBtRsp) {//todo:
         *(bt_tx_data + packet_length++) = INQUIRY_RESPONSE;
         //*(uint16_t *)(bt_tx_data + packet_length) = *(uint16_t *)(storedConfig + NV_SAMPLING_RATE); //ADC sampling rate
         S4Ram_storedConfigGet(bt_tx_data + packet_length, NV_SAMPLING_RATE, 2);
         packet_length += 2;
//         memcpy((bt_tx_data + packet_length), (storedConfig + NV_CONFIG_SETUP_BYTE0), 4);           //4 config bytes
         S4Ram_storedConfigGet(bt_tx_data + packet_length, NV_CONFIG_SETUP_BYTE0, 4);
         packet_length += 4;
         *(bt_tx_data + packet_length++) = sensing.nbrAdcChans + sensing.nbrDigiChans;                        //number of data channels
         *(bt_tx_data + packet_length++) = S4Ram_storedConfigGetByte(NV_BUFFER_SIZE);                      //buffer size
         memcpy((bt_tx_data + packet_length), sensing.cc, (sensing.nbrAdcChans + sensing.nbrDigiChans));
         packet_length += sensing.nbrAdcChans + sensing.nbrDigiChans;
         inquiryBtRsp = 0;
      } else if (samplingRateBtRsp) {
         *(bt_tx_data + packet_length++) = SAMPLING_RATE_RESPONSE;
         //*(uint16_t *)(bt_tx_data + packet_length) = *(uint16_t *)(storedConfig + NV_SAMPLING_RATE); //ADC sampling rate
         S4Ram_storedConfigGet(bt_tx_data + packet_length, NV_SAMPLING_RATE, 2);
         packet_length += 2;
         samplingRateBtRsp = 0;
//      } else if(lsm303dlhcAccelRangeResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_RANGE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE0] & 0x0C) >> 2;
//         lsm303dlhcAccelRangeResponse = 0;
//      } else if(lsm303dlhcMagGainResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_MAG_GAIN_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE2] & 0xE0) >> 5;
//         lsm303dlhcMagGainResponse = 0;
//      } else if(lsm303dlhcMagSamplingRateResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_MAG_SAMPLING_RATE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE2] & 0x1C) >> 2;
//         lsm303dlhcMagSamplingRateResponse = 0;
      } else if (dockStatusBtRsp) {
         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
         *(bt_tx_data + packet_length++) = STATUS_RESPONSE;
//         *(bt_tx_data + packet_length++) = ((isStreaming & 0x01) << 4) +
//                                         ((isLogging & 0x01) << 3) + ((0 & 0x01) << 2) +
//                                         ((sensing.en & 0x01) << 1) + (docked & 0x01);
         *(bt_tx_data + packet_length++) = 0;
         dockStatusBtRsp = 0;
#if defined(SHIMMER4_SDK)
      } else if (i2cvBattBtRsp) {
         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
         *(bt_tx_data + packet_length++) = RSP_I2C_BATT_STATUS_COMMAND;
         memcpy((bt_tx_data + packet_length), (uint8_t*)stat.battDigital, STC3100_DATA_LEN);
         packet_length += STC3100_DATA_LEN;
         i2cvBattBtRsp = 0;
#endif
      } else if (vbattBtRsp) {
         //ReadBatt();
         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
         *(bt_tx_data + packet_length++) = VBATT_RESPONSE;
         memcpy((uint8_t*)(bt_tx_data + packet_length), (uint8_t*)stat.battVal, 3);
         packet_length += 3;
         vbattBtRsp = 0;
//      } else if(trialConfigResponse) {
//         *(bt_tx_data + packet_length++) = TRIAL_CONFIG_RESPONSE;
//         memcpy((bt_tx_data + packet_length), (storedConfig+NV_SD_TRIAL_CONFIG0), 3);  //2 trial config bytes + 1 interval byte
//         packet_length+=3;
//         trialConfigResponse = 0;
//      } else if(centerResponse) {
//         centerResponse = 0;
//      } else if(shimmerNameResponse) {
//         SetShimmerName();
//         uint8_t shimmer_name_len = strlen((char*)shimmerName);
//         *(bt_tx_data + packet_length++) = SHIMMERNAME_RESPONSE;
//         *(bt_tx_data + packet_length++) = shimmer_name_len;
//         memcpy((bt_tx_data + packet_length), shimmerName, shimmer_name_len);
//         packet_length+=shimmer_name_len;
//         shimmerNameResponse = 0;
//      } else if(expIDResponse) {
//         SetExpIdName();
//         uint8_t exp_id_name_len = strlen((char*)expIdName);
//         *(bt_tx_data + packet_length++) = EXPID_RESPONSE;
//         *(bt_tx_data + packet_length++) = exp_id_name_len;
//         memcpy((bt_tx_data + packet_length), expIdName, exp_id_name_len);
//         packet_length+=exp_id_name_len;
//         expIDResponse = 0;
//      } else if(configTimeResponse) {
//         SetCfgTime();
//         uint8_t cfgtime_name_len = strlen((char*)configTimeText);
//         *(bt_tx_data + packet_length++) = CONFIGTIME_RESPONSE;
//         *(bt_tx_data + packet_length++) = cfgtime_name_len;
//         memcpy((bt_tx_data + packet_length), configTimeText, cfgtime_name_len);
//         packet_length+=cfgtime_name_len;
//         configTimeResponse = 0;
//      } else if(dirResponse) {
//         uint8_t dir_len = strlen((char*)fileName)-3;
//         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
//         *(bt_tx_data + packet_length++) = DIR_RESPONSE;
//         *(bt_tx_data + packet_length++) = dir_len;
//         memcpy((bt_tx_data + packet_length), fileName, dir_len);
//         packet_length+=dir_len;
//         dirResponse = 0;
//      } else if(nshimmerResponse) {
//         *(bt_tx_data + packet_length++) = NSHIMMER_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_SD_NSHIMMER];
//         nshimmerResponse = 0;
//      } else if(myIDResponse) {
//          *(bt_tx_data + packet_length++) = MYID_RESPONSE;
//          *(bt_tx_data + packet_length++) = storedConfig[NV_SD_MYTRIAL_ID];
//          myIDResponse = 0;
//      } else if(lsm303dlhcAccelSamplingRateResponse) {
//          *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_SAMPLING_RATE_RESPONSE;
//          *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE0] & 0xF0) >> 4;
//          lsm303dlhcAccelSamplingRateResponse = 0;
//      } else if(lsm303dlhcAccelLPModeResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_LPMODE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE0] & 0x02) >> 1;
//         lsm303dlhcAccelLPModeResponse = 0;
//      } else if(lsm303dlhcAccelHRModeResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_HRMODE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_CONFIG_SETUP_BYTE0] & 0x01;
//         lsm303dlhcAccelHRModeResponse = 0;
//      } else if(mpu9250GyroRangeResponse) {
//         *(bt_tx_data + packet_length++) = MPU9250_GYRO_RANGE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_CONFIG_SETUP_BYTE2] & 0x03;
//         mpu9250GyroRangeResponse = 0;
      } else if (bmp180CalibCoeffBtRsp) {
         *(bt_tx_data + packet_length++) = BMP180_CALIBRATION_COEFFICIENTS_RESPONSE;
//         BMP180_init(hi2cSensor);//todo: pre read at init
//         BMP180_getCalib(bt_tx_data + packet_length);
//         packet_length += 22;
//         memset((bt_tx_data + packet_length), 0, 24);
//         packet_length += 24;
//         bmp180CalibCoeffBtRsp = 0;
//      } else if(mpu9250SamplingRateResponse) {
//         *(bt_tx_data + packet_length++) = MPU9250_SAMPLING_RATE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_CONFIG_SETUP_BYTE1];
//         mpu9250SamplingRateResponse = 0;
//      } else if(mpu9250AccelRangeResponse) {
//         *(bt_tx_data + packet_length++) = MPU9250_ACCEL_RANGE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0xC0) >> 6;
//         mpu9250AccelRangeResponse = 0;
//      } else if(bmp180OversamplingRatioResponse) {
//         *(bt_tx_data + packet_length++) = BMP180_PRES_OVERSAMPLING_RATIO_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0x30) >> 4;
//         bmp180OversamplingRatioResponse = 0;
//      } else if(internalExpPowerEnableResponse) {
//         *(bt_tx_data + packet_length++) = INTERNAL_EXP_POWER_ENABLE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_CONFIG_SETUP_BYTE3]&0x01;
//         internalExpPowerEnableResponse = 0;
      } else if (configSetupBytesResponse) {
         *(bt_tx_data + packet_length++) = CONFIG_SETUP_BYTES_RESPONSE;
         //memcpy((bt_tx_data + packet_length), &storedConfig[NV_CONFIG_SETUP_BYTE0], 4);
         S4Ram_storedConfigGet(bt_tx_data + packet_length, NV_CONFIG_SETUP_BYTE0, 4);
         packet_length += 4;
         configSetupBytesResponse = 0;
      } else if(calibRamResponse) {
         *(bt_tx_data + packet_length++) = RSP_CALIB_DUMP_COMMAND;
         *(bt_tx_data + packet_length++) = btCalibRamLength;
         *(bt_tx_data + packet_length++) = btCalibRamOffset&0xff;
         *(bt_tx_data + packet_length++) = (btCalibRamOffset>>8)&0xff;
         ShimmerCalib_ramRead(bt_tx_data+packet_length, btCalibRamLength, btCalibRamOffset);
         packet_length += btCalibRamLength;
         calibRamResponse = 0;
//      } else if(aAccelCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = A_ACCEL_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
//         packet_length += 21;
//         aAccelCalibrationResponse = 0;
//      } else if(gyroCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = MPU9250_GYRO_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_MPU9250_GYRO_CALIBRATION], 21);
//         packet_length += 21;
//         gyroCalibrationResponse = 0;
//      } else if(magCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_MAG_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
//         packet_length += 21;
//         magCalibrationResponse = 0;
//      } else if(dAccelCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
//         packet_length += 21;
//         dAccelCalibrationResponse = 0;
//      } else if(gsrRangeResponse) {
//         *(bt_tx_data + packet_length++) = GSR_RANGE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0x0E) >> 1;
//         gsrRangeResponse = 0;
//      } else if(allCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = ALL_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_A_ACCEL_CALIBRATION], NV_NUM_CALIBRATION_BYTES);
//         packet_length += NV_NUM_CALIBRATION_BYTES;
//         allCalibrationResponse = 0;
      } else if (deviceVersionBtRsp) {
         *(bt_tx_data + packet_length++) = DEVICE_VERSION_RESPONSE;
         *(bt_tx_data + packet_length++) = DEVICE_VER;
         deviceVersionBtRsp = 0;
//      } else if(mpu9250MagSensAdjValsResponse) {
//         MPU9250_init();
//         MPU9250_wake(1);
//         MPU9250_wake(0);
//         *(bt_tx_data + packet_length++) = MPU9250_MAG_SENS_ADJ_VALS_RESPONSE;
//         MPU9250_getMagSensitivityAdj(bt_tx_data+packet_length);
//         packet_length += 3;
//         mpu9250MagSensAdjValsResponse = 0;
      } else if (fwVersionBtRsp) {
         *(bt_tx_data + packet_length++) = FW_VERSION_RESPONSE;
         *(bt_tx_data + packet_length++) = FW_IDENTIFIER & 0xFF;
         *(bt_tx_data + packet_length++) = (FW_IDENTIFIER & 0xFF00) >> 8;
         *(bt_tx_data + packet_length++) = FW_VER_MAJOR & 0xFF;
         *(bt_tx_data + packet_length++) = (FW_VER_MAJOR & 0xFF00) >> 8;
         *(bt_tx_data + packet_length++) = FW_VER_MINOR;
         *(bt_tx_data + packet_length++) = FW_VER_REL;
         fwVersionBtRsp = 0;
      } else if (blinkLedBtRsp) {
         *(bt_tx_data + packet_length++) = CHARGE_STATUS_LED_RESPONSE;
         *(bt_tx_data + packet_length++) = stat.battStat;
         blinkLedBtRsp = 0;
//      } else if (bufferSizeResponse) {
//         *(bt_tx_data + packet_length++) = BUFFER_SIZE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_BUFFER_SIZE];
//         bufferSizeResponse = 0;
//      } else if (uniqueSerialResponse) {
//         *(bt_tx_data + packet_length++) = UNIQUE_SERIAL_RESPONSE;
//         memcpy((bt_tx_data+packet_length), dierecord, 8);
//         packet_length += 8;
//         uniqueSerialResponse = 0;
//      } else if (exgRegsResponse) {
//         *(bt_tx_data + packet_length++) = EXG_REGS_RESPONSE;
//         *(bt_tx_data + packet_length++) = exgLength;
//         if(exgLength) {
//            if(exgChip)
//               memcpy((bt_tx_data+packet_length), (storedConfig+NV_EXG_ADS1292R_2_CONFIG1+exgStartAddr), exgLength);
//            else
//               memcpy((bt_tx_data+packet_length), (storedConfig+NV_EXG_ADS1292R_1_CONFIG1+exgStartAddr), exgLength);
//            packet_length += exgLength;
//         }
//         exgRegsResponse = 0;
      } else if (dcIdBtRsp) {
         *(bt_tx_data + packet_length++) = DAUGHTER_CARD_ID_RESPONSE;
         *(bt_tx_data + packet_length++) = btDcMemLength;
         CAT24C16_read(btDcMemOffset, bt_tx_data + packet_length, btDcMemLength);
         packet_length += btDcMemLength;
         dcIdBtRsp = 0;
      } else if (dcMemBtRsp) {
         *(bt_tx_data + packet_length++) = DAUGHTER_CARD_MEM_RESPONSE;
         *(bt_tx_data + packet_length++) = btDcMemLength;
         CAT24C16_read(btDcMemOffset + 16, bt_tx_data + packet_length, btDcMemLength);
         packet_length += btDcMemLength;
         dcMemBtRsp = 0;
//      } else if (btCommsBaudRateResponse) {
//         *(bt_tx_data + packet_length++) = BT_COMMS_BAUD_RATE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_BT_COMMS_BAUD_RATE];
//         btCommsBaudRateResponse = 0;
      } else if (infomemBtRsp) {
         *(bt_tx_data + packet_length++) = INFOMEM_RESPONSE;
         *(bt_tx_data + packet_length++) = btInfomemLength;
         //memcpy((bt_tx_data + packet_length), &storedConfig[btInfomemOffset], btInfomemLength);
         S4Ram_storedConfigGet(bt_tx_data + packet_length, btInfomemOffset, btInfomemLength);
         packet_length += btInfomemLength;
         infomemBtRsp = 0;
//      } else if (derivedChannelResponse) {
//         *(bt_tx_data + packet_length++) = DERIVED_CHANNEL_BYTES_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_DERIVED_CHANNELS_0];
//         *(bt_tx_data + packet_length++) = storedConfig[NV_DERIVED_CHANNELS_1];
//         *(bt_tx_data + packet_length++) = storedConfig[NV_DERIVED_CHANNELS_2];
//         derivedChannelResponse = 0;
      } else if(rwcResponse){
         uint64_t temp_rtcCurrentTime = RTC_get64();
         *(bt_tx_data + packet_length++) = RWC_RESPONSE;
         memcpy(bt_tx_data + packet_length, (uint8_t*)(&temp_rtcCurrentTime), 8);
         packet_length += 8;
         rwcResponse = 0;
      }


      /*if(crcChecksum){
       uint16_t crc_value;
       crc_value = CRC_data(bt_tx_data, (uint8_t)packet_length);
       *(bt_tx_data+packet_length++) = crc_value & 0xFF;
       *(bt_tx_data+packet_length++) = (crc_value & 0xFF00) >> 8;
      }*/
      BT_write(bt_tx_data, packet_length);
   }
#endif
}
#endif

/*****************************************************
 *
 *  dock uart
 *
 *****************************************************/

void DockUart_init(void){
   uartSteps = 0;
   uartArgSize = 0;
   uartArg2Wait = 0;
   uartCrc2Wait = 0;

   uartSendRspAck = 0;
   uartSendRspBadCmd = 0;
   uartSendRspBadArg = 0;
   uartSendRspBadCrc = 0;

   uartSendRspMac = 0;
   uartSendRspVer = 0;
   uartSendRspBat = 0;
   uartSendRspRtcConfigTime = 0;
   uartSendRspCurrentTime = 0;

   uartSendRspGdi = 0;
   uartSendRspGdm = 0;
   uartSendRspGim = 0;
   uartSendRspCalibDump = 0;
   uartSendRspBtVer = 0;

   uartTimeStart = uartTimeEnd = 0;

   Uart_init();

   HAL_UART_DeInit(huartDock);
   huartDock->Init.WordLength = UART_WORDLENGTH_8B;
   huartDock->Init.StopBits = UART_STOPBITS_1;
   huartDock->Init.Parity = UART_PARITY_NONE;
   huartDock->Init.Mode = UART_MODE_TX_RX;
   huartDock->Init.HwFlowCtl = UART_HWCONTROL_NONE;
   huartDock->Init.OverSampling = UART_OVERSAMPLING_16;
   HAL_UART_Init(huartDock);

   HAL_UART_Receive_IT(huartDock, uartDockRxBuf, 1);

#if defined(SHIMMER3R)
   HAL_UART_RegisterCallback(huartDock, HAL_UART_RX_COMPLETE_CB_ID, dockUartRxCallback);
//   HAL_UART_RegisterCallback(huartDock, HAL_UART_TX_COMPLETE_CB_ID, btUartTxCpltCallback);
#endif
}

void DockUart_disable(void){
   __HAL_UART_DISABLE(huartDock);
}
void DockUart_enable(void){
   __HAL_UART_ENABLE(huartDock);
}

void DockUart_rxCallback(uint8_t data) {
   //uint8_t data = uartRxData[0];
   uint64_t uart_time = RTC_get64();
   if(uartTimeStart){
      if(uart_time - uartTimeStart > 3276){
         uartSteps = 0;
      }
   }
   if (uartSteps) { //wait for: cmd, len, data, crc -> process
      if (uartSteps == UART_STEP_WAIT4_CMD) {
         uartAction = data;
         uartArgSize = UART_RXBUF_CMD;
         dockRxBuf[uartArgSize++] = data;
         switch (uartAction) {
         case UART_SET:
         case UART_GET:
            uartSteps = UART_STEP_WAIT4_LEN;
            break;
         default:
            uartSteps = 0;
            uartSendRspBadCmd = 1;
            //uartSendResponses = 1;
            S4_Task_set(TASK_UARTRESPONSE);
         }
      }
      else if (uartSteps == UART_STEP_WAIT4_LEN) {
         uartSteps =  UART_STEP_WAIT4_DATA;
         uartArgSize = UART_RXBUF_LEN;
         dockRxBuf[uartArgSize++] = data;
         uartArg2Wait = data;
      }
      else if (uartSteps == UART_STEP_WAIT4_DATA) {
         dockRxBuf[uartArgSize++] = data;
         if (!--uartArg2Wait) {
            uartCrc2Wait = 2;
            uartSteps = UART_STEP_WAIT4_CRC;
         }
      }
      else if (uartSteps == UART_STEP_WAIT4_CRC) {
         dockRxBuf[uartArgSize++] = data;
         if (!--uartCrc2Wait) {
            uartSteps = 0;
            uartArgSize = 0;
            //uartProcessCmds = 1;
            S4_Task_set(TASK_UARTPROCESS);
            uartTimeStart = 0;
         }
      }
      else {
         uartSteps = 0;
         uartTimeStart = 0;
      }
   }
   else {
      if (data == '$') {
         uartAction = 0;
         uartArgSize = UART_RXBUF_START;
         dockRxBuf[UART_RXBUF_START] = '$';
         uartSteps = UART_STEP_WAIT4_CMD;
         uartTimeStart = uart_time;
      }
   }
}
void DockUart_processCmd() {
   HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_6);//green
   if (uartAction) {
      if (S4Calc_crcCheck(dockRxBuf, dockRxBuf[UART_RXBUF_LEN] + 5)) {
         if (uartAction == UART_GET) { // get
            if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_SHIMMER) { // get shimmer
               switch (dockRxBuf[UART_RXBUF_PROP]) {
               case UART_PROP_MAC:
                  if (dockRxBuf[UART_RXBUF_LEN] == 2)
                     uartSendRspMac = 1;
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_VER:
                  if (dockRxBuf[UART_RXBUF_LEN] == 2)
                     uartSendRspVer = 1;
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_RTC_CFG_TIME:
                  if (dockRxBuf[UART_RXBUF_LEN] == 2)
                     uartSendRspRtcConfigTime = 1;
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_CURR_LOCAL_TIME:
                  if (dockRxBuf[UART_RXBUF_LEN] == 2)
                     uartSendRspCurrentTime = 1;
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_INFOMEM:
                  uartInfoMemLength = dockRxBuf[UART_RXBUF_DATA];
                  uartInfoMemOffset = (uint16_t)dockRxBuf[UART_RXBUF_DATA + 1] + (((uint16_t)dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
//                  if ((uartInfoMemLength <= 128) && (uartInfoMemOffset <= 0x1ff) && (uartInfoMemLength + uartInfoMemOffset <= 0x200))
                  if ((uartInfoMemLength <= 0x80) && (uartInfoMemOffset <= 0x01ff) && (uartInfoMemLength + uartInfoMemOffset <= 0x0200))
                     uartSendRspGim = 1;
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_CALIB_DUMP:
                  uartCalibRamLength = dockRxBuf[UART_RXBUF_DATA];
                  uartCalibRamOffset = (uint16_t)dockRxBuf[UART_RXBUF_DATA + 1] + (((uint16_t)dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
                  if ((uartCalibRamLength <= 128) && (uartCalibRamOffset <= SHIMMER_CALIB_RAM_MAX-1) && (uartCalibRamLength + uartCalibRamOffset <= SHIMMER_CALIB_RAM_MAX))
                     uartSendRspCalibDump = 1;
                  else
                     uartSendRspBadArg = 1;
                  break;
               default:
                  uartSendRspBadCmd = 1;
                  break;
               }
            } else if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_BAT) { // get battery
               switch (dockRxBuf[UART_RXBUF_PROP]) {
               case UART_PROP_VALUE:
                  if (dockRxBuf[UART_RXBUF_LEN] == 2)
                     uartSendRspBat = 1; // already in the callback function
                  else
                     uartSendRspBadArg = 1;
                  break;
               default:
                  uartSendRspBadCmd = 1;
                  break;
               }
            }
            else if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_DAUGHTER_CARD) { // get daughter card
               switch (dockRxBuf[UART_RXBUF_PROP]) {
               case UART_PROP_CARD_ID:
                  uartDcMemLength = dockRxBuf[UART_RXBUF_DATA];
                  uartDcMemOffset = (uint16_t)dockRxBuf[UART_RXBUF_DATA + 1];
                  if ((uartDcMemLength <= 16) && (uartDcMemOffset <= 15) && ((uint16_t)uartDcMemLength + uartDcMemOffset <= 16)) {
                     uartSendRspGdi = 1;
                  }
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_CARD_MEM:
                  uartDcMemLength = dockRxBuf[UART_RXBUF_DATA];
                  uartDcMemOffset = (uint16_t)dockRxBuf[UART_RXBUF_DATA + 1] + (((uint16_t)dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
                  if ((uartDcMemLength <= 128) && (uartDcMemOffset <= 2031) && ((uint16_t)uartDcMemLength + uartDcMemOffset <= 2032)) {
                     uartSendRspGdm = 1;
                  }
                  else
                     uartSendRspBadArg = 1;
                  break;
               default:
                  uartSendRspBadCmd = 1;
                  break;
               }
            }
            else if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_BT)
            {
                switch (dockRxBuf[UART_RXBUF_PROP])
                {
                case UART_PROP_VER:
                    if ((dockRxBuf[UART_RXBUF_LEN] == 2))
                        uartSendRspBtVer = 1;
                    else
                        uartSendRspBadArg = 1;
                    break;
                default:
                    uartSendRspBadCmd = 1;
                    break;
                }
            }
            else {
               uartSendRspBadCmd = 1;
            }
         }
         else if (uartAction == UART_SET) { // set
            if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_SHIMMER) { // set shimmer
               switch (dockRxBuf[UART_RXBUF_PROP]) {
               case UART_PROP_RTC_CFG_TIME:
                  if (dockRxBuf[UART_RXBUF_LEN] == 10) {
                     uint64_t temp64;
                     memcpy((uint8_t*)(&temp64), dockRxBuf+UART_RXBUF_DATA, 8);// 64bits = 8bytes
                     RTC_init(temp64);
                     S4Ram_getStoredConfig()->rtcSetByBt = 1;
                     InfoMem_update();
                     //sdHeadText[SDH_TRIAL_CONFIG0] = storedConfig[NV_SD_TRIAL_CONFIG0];
                     uartSendRspAck = 1;
                     enableRTCAlarm(&hrtc); //configure Alarm after time set from dock
                  }
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_INFOMEM:
                  uartInfoMemLength = dockRxBuf[UART_RXBUF_DATA];
                  uartInfoMemOffset = (uint16_t)dockRxBuf[UART_RXBUF_DATA + 1] + (((uint16_t)dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
                  if ((uartInfoMemLength <= 0x80) && (uartInfoMemOffset <= 0x01ff) && (uartInfoMemLength + uartInfoMemOffset <= 0x0200)) {
                     uint8_t temp_btMacHex[6];
                     S4Ram_storedConfigGet(temp_btMacHex, NV_MAC_ADDRESS, 6);
                     S4Ram_storedConfigSet(dockRxBuf + UART_RXBUF_DATA + 3, uartInfoMemOffset, uartInfoMemLength);
                     S4Ram_storedConfigSet(temp_btMacHex, NV_MAC_ADDRESS, 6);
                     InfoMem_update();
                     uartSendRspAck = 1;
                  }
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_CALIB_DUMP:
                  uartCalibRamLength = dockRxBuf[UART_RXBUF_DATA];
                  uartCalibRamOffset = (uint16_t)dockRxBuf[UART_RXBUF_DATA + 1] + (((uint16_t)dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
                  if ((uartCalibRamLength <= 128) && (uartCalibRamOffset <= SHIMMER_CALIB_RAM_MAX-1) && (uartCalibRamLength + uartCalibRamOffset <= SHIMMER_CALIB_RAM_MAX)){
                     if(ShimmerCalib_ramWrite(dockRxBuf+UART_RXBUF_DATA+3, uartCalibRamLength, uartCalibRamOffset) == 1){
                     }
                     uartSendRspAck = 1;
                  }
                  else
                     uartSendRspBadArg = 1;
                  break;
               default:
                  uartSendRspBadCmd = 1;
                  break;
               }
            }
            else if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_DAUGHTER_CARD) { // set daughter card
               switch (dockRxBuf[UART_RXBUF_PROP]) {
               case UART_PROP_CARD_ID:
                  uartDcMemLength = dockRxBuf[UART_RXBUF_DATA];
                  uartDcMemOffset = (uint16_t)dockRxBuf[UART_RXBUF_DATA + 1];
                  if ((uartDcMemLength <= 16) && (uartDcMemOffset <= 15) && ((uint16_t)uartDcMemLength + uartDcMemOffset <= 16)) {
                     CAT24C16_write(uartDcMemOffset, dockRxBuf + UART_RXBUF_DATA + 2, uartDcMemLength);
                     uartSendRspAck = 1;
                  }
                  else
                     uartSendRspBadArg = 1;
                  break;
               case UART_PROP_CARD_MEM:
                  uartDcMemLength = dockRxBuf[UART_RXBUF_DATA];
                  uartDcMemOffset = (uint16_t)dockRxBuf[UART_RXBUF_DATA + 1] + (((uint16_t)dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
                  if ((uartDcMemLength <= 128) && (uartDcMemOffset <= 2031) && ((uint16_t)uartDcMemLength + uartDcMemOffset <= 2032)) {
                     CAT24C16_write(uartDcMemOffset + 16, dockRxBuf + UART_RXBUF_DATA + 3, uartDcMemLength);
                     uartSendRspAck = 1;
                  }
                  else
                     uartSendRspBadArg = 1;
                  break;
               default:
                  uartSendRspBadCmd = 1;
                  break;
               }
            }
            else {
               uartSendRspBadCmd = 1;
            }
         }
         else {
            uartSendRspBadCmd = 1;
         }
      }
      else
         uartSendRspBadCrc = 1;
      //uartSendResponses = 1;
      S4_Task_set(TASK_UARTRESPONSE);
   }
}

void DockUart_sendRsp() {
   uint8_t uart_resp_len = 0;
   uint16_t uartRespCrc;

   if (uartSendRspAck) {
      uartSendRspAck = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_ACK_RESPONSE;
   } else if (uartSendRspBadCmd) {
      uartSendRspBadCmd = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_BAD_CMD_RESPONSE;
   } else if (uartSendRspBadArg) {
      uartSendRspBadArg = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_BAD_ARG_RESPONSE;
   } else if (uartSendRspBadCrc) {
      uartSendRspBadCrc = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_BAD_CRC_RESPONSE;
   } else if (uartSendRspMac) {
      uartSendRspMac = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = 8;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_MAC;
      //memcpy(uartRespBuf + uart_resp_len, btMacHex, 6);
      S4Ram_btMacHexGet(uartRespBuf + uart_resp_len);
      uart_resp_len += 6;
   } else if (uartSendRspVer) {
      uartSendRspVer = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = 9;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_VER;
      *(uartRespBuf + uart_resp_len++) = DEVICE_VER;
      *(uartRespBuf + uart_resp_len++) = (FW_IDENTIFIER & 0xFF);
      *(uartRespBuf + uart_resp_len++) = ((FW_IDENTIFIER & 0xFF00) >> 8);
      *(uartRespBuf + uart_resp_len++) = (FW_VER_MAJOR & 0xFF);
      *(uartRespBuf + uart_resp_len++) = ((FW_VER_MAJOR & 0xFF00) >> 8);
      *(uartRespBuf + uart_resp_len++) = (FW_VER_MINOR);
      *(uartRespBuf + uart_resp_len++) = (FW_VER_REL);
   } else if(uartSendRspBat){
      uartSendRspBat = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = 5;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_BAT;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_VALUE;
      memcpy(uartRespBuf + uart_resp_len, (uint8_t*)stat.battVal, 3);
      uart_resp_len+=3;
   } else if (uartSendRspRtcConfigTime) {
      uartSendRspRtcConfigTime = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = 10;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_RTC_CFG_TIME;
      uint64_t temp_rtcConfigTime = S4_RWC_getConfigTime();
      memcpy(uartRespBuf + uart_resp_len, (uint8_t*)(&temp_rtcConfigTime), 8);
      //memset(uartRespBuf + uart_resp_len, 0, 8);
      uart_resp_len += 8;
   } else if (uartSendRspCurrentTime) {
      uartSendRspCurrentTime = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = 10;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_CURR_LOCAL_TIME;
      uint64_t temp_rtcCurrentTime = RTC_get64();
      memcpy(uartRespBuf + uart_resp_len, (uint8_t*)(&temp_rtcCurrentTime), 8);
      uart_resp_len += 8;
   } else if (uartSendRspGdi) {
      uartSendRspGdi = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = uartDcMemLength + 2;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_DAUGHTER_CARD;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_CARD_ID;
      if ((uartDcMemLength + uart_resp_len) < UART_RSP_PACKET_SIZE) {
#if IS_CONNECTED_EEPROM
         CAT24C16_read(uartDcMemOffset, uartRespBuf + uart_resp_len, uartDcMemLength);
#else
         uartRespBuf[uart_resp_len+0] = EXP_BRD_ID_MAJOR;
         uartRespBuf[uart_resp_len+1] = EXP_BRD_ID_MINOR;
         uartRespBuf[uart_resp_len+2] = EXP_BRD_ID_INTERNAL;
#endif
      }
      uart_resp_len += uartDcMemLength;
   } else if (uartSendRspGdm) {
      uartSendRspGdm = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = uartDcMemLength + 2;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_DAUGHTER_CARD;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_CARD_MEM;
      if ((uartDcMemLength + uart_resp_len) < UART_RSP_PACKET_SIZE) {
#if IS_CONNECTED_EEPROM
         CAT24C16_read(uartDcMemOffset + 16, uartRespBuf + uart_resp_len, uartDcMemLength);
#endif
      }
      uart_resp_len += uartDcMemLength;
   } else if (uartSendRspGim) {
      uartSendRspGim = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = uartInfoMemLength + 2;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_INFOMEM;
      if ((uartDcMemLength + uart_resp_len) < UART_RSP_PACKET_SIZE) {
         InfoMem_readRam(uartRespBuf + uart_resp_len, uartInfoMemOffset, uartInfoMemLength);
      }
      uart_resp_len += uartInfoMemLength;
   } else if (uartSendRspCalibDump) {
      uartSendRspCalibDump = 0;
      *(uartRespBuf + uart_resp_len++) = '$';
      *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
      *(uartRespBuf + uart_resp_len++) = uartCalibRamLength + 2;
      *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
      *(uartRespBuf + uart_resp_len++) = UART_PROP_CALIB_DUMP;
      if ((uartCalibRamLength + uart_resp_len) < UART_RSP_PACKET_SIZE) {
         InfoMem_readCalib(uartRespBuf + uart_resp_len, uartCalibRamOffset, uartCalibRamLength);
      }
      uart_resp_len += uartCalibRamLength;
   }
   else if (uartSendRspBtVer)
   {
       uartSendRspBtVer = 0;
       uint8_t btVerStrLen = getBtVerStrLen();
       *(uartRespBuf + uart_resp_len++) = '$';
       *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
       *(uartRespBuf + uart_resp_len++) = 2U + btVerStrLen;
       *(uartRespBuf + uart_resp_len++) = UART_COMP_BT;
       *(uartRespBuf + uart_resp_len++) = UART_PROP_VER;

       memcpy(uartRespBuf + uart_resp_len, getBtVerStrPtr(), btVerStrLen);
       uart_resp_len += btVerStrLen;
   }

   uartRespCrc = S4Calc_crcCalc(uartRespBuf, uart_resp_len);
   *(uartRespBuf + uart_resp_len++) = uartRespCrc & 0xff;
   *(uartRespBuf + uart_resp_len++) = (uartRespCrc & 0xff00) >> 8;

   //HAL_UART_Transmit_IT(&huart6, uartRespBuf, uart_resp_len);
   // Takes ~1.2ms to transmit 135 bytes @ 115200 baud therefore setting timeout to be > ~38.4ms
   HAL_UART_Transmit(huartDock, uartRespBuf, uart_resp_len, 100);
}

#if defined(SHIMMER4_SDK)
/*****************************************************
 *
 *  Expansion uart
 *
 *****************************************************/
void ExpUart_init(void){
   Uart_init();
}

void ExpUart_rxCallback(uint8_t data){
}

uint8_t ExpUart_TxIT(uint8_t *pData, uint16_t Size){
//   if(!stat.isDocked){
//      if(HAL_OK != HAL_UART_Transmit_IT(huartExp, pData, Size))
//         return 1;//fail
//   }
//   else{//docked
//      return 1;
//   }
   HAL_UART_Transmit_IT(huartExp, pData, Size);
   return 0;
}
#endif

uint8_t BtUart_connectIntCheck(void) {
   if (HAL_GPIO_ReadPin(BT_CONNECTION_GPIO_Port, BT_CONNECTION_Pin) == GPIO_PIN_SET) { //connected
      //HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, GPIO_PIN_RESET);//blue
      BT_connectionInterrupt(1);
      stat.isBtConnected = 1;
      Board_ledOn(LED_BLUE);
   } else {
      //HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, GPIO_PIN_SET);//blue
      BT_connectionInterrupt(0);
      stat.isBtConnected = 0;
      S4_Task_set(TASK_STOPSENSING);
      Board_ledOff(LED_BLUE);
   }
   return stat.isBtConnected;
}


//void BtUart_rtsIntCheck(void) {
//   BT_rtsInterrupt(HAL_GPIO_ReadPin(BTH_RTS_GPIO_Port, BTH_RTS_Pin));
//}

uint8_t DockUart_interruptCheck(void) {
#if TEST_UNDOCKED
   if ( 0 ){
#else
   if (HAL_GPIO_ReadPin(DOCK_DETECT_GPIO_Port, DOCK_DETECT_Pin) == GPIO_PIN_SET) { //docked
#endif
      stat.isDocked = 1;
      //Board_sd2Pc();
      //Board_ledOn(LED_GREEN0);
   } else {
      stat.isDocked = 0;
      //Board_sd2Arm();
      //SD_mount(1);
      //Board_ledOff(LED_GREEN0);
   }
   S4_Task_set(TASK_DOCKSETUP);
   return stat.isDocked;
}


void DockUart_setup(void) {
   if (stat.isDocked) {
      Board_sd2Pc();
   } else {
      Board_sd2Arm();
   }
}

//HAL_StatusTypeDef BtUart_Transmit_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
//{
//   PeriStat_Set(STAT_PERI_BT);
//   return HAL_UART_Transmit_DMA(huart, pData, Size);
//}

#if defined(SHIMMER4_SDK)
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
//#if UART_DOCK0BT1
   if (huart->Instance == huartBt->Instance) {
      BT_txIsr(huart);
   }
//#else
   else if (huart->Instance == huartDock->Instance) {
      //HAL_GPIO_TogglePin(GPIOK, GPIO_PIN_3);//blue
   }
//#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//#if UART_DOCK0BT1
   __NOP();
   if (huart->Instance == huartBt->Instance) {
      BT_rxIsr(huart);
   }
//#else
   else if (huart->Instance == huartDock->Instance) {
     dockUartRxCallback(huart);
   }
//#endif
}
#endif

void dockUartRxCallback(UART_HandleTypeDef *huart)
{
  //Board_ledToggle(LED_YELLOW);
  if (stat.isDocked) {
     DockUart_rxCallback(uartDockRxBuf[0]);
//      } else {
//         ExpUart_rxCallback(uartDockRxBuf[0]);
  }
//  HAL_UART_Receive_DMA(huartDock, uartDockRxBuf, 1);
  HAL_UART_Receive_IT(huartDock, uartDockRxBuf, 1);
}

/* USER CODE END 1 */
