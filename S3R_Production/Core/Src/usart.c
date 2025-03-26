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

#include "crc.h"

#include "log_and_stream_externs.h"
#include "shimmer_definitions.h"

UART_HandleTypeDef *huartBt;
UART_HandleTypeDef *huartDock;

uint8_t uartDockRxBuf[2];

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef handle_GPDMA1_Channel1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  //Control Return if peripheral is already initialised
  if (!shimmerStatus.docked || DockUart_isInitialised())
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

  ShimDock_resetVariables();
  DockUart_init(&huart1);

  MX_CRC_Init();

  /* USER CODE END USART1_Init 2 */
}

/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

#ifdef SR48_6_0
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 1000000;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_SWAP_INIT;
  huart3.AdvancedInit.Swap = UART_ADVFEATURE_SWAP_ENABLE;
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
#else

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

#endif

  MX_CRC_Init();

  /* USER CODE END USART3_Init 2 */
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (uartHandle->Instance == USART1)
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
    GPIO_InitStruct.Pin = DOCK_TXD_Pin | DOCK_RXD_Pin;
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
  else if (uartHandle->Instance == USART3)
  {
    /* USER CODE BEGIN USART3_MspInit 0 */

#ifdef SR48_6_0
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
    PD9     ------> USART3_RX
    PD8     ------> USART3_TX
    */
    GPIO_InitStruct.Pin = BT_RXD_Pin | BT_TXD_Pin;
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
    handle_GPDMA1_Channel1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel1.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel1.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel1.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel1.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel1.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
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
    handle_GPDMA1_Channel0.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
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
#else

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
    PD9     ------> USART3_RX
    PD11     ------> USART3_CTS
    PD12     ------> USART3_RTS
    PD8     ------> USART3_TX
    */
    GPIO_InitStruct.Pin = BT_RXD_Pin | BT_CTS_Pin | BT_RTS_Pin | BT_TXD_Pin;
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
    handle_GPDMA1_Channel1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel1.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel1.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel1.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel1.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel1.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
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
    handle_GPDMA1_Channel0.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
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

#endif

    /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle)
{

  if (uartHandle->Instance == USART1)
  {
    /* USER CODE BEGIN USART1_MspDeInit 0 */

    /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOB, DOCK_TXD_Pin | DOCK_RXD_Pin);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    /* USER CODE BEGIN USART1_MspDeInit 1 */

    /* USER CODE END USART1_MspDeInit 1 */
  }
  else if (uartHandle->Instance == USART3)
  {
    /* USER CODE BEGIN USART3_MspDeInit 0 */

    /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PD9     ------> USART3_RX
    PD11     ------> USART3_CTS
    PD12     ------> USART3_RTS
    PD8     ------> USART3_TX
    */
    HAL_GPIO_DeInit(GPIOD, BT_RXD_Pin | BT_CTS_Pin | BT_RTS_Pin | BT_TXD_Pin);

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
void BtUart_init(uint32_t baudRate, uint32_t hwFlowCtrl)
{
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
#ifdef SR48_6_0
  huartBt->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_SWAP_INIT;
  huartBt->AdvancedInit.Swap = UART_ADVFEATURE_SWAP_ENABLE;
#else
  huartBt->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif
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

  MX_CRC_Init();
}

void BtUart_update(uint32_t baudRate, uint32_t hwFlowCtrl)
{
  btUart_deint();
  BtUart_init(baudRate, hwFlowCtrl);
}

void btUart_deint(void)
{
  HAL_StatusTypeDef status = HAL_UART_Abort(huartBt);
  status = HAL_UART_DeInit(huartBt);
  if (!DockUart_isInitialised())
  {
    deinitCrc();
  }
}

uint8_t BtUart_isInitialised(void)
{
  return (huartBt != 0 && ((HAL_UART_GetState(huartBt) & 0x20) == 0x20));
}

/*****************************************************
 *
 *  dock uart
 *
 *****************************************************/

void DockUart_init(UART_HandleTypeDef *huart)
{
  huartDock = huart;

#if defined(SHIMMER3R)
  HAL_UART_RegisterCallback(huartDock, HAL_UART_RX_COMPLETE_CB_ID, dockUartRxCallback);
//HAL_UART_RegisterCallback(huartDock, HAL_UART_TX_COMPLETE_CB_ID, btUartTxCpltCallback);
#endif

  HAL_UART_Receive_IT(huartDock, uartDockRxBuf, 1);

  if (shimmerStatus.sensing)
  {
    DockUart_disable();
  }
  else
  {
    /* Not sure if this is needed but enabling here in case a previous disable
     * action is not automatically reset when the periheral is reinitialised. */
    DockUart_enable();
  }
}

void DockUart_deint(void)
{
  if (DockUart_isInitialised())
  {
    HAL_UART_DeInit(huartDock);
    if (!BtUart_isInitialised())
    {
      deinitCrc();
    }
  }
}

void DockUart_disable(void)
{
  if (DockUart_isInitialised())
  {
    __HAL_UART_DISABLE(huartDock);
  }
}

void DockUart_enable(void)
{
  if (DockUart_isInitialised())
  {
    __HAL_UART_ENABLE(huartDock);
  }
}

uint8_t DockUart_isInitialised(void)
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
  //if(!shimmerStatus.isDocked){
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
    // BT_connectionInterrupt(1);  // commenting this as this function is RN42 related
    shimmerStatus.btConnected = 1;
    Board_ledOn(LED_UPR_BLUE);
  }
  else
  {
    //HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, GPIO_PIN_SET);//blue
    //BT_connectionInterrupt(0); // commenting this as this function is RN42 related
    shimmerStatus.btConnected = 0;
    ShimTask_set(TASK_STOPSENSING);
    Board_ledOff(LED_UPR_BLUE);
  }
  return shimmerStatus.btConnected;
}

