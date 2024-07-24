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

//DOCK_UART variables

UART_HandleTypeDef *huartBt;
UART_HandleTypeDef *huartDock;

uint8_t uartSteps, uartArgSize, uartArg2Wait, uartCrc2Wait,
    uartAction; //uartProcessCmds, uartSendResponses,
uint8_t uartDockRxBuf[2], dockRxBuf[UART_DATA_LEN_MAX], uartRespBuf[UART_RSP_PACKET_SIZE];
uint8_t uartSendRspMac, uartSendRspVer, uartSendRspBat,
    uartSendRspRtcConfigTime, uartSendRspCurrentTime, uartSendRspGdi,
    uartSendRspGdm, uartSendRspGim, uartSendRspCalibDump, uartSendRspBtVer,
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

  // Control Return if peripheral is already initialised
  if (!stat.isDocked || isDockUartInitialised())
  {
    return;
  }

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
  if (HAL_UARTEx_EnableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  DockUart_init(&huart1);
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

void setUartPeripheralPointers(void)
{
  huartBt = &huart3;
#if defined(SHIMMER3R)
  huartDock = &huart1;
#elif defined(SHIMMER4_SDK)
  huartDock = &huart6;
#endif
  //huartExp = &huart6;

  //pStat = GetStatus();
  //pSensing = S4Sens_getSensing();
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
  huartBt->Init.HwFlowCtl = hwFlowCtrl ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
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

/*****************************************************
 *
 *  dock uart
 *
 *****************************************************/

void DockUart_init(UART_HandleTypeDef *huart)
{
  huartDock = huart;

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

#if defined(SHIMMER3R)
  HAL_UART_RegisterCallback(huartDock, HAL_UART_RX_COMPLETE_CB_ID, dockUartRxCallback);
//HAL_UART_RegisterCallback(huartDock, HAL_UART_TX_COMPLETE_CB_ID, btUartTxCpltCallback);
#endif

  HAL_UART_Receive_IT(huartDock, uartDockRxBuf, 1);
}

void DockUart_deint(void)
{
  if (isDockUartInitialised())
  {
    HAL_UART_DeInit(huartDock);
  }
}

void DockUart_disable(void)
{
  if (isDockUartInitialised())
  {
    __HAL_UART_DISABLE(huartDock);
  }
}

void DockUart_enable(void)
{
  if (isDockUartInitialised())
  {
    __HAL_UART_ENABLE(huartDock);
  }
}

void DockUart_rxCallback(uint8_t data)
{
  //uint8_t data = uartRxData[0];
  uint64_t uart_time = RTC_get64();
  if (uartTimeStart)
  {
    if (uart_time - uartTimeStart > 3276)
    {
      uartSteps = 0;
    }
  }
  if (uartSteps)
  { //wait for: cmd, len, data, crc -> process
    if (uartSteps == UART_STEP_WAIT4_CMD)
    {
      uartAction = data;
      uartArgSize = UART_RXBUF_CMD;
      dockRxBuf[uartArgSize++] = data;
      switch (uartAction)
      {
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
    else if (uartSteps == UART_STEP_WAIT4_LEN)
    {
      uartSteps = UART_STEP_WAIT4_DATA;
      uartArgSize = UART_RXBUF_LEN;
      dockRxBuf[uartArgSize++] = data;
      uartArg2Wait = data;
    }
    else if (uartSteps == UART_STEP_WAIT4_DATA)
    {
      dockRxBuf[uartArgSize++] = data;
      if (!--uartArg2Wait)
      {
        uartCrc2Wait = 2;
        uartSteps = UART_STEP_WAIT4_CRC;
      }
    }
    else if (uartSteps == UART_STEP_WAIT4_CRC)
    {
      dockRxBuf[uartArgSize++] = data;
      if (!--uartCrc2Wait)
      {
        uartSteps = 0;
        uartArgSize = 0;
        //uartProcessCmds = 1;
        S4_Task_set(TASK_UARTPROCESS);
        uartTimeStart = 0;
      }
    }
    else
    {
      uartSteps = 0;
      uartTimeStart = 0;
    }
  }
  else
  {
    if (data == '$')
    {
      uartAction = 0;
      uartArgSize = UART_RXBUF_START;
      dockRxBuf[UART_RXBUF_START] = '$';
      uartSteps = UART_STEP_WAIT4_CMD;
      uartTimeStart = uart_time;
    }
  }
}

void DockUart_processCmd()
{
#if defined(SHIMMER4_SDK)
  HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_6); //green
#else
  Board_ledLwrToggleColourRgb(LED_RGB_GREEN);
#endif
  if (uartAction)
  {
    if (S4Calc_crcCheck(dockRxBuf, dockRxBuf[UART_RXBUF_LEN] + 5))
    {
      if (uartAction == UART_GET)
      { //get
        if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_SHIMMER)
        { //get shimmer
          switch (dockRxBuf[UART_RXBUF_PROP])
          {
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
            uartInfoMemOffset = (uint16_t) dockRxBuf[UART_RXBUF_DATA + 1]
                + (((uint16_t) dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
            //if ((uartInfoMemLength <= 128) && (uartInfoMemOffset <= 0x1ff) && (uartInfoMemLength + uartInfoMemOffset <= 0x200))
            if ((uartInfoMemLength <= 0x80) && (uartInfoMemOffset <= 0x01ff)
                && (uartInfoMemLength + uartInfoMemOffset <= 0x0200))
              uartSendRspGim = 1;
            else
              uartSendRspBadArg = 1;
            break;
          case UART_PROP_CALIB_DUMP:
            uartCalibRamLength = dockRxBuf[UART_RXBUF_DATA];
            uartCalibRamOffset = (uint16_t) dockRxBuf[UART_RXBUF_DATA + 1]
                + (((uint16_t) dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
            if ((uartCalibRamLength <= 128)
                && (uartCalibRamOffset <= SHIMMER_CALIB_RAM_MAX - 1)
                && (uartCalibRamLength + uartCalibRamOffset <= SHIMMER_CALIB_RAM_MAX))
              uartSendRspCalibDump = 1;
            else
              uartSendRspBadArg = 1;
            break;
          default:
            uartSendRspBadCmd = 1;
            break;
          }
        }
        else if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_BAT)
        { //get battery
          switch (dockRxBuf[UART_RXBUF_PROP])
          {
          case UART_PROP_VALUE:
            if (dockRxBuf[UART_RXBUF_LEN] == 2)
              uartSendRspBat = 1; //already in the callback function
            else
              uartSendRspBadArg = 1;
            break;
          default:
            uartSendRspBadCmd = 1;
            break;
          }
        }
        else if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_DAUGHTER_CARD)
        { //get daughter card
          switch (dockRxBuf[UART_RXBUF_PROP])
          {
          case UART_PROP_CARD_ID:
            uartDcMemLength = dockRxBuf[UART_RXBUF_DATA];
            uartDcMemOffset = (uint16_t) dockRxBuf[UART_RXBUF_DATA + 1];
            if ((uartDcMemLength <= 16) && (uartDcMemOffset <= 15)
                && ((uint16_t) uartDcMemLength + uartDcMemOffset <= 16))
            {
              uartSendRspGdi = 1;
            }
            else
              uartSendRspBadArg = 1;
            break;
          case UART_PROP_CARD_MEM:
            uartDcMemLength = dockRxBuf[UART_RXBUF_DATA];
            uartDcMemOffset = (uint16_t) dockRxBuf[UART_RXBUF_DATA + 1]
                + (((uint16_t) dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
            if ((uartDcMemLength <= 128) && (uartDcMemOffset <= 2031)
                && ((uint16_t) uartDcMemLength + uartDcMemOffset <= 2032))
            {
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
        else
        {
          uartSendRspBadCmd = 1;
        }
      }
      else if (uartAction == UART_SET)
      { //set
        if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_SHIMMER)
        { //set shimmer
          switch (dockRxBuf[UART_RXBUF_PROP])
          {
          case UART_PROP_RTC_CFG_TIME:
            if (dockRxBuf[UART_RXBUF_LEN] == 10)
            {
              uint64_t temp64;
              memcpy((uint8_t *) (&temp64), dockRxBuf + UART_RXBUF_DATA, 8); //64bits = 8bytes
              RTC_init(temp64);
              S4Ram_getStoredConfig()->rtcSetByBt = 1;
              InfoMem_update();
              //sdHeadText[SDH_TRIAL_CONFIG0] = storedConfig[NV_SD_TRIAL_CONFIG0];
              uartSendRspAck = 1;
            }
            else
              uartSendRspBadArg = 1;
            break;
          case UART_PROP_INFOMEM:
            uartInfoMemLength = dockRxBuf[UART_RXBUF_DATA];
            uartInfoMemOffset = (uint16_t) dockRxBuf[UART_RXBUF_DATA + 1]
                + (((uint16_t) dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
            if ((uartInfoMemLength <= 0x80) && (uartInfoMemOffset <= 0x01ff)
                && (uartInfoMemLength + uartInfoMemOffset <= 0x0200))
            {
              uint8_t temp_btMacHex[6];
              S4Ram_storedConfigGet(temp_btMacHex, NV_MAC_ADDRESS, 6);
              S4Ram_storedConfigSet(dockRxBuf + UART_RXBUF_DATA + 3,
                  uartInfoMemOffset, uartInfoMemLength);
              S4Ram_storedConfigSet(temp_btMacHex, NV_MAC_ADDRESS, 6);
              InfoMem_update();
              uartSendRspAck = 1;
            }
            else
              uartSendRspBadArg = 1;
            break;
          case UART_PROP_CALIB_DUMP:
            uartCalibRamLength = dockRxBuf[UART_RXBUF_DATA];
            uartCalibRamOffset = (uint16_t) dockRxBuf[UART_RXBUF_DATA + 1]
                + (((uint16_t) dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
            if ((uartCalibRamLength <= 128)
                && (uartCalibRamOffset <= SHIMMER_CALIB_RAM_MAX - 1)
                && (uartCalibRamLength + uartCalibRamOffset <= SHIMMER_CALIB_RAM_MAX))
            {
              if (ShimmerCalib_ramWrite(dockRxBuf + UART_RXBUF_DATA + 3,
                      uartCalibRamLength, uartCalibRamOffset)
                  == 1)
              {
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
        else if (dockRxBuf[UART_RXBUF_COMP] == UART_COMP_DAUGHTER_CARD)
        { //set daughter card
          switch (dockRxBuf[UART_RXBUF_PROP])
          {
          case UART_PROP_CARD_ID:
            uartDcMemLength = dockRxBuf[UART_RXBUF_DATA];
            uartDcMemOffset = (uint16_t) dockRxBuf[UART_RXBUF_DATA + 1];
            if ((uartDcMemLength <= 16) && (uartDcMemOffset <= 15)
                && ((uint16_t) uartDcMemLength + uartDcMemOffset <= 16))
            {

              //Write (up to) 16 bytes to eeprom
              eepromWrite(uartDcMemOffset, (uint16_t) uartDcMemLength,
                  dockRxBuf + UART_RXBUF_DATA + 2U);
              //Copy new bytes to active daughter card byte array
              memcpy(getDaughtCardId() + ((uint8_t) uartDcMemOffset),
                  dockRxBuf + UART_RXBUF_DATA + 2, uartDcMemLength);

              uartSendRspAck = 1;
            }
            else
              uartSendRspBadArg = 1;
            break;
          case UART_PROP_CARD_MEM:
            uartDcMemLength = dockRxBuf[UART_RXBUF_DATA];
            uartDcMemOffset = (uint16_t) dockRxBuf[UART_RXBUF_DATA + 1]
                + (((uint16_t) dockRxBuf[UART_RXBUF_DATA + 2]) << 8);
            if ((uartDcMemLength <= 128) && (uartDcMemOffset <= 2031)
                && ((uint16_t) uartDcMemLength + uartDcMemOffset <= 2032))
            {

              eepromWrite(uartDcMemOffset + 16U, (uint16_t) uartDcMemLength,
                  dockRxBuf + UART_RXBUF_DATA + 3U);

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
        else
        {
          uartSendRspBadCmd = 1;
        }
      }
      else
      {
        uartSendRspBadCmd = 1;
      }
    }
    else
      uartSendRspBadCrc = 1;
    //uartSendResponses = 1;
    S4_Task_set(TASK_UARTRESPONSE);
  }
}

void DockUart_sendRsp()
{
  uint8_t uart_resp_len = 0;
  uint16_t uartRespCrc;

  if (uartSendRspAck)
  {
    uartSendRspAck = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_ACK_RESPONSE;
  }
  else if (uartSendRspBadCmd)
  {
    uartSendRspBadCmd = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_BAD_CMD_RESPONSE;
  }
  else if (uartSendRspBadArg)
  {
    uartSendRspBadArg = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_BAD_ARG_RESPONSE;
  }
  else if (uartSendRspBadCrc)
  {
    uartSendRspBadCrc = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_BAD_CRC_RESPONSE;
  }
  else if (uartSendRspMac)
  {
    uartSendRspMac = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
    *(uartRespBuf + uart_resp_len++) = 8;
    *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
    *(uartRespBuf + uart_resp_len++) = UART_PROP_MAC;
    //memcpy(uartRespBuf + uart_resp_len, btMacHex, 6);
    S4Ram_btMacHexGet(uartRespBuf + uart_resp_len);
    uart_resp_len += 6;
  }
  else if (uartSendRspVer)
  {
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
  }
  else if (uartSendRspBat)
  {
    uartSendRspBat = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
    *(uartRespBuf + uart_resp_len++) = 5;
    *(uartRespBuf + uart_resp_len++) = UART_COMP_BAT;
    *(uartRespBuf + uart_resp_len++) = UART_PROP_VALUE;
    memcpy(uartRespBuf + uart_resp_len, (uint8_t *) stat.battVal, 3);
    uart_resp_len += 3;
  }
  else if (uartSendRspRtcConfigTime)
  {
    uartSendRspRtcConfigTime = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
    *(uartRespBuf + uart_resp_len++) = 10;
    *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
    *(uartRespBuf + uart_resp_len++) = UART_PROP_RTC_CFG_TIME;
    uint64_t temp_rtcConfigTime = S4_RWC_getConfigTime();
    memcpy(uartRespBuf + uart_resp_len, (uint8_t *) (&temp_rtcConfigTime), 8);
    //memset(uartRespBuf + uart_resp_len, 0, 8);
    uart_resp_len += 8;
  }
  else if (uartSendRspCurrentTime)
  {
    uartSendRspCurrentTime = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
    *(uartRespBuf + uart_resp_len++) = 10;
    *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
    *(uartRespBuf + uart_resp_len++) = UART_PROP_CURR_LOCAL_TIME;
    uint64_t temp_rtcCurrentTime = RTC_get64();
    memcpy(uartRespBuf + uart_resp_len, (uint8_t *) (&temp_rtcCurrentTime), 8);
    uart_resp_len += 8;
  }
  else if (uartSendRspGdi)
  {
    uartSendRspGdi = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
    *(uartRespBuf + uart_resp_len++) = uartDcMemLength + 2;
    *(uartRespBuf + uart_resp_len++) = UART_COMP_DAUGHTER_CARD;
    *(uartRespBuf + uart_resp_len++) = UART_PROP_CARD_ID;
    if ((uartDcMemLength + uart_resp_len) < UART_RSP_PACKET_SIZE)
    {
      eepromRead(uartDcMemOffset, (uint16_t) uartDcMemLength, uartRespBuf + uart_resp_len);
    }
    uart_resp_len += uartDcMemLength;
  }
  else if (uartSendRspGdm)
  {
    uartSendRspGdm = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
    *(uartRespBuf + uart_resp_len++) = uartDcMemLength + 2;
    *(uartRespBuf + uart_resp_len++) = UART_COMP_DAUGHTER_CARD;
    *(uartRespBuf + uart_resp_len++) = UART_PROP_CARD_MEM;
    if ((uartDcMemLength + uart_resp_len) < UART_RSP_PACKET_SIZE)
    {
      eepromRead(uartDcMemOffset + 16, (uint16_t) uartDcMemLength, uartRespBuf + uart_resp_len);
    }
    uart_resp_len += uartDcMemLength;
  }
  else if (uartSendRspGim)
  {
    uartSendRspGim = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
    *(uartRespBuf + uart_resp_len++) = uartInfoMemLength + 2;
    *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
    *(uartRespBuf + uart_resp_len++) = UART_PROP_INFOMEM;
    if ((uartDcMemLength + uart_resp_len) < UART_RSP_PACKET_SIZE)
    {
      InfoMem_readRam(uartRespBuf + uart_resp_len, uartInfoMemOffset, uartInfoMemLength);
    }
    uart_resp_len += uartInfoMemLength;
  }
  else if (uartSendRspCalibDump)
  {
    uartSendRspCalibDump = 0;
    *(uartRespBuf + uart_resp_len++) = '$';
    *(uartRespBuf + uart_resp_len++) = UART_RESPONSE;
    *(uartRespBuf + uart_resp_len++) = uartCalibRamLength + 2;
    *(uartRespBuf + uart_resp_len++) = UART_COMP_SHIMMER;
    *(uartRespBuf + uart_resp_len++) = UART_PROP_CALIB_DUMP;
    if ((uartCalibRamLength + uart_resp_len) < UART_RSP_PACKET_SIZE)
    {
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

uint8_t isDockUartInitialised(void)
{
  return (huartDock != 0 && ((HAL_UART_GetState(huartDock) & 0x20) == 0x20));
}

#if defined(SHIMMER4_SDK)
/*****************************************************
 *
 *  Expansion uart
 *
 *****************************************************/
void ExpUart_init(void)
{
  setUartPeripheralPointers();
}

void ExpUart_rxCallback(uint8_t data)
{
}

uint8_t ExpUart_TxIT(uint8_t *pData, uint16_t Size)
{
  //if(!stat.isDocked){
  //   if(HAL_OK != HAL_UART_Transmit_IT(huartExp, pData, Size))
  //      return 1;//fail
  //}
  //else{//docked
  //   return 1;
  //}
  HAL_UART_Transmit_IT(huartExp, pData, Size);
  return 0;
}
#endif

uint8_t BtUart_connectIntCheck(void)
{
  if (HAL_GPIO_ReadPin(BT_CONNECTION_GPIO_Port, BT_CONNECTION_Pin) == GPIO_PIN_SET)
  { //connected
    //HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, GPIO_PIN_RESET);//blue
    BT_connectionInterrupt(1);
    stat.isBtConnected = 1;
    Board_ledOn(LED_BLUE);
  }
  else
  {
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

uint8_t DockUart_interruptCheck(void)
{
#if TEST_UNDOCKED
  if (0)
  {
#else
  if (HAL_GPIO_ReadPin(DOCK_DETECT_GPIO_Port, DOCK_DETECT_Pin) == GPIO_PIN_SET)
  { //docked
#endif
    stat.isDocked = 1;
    //Board_sd2Pc();
    //Board_ledOn(LED_GREEN0);
  }
  else
  {
    stat.isDocked = 0;
    //Board_sd2Arm();
    //SD_mount(1);
    //Board_ledOff(LED_GREEN0);
  }
  return stat.isDocked;
}

void DockUart_setup(void)
{
  if (stat.isDocked)
  {
    Board_sd2Pc();
  }
  else
  {
    Board_sd2Arm();
  }
  SetupDock();
}

//HAL_StatusTypeDef BtUart_Transmit_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
//{
//   PeriStat_Set(STAT_PERI_BT);
//   return HAL_UART_Transmit_DMA(huart, pData, Size);
//}

#if defined(SHIMMER4_SDK)
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  //#if UART_DOCK0BT1
  if (huart->Instance == huartBt->Instance)
  {
    BT_txIsr(huart);
  }
  //#else
  else if (huart->Instance == huartDock->Instance)
  {
    //HAL_GPIO_TogglePin(GPIOK, GPIO_PIN_3);//blue
  }
  //#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  //#if UART_DOCK0BT1
  __NOP();
  if (huart->Instance == huartBt->Instance)
  {
    BT_rxIsr(huart);
  }
  //#else
  else if (huart->Instance == huartDock->Instance)
  {
    dockUartRxCallback(huart);
  }
  //#endif
}
#endif

void dockUartRxCallback(UART_HandleTypeDef *huart)
{
  //Board_ledToggle(LED_YELLOW);
  if (stat.isDocked)
  {
    DockUart_rxCallback(uartDockRxBuf[0]);
    //} else {
    //   ExpUart_rxCallback(uartDockRxBuf[0]);
  }
  //HAL_UART_Receive_DMA(huartDock, uartDockRxBuf, 1);
  HAL_UART_Receive_IT(huartDock, uartDockRxBuf, 1);
}

/* USER CODE END 1 */