//void BtUart_rtsIntCheck(void) {
//   BT_rtsInterrupt(HAL_GPIO_ReadPin(BTH_RTS_GPIO_Port, BTH_RTS_Pin));
//}

uint8_t DockUart_interruptCheck(void)
{
  //#if TEST_UNDOCKED
  //  if (0)
  //  {
  //#else
  //  if (HAL_GPIO_ReadPin(DOCK_DETECT_GPIO_Port, DOCK_DETECT_Pin) ==
  //  GPIO_PIN_SET) { //docked
  //#endif
  //    shimmerStatus.isDocked = 1;
  //    //Board_sd2Pc();
  //    //Board_ledOn(LED_GREEN0);
  //  }
  //  else
  //  {
  //    shimmerStatus.isDocked = 0;
  //    //Board_sd2Arm();
  //    //SD_mount(1);
  //    //Board_ledOff(LED_GREEN0);
  //  }

#if TEST_UNDOCKED
  shimmerStatus.isDocked = 1;
#else
#if SR48_6_0_PATCH_DOCK_DETECT
  /* Re-purposing SR48-6-0 BOOT0/USER button interrupt for dock detection*/
  shimmerStatus.docked = HAL_GPIO_ReadPin(SR48_6_0_BOOT0_USER_BTN_GPIO_Port, SR48_6_0_BOOT0_USER_BTN_Pin)
      == GPIO_PIN_SET;
#else
  shimmerStatus.docked
      = HAL_GPIO_ReadPin(DOCK_DETECT_GPIO_Port, DOCK_DETECT_Pin) == GPIO_PIN_SET;
#endif
#endif

  return shimmerStatus.docked;
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

  ShimDock_rxCallback(uartDockRxBuf[0]);

  //HAL_UART_Receive_DMA(huartDock, uartDockRxBuf, 1);
  HAL_UART_Receive_IT(huartDock, uartDockRxBuf, 1);
}

void DockUart_writeBlocking(uint8_t *buf, uint8_t len)
{
  //HAL_UART_Transmit_IT(&huart6, uartRespBuf, uart_resp_len);
  // Takes ~1.2ms to transmit 135 bytes @ 115200 baud therefore setting timeout to be > ~38.4ms
  HAL_UART_Transmit(huartDock, buf, len, 100);
}

void DockUart_writeText(char *str)
{
  DockUart_writeBlocking((uint8_t *) str, strlen(str));
}

/* USER CODE END 1 */
