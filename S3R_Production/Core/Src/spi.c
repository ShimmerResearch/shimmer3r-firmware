/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    spi.c
 * @brief   This file provides code for the configuration
 *          of the SPI instances.
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
#include "spi.h"

/* USER CODE BEGIN 0 */

//TODO remove, needed until SW supports setting pressure sensor rate in config
#include "bmp3_defs.h"
#define BOOT_TIME 20 //LIS2MDL = lis2dw12 = 20ms, LSM6DSV = 10

#if defined(SHIMMER3R)
SPI_HandleTypeDef *hspiSensing1;
SPI_HandleTypeDef *hspiSensing2;
#endif
SPI_HandleTypeDef *hspiExg;
//static SENSINGTypeDef *pSensing;

#if defined(SHIMMER3R)
spi1ReadBuf spi1Sens_buf;
spi2ReadBuf spi2Sens_buf;
spi3ReadBuf spi3Sens_buf;

SPITypeDef spi1Sens;
SPITypeDef spi2Sens;
SPITypeDef spi3Sens;

volatile uint8_t expectedSpiBusCbFlags = 0;
volatile uint8_t currentSpiBusCbFlags = 0;
SPI_ADCTypeDef spiAdc;
#endif

void (*SPI_gatherDataDone_cb)(void);

/* USER CODE END 0 */

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef handle_GPDMA1_Channel5;
DMA_HandleTypeDef handle_GPDMA1_Channel4;
DMA_HandleTypeDef handle_GPDMA1_Channel7;
DMA_HandleTypeDef handle_GPDMA1_Channel6;
DMA_HandleTypeDef handle_GPDMA1_Channel9;
DMA_HandleTypeDef handle_GPDMA1_Channel8;

/* SPI1 init function */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  lsm6dsv_unselectDevice();
  adxl371_unselectDevice();
  bmp3_unselectDevice();

  if (isAds7028Present())
  {
    ads7028_setCS(1);
  }

  /* USER CODE END SPI1_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = { 0 };

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x7;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi1.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi1.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi1, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  HAL_SPI_RegisterCallback(&hspi1, HAL_SPI_TX_RX_COMPLETE_CB_ID, SPI1_TxRxCpltCallback);
  HAL_SPI_RegisterCallback(&hspi1, HAL_SPI_ERROR_CB_ID, SPI_ErrorCallback);

  hspiSensing1 = &hspi1;

  lsm6dsv_driver_init();
  bmp3_driver_init();
  adxl371_driver_init();

  HAL_Delay(BOOT_TIME);

  /* USER CODE END SPI1_Init 2 */
}

/* SPI2 init function */
void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  lis3mdl_unselectDevice();
  lis2dw12_unselectDevice();

  /* USER CODE END SPI2_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = { 0 };

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x7;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi2, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  HAL_SPI_RegisterCallback(&hspi2, HAL_SPI_TX_RX_COMPLETE_CB_ID, SPI2_TxRxCpltCallback);
  HAL_SPI_RegisterCallback(&hspi2, HAL_SPI_ERROR_CB_ID, SPI_ErrorCallback);

  hspiSensing2 = &hspi2;

  lis2dw12_driver_init();
  lis3mdl_driver_init();

  HAL_Delay(BOOT_TIME);

  /* USER CODE END SPI2_Init 2 */
}

/* SPI3 init function */
void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */
  if (ShimBrd_isAds1292Present())
  {

    /* USER CODE END SPI3_Init 0 */

    SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = { 0 };

    /* USER CODE BEGIN SPI3_Init 1 */

    /* USER CODE END SPI3_Init 1 */
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi3.Init.NSS = SPI_NSS_SOFT;
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 0x7;
    hspi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi3.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi3.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi3.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi3.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi3.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi3.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi3.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    hspi3.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
    hspi3.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
    if (HAL_SPI_Init(&hspi3) != HAL_OK)
    {
      Error_Handler();
    }
    HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
    HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP2_LPDMA_CH0_TCF_TRG;
    HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
    if (HAL_SPIEx_SetConfigAutonomousMode(&hspi3, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
    {
      Error_Handler();
    }
    /* USER CODE BEGIN SPI3_Init 2 */

    HAL_SPI_RegisterCallback(&hspi3, HAL_SPI_TX_RX_COMPLETE_CB_ID, SPI3_TxRxCpltCallback);
    HAL_SPI_RegisterCallback(&hspi3, HAL_SPI_ERROR_CB_ID, SPI_ErrorCallback);
    hspiExg = &hspi3;
  }

  /* USER CODE END SPI3_Init 2 */
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (spiHandle->Instance == SPI1)
  {
    /* USER CODE BEGIN SPI1_MspInit 0 */

    /* USER CODE END SPI1_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
    PeriphClkInit.Spi1ClockSelection = RCC_SPI1CLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PE14     ------> SPI1_MISO
    PE15     ------> SPI1_MOSI
    PE13     ------> SPI1_SCK
    */
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* GPDMA1_REQUEST_SPI1_TX Init */
    handle_GPDMA1_Channel5.Instance = GPDMA1_Channel5;
    handle_GPDMA1_Channel5.Init.Request = GPDMA1_REQUEST_SPI1_TX;
    handle_GPDMA1_Channel5.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel5.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel5.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel5.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel5.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel5.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel5.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel5.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel5.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel5.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel5.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel5.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel5) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle, hdmatx, handle_GPDMA1_Channel5);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel5, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* GPDMA1_REQUEST_SPI1_RX Init */
    handle_GPDMA1_Channel4.Instance = GPDMA1_Channel4;
    handle_GPDMA1_Channel4.Init.Request = GPDMA1_REQUEST_SPI1_RX;
    handle_GPDMA1_Channel4.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel4.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel4.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel4.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel4.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel4.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel4.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel4.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel4.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel4.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel4.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel4.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel4) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle, hdmarx, handle_GPDMA1_Channel4);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel4, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI1 interrupt Init */
    HAL_NVIC_SetPriority(SPI1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);
    /* USER CODE BEGIN SPI1_MspInit 1 */

    /* USER CODE END SPI1_MspInit 1 */
  }
  else if (spiHandle->Instance == SPI2)
  {
    /* USER CODE BEGIN SPI2_MspInit 0 */

    /* USER CODE END SPI2_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
    PeriphClkInit.Spi2ClockSelection = RCC_SPI2CLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI2 clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**SPI2 GPIO Configuration
    PB13     ------> SPI2_SCK
    PB14     ------> SPI2_MISO
    PB15     ------> SPI2_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* SPI2 DMA Init */
    /* GPDMA1_REQUEST_SPI2_TX Init */
    handle_GPDMA1_Channel7.Instance = GPDMA1_Channel7;
    handle_GPDMA1_Channel7.Init.Request = GPDMA1_REQUEST_SPI2_TX;
    handle_GPDMA1_Channel7.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel7.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel7.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel7.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel7.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel7.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel7.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel7.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel7.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel7.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel7.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel7.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel7) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle, hdmatx, handle_GPDMA1_Channel7);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel7, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* GPDMA1_REQUEST_SPI2_RX Init */
    handle_GPDMA1_Channel6.Instance = GPDMA1_Channel6;
    handle_GPDMA1_Channel6.Init.Request = GPDMA1_REQUEST_SPI2_RX;
    handle_GPDMA1_Channel6.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel6.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel6.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel6.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel6.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel6.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel6.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel6.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel6.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel6.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel6.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel6.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel6) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle, hdmarx, handle_GPDMA1_Channel6);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel6, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI2 interrupt Init */
    HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);
    /* USER CODE BEGIN SPI2_MspInit 1 */

    /* USER CODE END SPI2_MspInit 1 */
  }
  else if (spiHandle->Instance == SPI3)
  {
    /* USER CODE BEGIN SPI3_MspInit 0 */

    /* USER CODE END SPI3_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI3;
    PeriphClkInit.Spi3ClockSelection = RCC_SPI3CLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI3 clock enable */
    __HAL_RCC_SPI3_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    /**SPI3 GPIO Configuration
    PB4 (NJTRST)     ------> SPI3_MISO
    PG9     ------> SPI3_SCK
    PB5     ------> SPI3_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* SPI3 DMA Init */
    /* GPDMA1_REQUEST_SPI3_TX Init */
    handle_GPDMA1_Channel9.Instance = GPDMA1_Channel9;
    handle_GPDMA1_Channel9.Init.Request = GPDMA1_REQUEST_SPI3_TX;
    handle_GPDMA1_Channel9.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel9.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel9.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel9.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel9.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel9.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel9.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel9.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel9.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel9.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel9.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel9.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel9) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle, hdmatx, handle_GPDMA1_Channel9);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel9, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* GPDMA1_REQUEST_SPI3_RX Init */
    handle_GPDMA1_Channel8.Instance = GPDMA1_Channel8;
    handle_GPDMA1_Channel8.Init.Request = GPDMA1_REQUEST_SPI3_RX;
    handle_GPDMA1_Channel8.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel8.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel8.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel8.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel8.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel8.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel8.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel8.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel8.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel8.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel8.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel8.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel8) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle, hdmarx, handle_GPDMA1_Channel8);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel8, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI3 interrupt Init */
    HAL_NVIC_SetPriority(SPI3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SPI3_IRQn);
    /* USER CODE BEGIN SPI3_MspInit 1 */

    /* USER CODE END SPI3_MspInit 1 */
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *spiHandle)
{

  if (spiHandle->Instance == SPI1)
  {
    /* USER CODE BEGIN SPI1_MspDeInit 0 */

    /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PE14     ------> SPI1_MISO
    PE15     ------> SPI1_MOSI
    PE13     ------> SPI1_SCK
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_13);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmatx);
    HAL_DMA_DeInit(spiHandle->hdmarx);

    /* SPI1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI1_IRQn);
    /* USER CODE BEGIN SPI1_MspDeInit 1 */

    /* USER CODE END SPI1_MspDeInit 1 */
  }
  else if (spiHandle->Instance == SPI2)
  {
    /* USER CODE BEGIN SPI2_MspDeInit 0 */

    /* USER CODE END SPI2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI2_CLK_DISABLE();

    /**SPI2 GPIO Configuration
    PB13     ------> SPI2_SCK
    PB14     ------> SPI2_MISO
    PB15     ------> SPI2_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    /* SPI2 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmatx);
    HAL_DMA_DeInit(spiHandle->hdmarx);

    /* SPI2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI2_IRQn);
    /* USER CODE BEGIN SPI2_MspDeInit 1 */

    /* USER CODE END SPI2_MspDeInit 1 */
  }
  else if (spiHandle->Instance == SPI3)
  {
    /* USER CODE BEGIN SPI3_MspDeInit 0 */

    /* USER CODE END SPI3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();

    /**SPI3 GPIO Configuration
    PB4 (NJTRST)     ------> SPI3_MISO
    PG9     ------> SPI3_SCK
    PB5     ------> SPI3_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_4 | GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_9);

    /* SPI3 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmatx);
    HAL_DMA_DeInit(spiHandle->hdmarx);

    /* SPI3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI3_IRQn);
    /* USER CODE BEGIN SPI3_MspDeInit 1 */

    /* USER CODE END SPI3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void SPI1_DeInit(void)
{
  HAL_SPI_DeInit(hspiSensing1);

  lsm6dsv_selectDevice();
  adxl371_selectDevice();
  bmp3_selectDevice();

  if (isAds7028Present())
  {
    ads7028_setCS(0);
  }
}

void SPI2_DeInit(void)
{
  HAL_SPI_DeInit(hspiSensing2);

  lis3mdl_selectDevice();
  lis2dw12_selectDevice();
}

void SPI3_DeInit(void)
{
  HAL_SPI_DeInit(hspiExg);

  //TODO
  //ads1292r_exg1_unselectDevice();
  //ads1292r_exg2_unselectDevice();

  //Board_SW_SPI2(0);
  ADS1292_chip1CsEnable(0);
  ADS1292_chip2CsEnable(0);
}

void SPI_configureChannels()
{
  uint8_t *channel_contents_ptr = sensing.cc + sensing.ccLen;
  gConfigBytes *configBytes = ShimConfig_getStoredConfig();

#if defined(SHIMMER3R)
  memset((uint8_t *) &spi1Sens, 0, sizeof(spi1Sens));
  memset((uint8_t *) &spi2Sens, 0, sizeof(spi2Sens));
  memset((uint8_t *) &spi3Sens, 0, sizeof(spi3Sens));

  spi1Sens.busId = SPI1_BUS_FLAG;
  spi2Sens.busId = SPI2_BUS_FLAG;
  spi3Sens.busId = SPI3_BUS_FLAG;

  if (configBytes->chEnGyro)
  {
    *channel_contents_ptr++ = X_GYRO;
    *channel_contents_ptr++ = Y_GYRO;
    *channel_contents_ptr++ = Z_GYRO;
    sensing.nbrSpiChans += 3;
    sensing.ptr.gyro = sensing.dataLen;
    sensing.dataLen += 6;
  }

  if (configBytes->chEnLnAccel)
  {
    *channel_contents_ptr++ = X_LN_ACCEL;
    *channel_contents_ptr++ = Y_LN_ACCEL;
    *channel_contents_ptr++ = Z_LN_ACCEL;
    sensing.nbrSpiChans += 3;
    sensing.ptr.accel1 = sensing.dataLen;
    sensing.dataLen += 6;
  }

  if (configBytes->chEnGyro && configBytes->chEnLnAccel)
  {
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_LSM6DSV_GYRO_AND_ACCEL;
  }
  else if (configBytes->chEnGyro)
  {
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_LSM6DSV_GYRO_ONLY;
  }
  else if (configBytes->chEnLnAccel)
  {
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_LSM6DSV_ACCEL_ONLY;
  }

  if (configBytes->chEnPressureAndTemperature)
  {
    sensing.nbrSpiChans += 2; //TEMP & PRES, ON/OFF together
#if defined(SHIMMER3R)
    *channel_contents_ptr++ = BMP_PRESSURE;
    *channel_contents_ptr++ = BMP_TEMPERATURE;
    sensing.ptr.pressure = sensing.dataLen;
    sensing.dataLen += 3;
    sensing.ptr.temperature = sensing.dataLen;
    sensing.dataLen += 3;
#else
    *channel_contents_ptr++ = BMP_TEMPERATURE;
    *channel_contents_ptr++ = BMP_PRESSURE;
    sensing.ptr.temperature = sensing.dataLen;
    sensing.dataLen += 3;
    sensing.ptr.pressure = sensing.dataLen;
    sensing.dataLen += 3;
#endif
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_BMP390_PRESSURE_TEMP;
  }

  if (configBytes->chEnAltAccel)
  {
    *channel_contents_ptr++ = X_ALT_ACCEL;
    *channel_contents_ptr++ = Y_ALT_ACCEL;
    *channel_contents_ptr++ = Z_ALT_ACCEL;
    sensing.nbrSpiChans += 3;
    sensing.ptr.accel3 = sensing.dataLen;
    sensing.dataLen += 6;
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADXL371_ACCEL;
  }

  if (configBytes->chEnWrAccel)
  {
    *channel_contents_ptr++ = X_WR_ACCEL;
    *channel_contents_ptr++ = Y_WR_ACCEL;
    *channel_contents_ptr++ = Z_WR_ACCEL;
    sensing.nbrSpiChans += 3;
    sensing.ptr.accel2 = sensing.dataLen;
    sensing.dataLen += 6;
    spi2Sens.sensorList[spi2Sens.sensorLen++] = SPI2_LIS2DW12_ACCEL;
  }

  if (configBytes->chEnAltMag)
  {
    *channel_contents_ptr++ = X_ALT_MAG;
    *channel_contents_ptr++ = Y_ALT_MAG;
    *channel_contents_ptr++ = Z_ALT_MAG;
    sensing.nbrSpiChans += 3;
    sensing.ptr.mag2 = sensing.dataLen;
    sensing.dataLen += 6;
    spi2Sens.sensorList[spi2Sens.sensorLen++] = SPI2_LIS3MDL_MAG;
  }
#endif

  //ExG (spi)
  if (ShimBrd_isAds1292Present())
  {
    if (configBytes->chEnExg1_24Bit || configBytes->chEnExg2_24Bit
        || configBytes->chEnExg1_16Bit || configBytes->chEnExg2_16Bit)
    {
      if (configBytes->chEnExg1_24Bit)
      {
        *channel_contents_ptr++ = EXG_ADS1292R_1_STATUS;
        *channel_contents_ptr++ = EXG_ADS1292R_1_CH1_24BIT;
        *channel_contents_ptr++ = EXG_ADS1292R_1_CH2_24BIT;
        sensing.nbrSpiChans += 3;
        configBytes->chEnExg1_16Bit = 0;
        sensing.ptr.exg1 = sensing.dataLen;
        sensing.dataLen += 7;
#if defined(SHIMMER3R)
        spi3Sens.sensorList[spi3Sens.sensorLen++] = SPI3_ADS1292R_EXG1;
#endif
      }
      else if (configBytes->chEnExg1_16Bit)
      {
        *channel_contents_ptr++ = EXG_ADS1292R_1_STATUS;
        *channel_contents_ptr++ = EXG_ADS1292R_1_CH1_16BIT;
        *channel_contents_ptr++ = EXG_ADS1292R_1_CH2_16BIT;
        sensing.nbrSpiChans += 3;
        sensing.ptr.exg1 = sensing.dataLen;
        sensing.dataLen += 5;
#if defined(SHIMMER3R)
        spi3Sens.sensorList[spi3Sens.sensorLen++] = SPI3_ADS1292R_EXG1;
#endif
      }
      if (configBytes->chEnExg2_24Bit)
      {
        *channel_contents_ptr++ = EXG_ADS1292R_2_STATUS;
        *channel_contents_ptr++ = EXG_ADS1292R_2_CH1_24BIT;
        *channel_contents_ptr++ = EXG_ADS1292R_2_CH2_24BIT;
        sensing.nbrSpiChans += 3;
        configBytes->chEnExg2_16Bit = 0;
        sensing.ptr.exg2 = sensing.dataLen;
        sensing.dataLen += 7;
#if defined(SHIMMER3R)
        spi3Sens.sensorList[spi3Sens.sensorLen++] = SPI3_ADS1292R_EXG2;
#endif
      }
      else if (configBytes->chEnExg2_16Bit)
      {
        *channel_contents_ptr++ = EXG_ADS1292R_2_STATUS;
        *channel_contents_ptr++ = EXG_ADS1292R_2_CH1_16BIT;
        *channel_contents_ptr++ = EXG_ADS1292R_2_CH2_16BIT;
        sensing.nbrSpiChans += 3;
        sensing.ptr.exg2 = sensing.dataLen;
        sensing.dataLen += 5;
#if defined(SHIMMER3R)
        spi3Sens.sensorList[spi3Sens.sensorLen++] = SPI3_ADS1292R_EXG2;
#endif
      }
    }
  }

  //External ADC ADS7028
  if (isAds7028Present())
  {
    ads7028_configureChannels(channel_contents_ptr);
  }

  sensing.ccLen += sensing.nbrSpiChans;

#if defined(SHIMMER3R)
  expectedSpiBusCbFlags = 0;
  if (spi1Sens.sensorLen > 0)
  {
    expectedSpiBusCbFlags |= SPI1_BUS_FLAG;
  }
  if (spi2Sens.sensorLen > 0)
  {
    expectedSpiBusCbFlags |= SPI2_BUS_FLAG;
  }
  if (spi3Sens.sensorLen > 0)
  {
    expectedSpiBusCbFlags |= SPI3_BUS_FLAG;
  }
#endif
}

void SPI_startSensing()
{
  static uint8_t temp_exg_buf[11];
  gConfigBytes *configBytes = ShimConfig_getStoredConfig();
  float shimmerSamplingFreq = ShimConfig_getShimmerSamplingFreq();

  memset((uint8_t *) &spi1Sens_buf, 0, sizeof(spi1ReadBuf));
  memset((uint8_t *) &spi2Sens_buf, 0, sizeof(spi2ReadBuf));
  memset((uint8_t *) &spi3Sens_buf, 0, sizeof(spi3ReadBuf));

  if (spi1Sens.sensorLen > 0)
  {
    MX_SPI1_Init();
  }
  if (spi2Sens.sensorLen > 0)
  {
    MX_SPI2_Init();
  }

#if defined(SHIMMER3R)
  /* SPI1 */
  if ((configBytes->chEnLnAccel) || (configBytes->chEnGyro))
  {
    uint8_t gyroRange = ShimConfig_gyroRangeGet();
    //Shimmer config maps 0x05 to the chip's 0x0C for 4000dps
    gyroRange = (gyroRange == 5) ? LSM6DSV_4000dps : gyroRange;
    lsm6dsv_configure(shimmerSamplingFreq, configBytes->chEnGyro, configBytes->chEnLnAccel,
        configBytes->gyroRate, gyroRange, configBytes->lnAccelRange);
  }

  if (configBytes->chEnPressureAndTemperature)
  {
    int8_t rslt = bmp3_configure(shimmerSamplingFreq,
        ShimConfig_configBytePressureOversamplingRatioGet());
  }

  if (configBytes->chEnAltAccel)
  {
    adxl371_configure(configBytes->altAccelRate);
  }

  if (isAds7028Present() && ads7028_areAnyChannelsEnabled()) //External ADC ADS7028
  {
    if (configBytes->chEnGsr)
    {
      GSR_init(configBytes->gsrRange, configBytes->samplingRateTicks);
    }
    //configureAutoSequenceChannel(enabledAdcChannels);
  }

  /* SPI2 */
  if (configBytes->chEnWrAccel)
  {
    lis2dw12_configure(shimmerSamplingFreq, configBytes->wrAccelRate,
        configBytes->wrAccelRange, ShimConfig_wrAccelModeGet());
  }

  if (configBytes->chEnAltMag)
  {
    lis3mdl_configure(shimmerSamplingFreq, ShimConfig_configByteAltMagRateGet(),
        configBytes->altMagRange);
  }

#endif

  //ExG (SPI3)
  if (ShimBrd_isAds1292Present()
      && (configBytes->chEnExg1_24Bit || configBytes->chEnExg2_24Bit
          || configBytes->chEnExg1_16Bit || configBytes->chEnExg2_16Bit))
  {
    ADS1292_init();
    MX_SPI3_Init();
    EXG_init(hspiExg);
    if (configBytes->chEnExg1_24Bit || configBytes->chEnExg1_16Bit)
    {
      ShimConfig_storedConfigGet(temp_exg_buf, NV_EXG_ADS1292R_1_CONFIG1, 10);
      EXG_writeRegs(0, ADS1292R_CONFIG1, 10, temp_exg_buf);
      EXG_readRegs(0, 0, 11, temp_exg_buf); //can read back to check if write is done successfully
    }
    if (configBytes->chEnExg2_24Bit || configBytes->chEnExg2_16Bit)
    {
      HAL_Delay(100); //100ms
      EXG_setRdatac(1, 0);
      ShimConfig_storedConfigGet(temp_exg_buf, NV_EXG_ADS1292R_2_CONFIG1, 10);
      EXG_writeRegs(1, ADS1292R_CONFIG1, 10, temp_exg_buf);
      EXG_readRegs(1, 0, 11, temp_exg_buf);
      EXG_enableChip2(1);
    }
    __NOP();
    __NOP();
    __NOP();

    //probably turning on internal reference, so wait for it to settle
    HAL_Delay(100); //100ms

    //probably setting the PGA gain so cancel the channel offset
    if ((configBytes->chEnExg1_24Bit || configBytes->chEnExg1_16Bit)
        && (ShimConfig_storedConfigGetByte(NV_EXG_ADS1292R_1_RESP2) & 0x80))
    {
      EXG_offsetCal(0);
    }
    if ((configBytes->chEnExg2_24Bit || configBytes->chEnExg2_16Bit)
        && (ShimConfig_storedConfigGetByte(NV_EXG_ADS1292R_2_RESP2) & 0x80))
    {
      EXG_offsetCal(1);
    }

    if ((configBytes->chEnExg1_24Bit || configBytes->chEnExg1_16Bit)
        && (configBytes->chEnExg2_24Bit || configBytes->chEnExg2_16Bit))
    {
      EXG_start(2);
    }
    else if (configBytes->chEnExg1_24Bit || configBytes->chEnExg1_16Bit)
    {
      EXG_start(0);
    }
    else
    {
      EXG_start(1);
    }

    EXG_setDrdyInterruptState(1, configBytes->chEnExg1_24Bit || configBytes->chEnExg1_16Bit,
        configBytes->chEnExg2_24Bit || configBytes->chEnExg2_16Bit);
  }
}

void SPI_pollSensors(void)
{
  currentSpiBusCbFlags = 0;

  //ExG (SPI)
  if (ShimBrd_isAds1292Present())
  {
    //exg
    gConfigBytes *storedConfigPtr = ShimConfig_getStoredConfig();

    if (storedConfigPtr->chEnExg1_24Bit || storedConfigPtr->chEnExg1_16Bit
        || storedConfigPtr->chEnExg2_24Bit || storedConfigPtr->chEnExg2_16Bit)
    {
      if (storedConfigPtr->chEnExg1_24Bit)
      {
        EXG_readData(0, 0, &sensing.dataBuf[sensing.ptr.exg1]);
      }
      else if (storedConfigPtr->chEnExg1_16Bit)
      {
        EXG_readData(0, 1, &sensing.dataBuf[sensing.ptr.exg1]);
      }
      if (storedConfigPtr->chEnExg2_24Bit)
      {
        EXG_readData(1, 0, &sensing.dataBuf[sensing.ptr.exg2]);
        if (!(sensing.dataBuf[sensing.ptr.exg2 + 1] == 0x00
                || sensing.dataBuf[sensing.ptr.exg2 + 1] == 0xff))
        {
          __NOP();
        }
      }
      else if (storedConfigPtr->chEnExg2_16Bit)
      {
        EXG_readData(1, 1, &sensing.dataBuf[sensing.ptr.exg2]);
      }

      SPI_busGatherDataDone_cb(spi3Sens.busId);
    }
  }

  if (spi1Sens.sensorLen > 0)
  {
    SpiSensing(&spi1Sens, SPI_FIRST_SENSOR);
  }
  if (spi2Sens.sensorLen > 0)
  {
    SpiSensing(&spi2Sens, SPI_FIRST_SENSOR);
  }
}

void SPI_stopSensing()
{
  //SPI3
  if (ShimBrd_isAds1292Present())
  {
    gConfigBytes *configBytes = ShimConfig_getStoredConfig();

    //HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    //HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    if (configBytes->chEnExg2_24Bit || configBytes->chEnExg2_16Bit)
    {
      EXG_stop(1); //probably not needed
    }
    if (configBytes->chEnExg1_24Bit || configBytes->chEnExg1_16Bit)
    {
      EXG_stop(0); //probably not needed
    }
    //if(configBytes->chEnExg1_24Bit
    //   || configBytes->chEnExg2_24Bit
    //   || configBytes->chEnExg1_16Bit
    //   || configBytes->chEnExg2_16Bit) {
    EXG_powerOff();
    //}
    //HAL_SPI_MspDeInit(hspiExg);//this may save .2-.3 mA?

    EXG_setDrdyInterruptState(0, configBytes->chEnExg1_24Bit || configBytes->chEnExg1_16Bit,
        configBytes->chEnExg2_24Bit || configBytes->chEnExg2_16Bit);
  }

  resetGsrPwrAndRange();
  if (ads7028_areAnyChannelsEnabled())
  {
    stopAds7028Conversions();
    //HAL_TIM_Base_MspDeInit(&htim1);
  }
  SPI1_DeInit();
  SPI2_DeInit();
  SPI3_DeInit();
}

void SPI_gatherDataCb(void (*done_cb)(void))
{
  SPI_gatherDataDone_cb = done_cb;
#if defined(SHIMMER4_SDK)
  EXG_gatherDataInit(SpiStep2Start);
#endif
}

#if defined(SHIMMER3R)
void SPI_busGatherDataDone_cb(uint8_t flag)
{
  //if all SPI buses complete, call main callback to sensing.c
  currentSpiBusCbFlags |= flag;
  if (currentSpiBusCbFlags == expectedSpiBusCbFlags)
  {
    SPI_gatherDataDone_cb();
  }
}
#endif

#if defined(SHIMMER4_SDK)
void SPI_gatherDataStart(void)
{
  SpiStep1Start();
}

void SpiStep1Start(void)
{
  EXG_gatherDataStart();
}

void SpiStep2Start(void)
{
  SpiStep3Start();
}

void SpiStep3Start(void)
{
  SpiStepDone();
}

void SpiStepDone(void)
{
  SPI_gatherDataDone_cb();
}
#endif

#if defined(SHIMMER3R)
void SpiSensing(SPITypeDef *spiSensingInfo, SPI_SENSING_TYPE start)
{
  spiSensingInfo->sensorCnt
      = (start == SPI_FIRST_SENSOR) ? 0 : spiSensingInfo->sensorCnt + 1;
  if (spiSensingInfo->sensorCnt == spiSensingInfo->sensorLen)
  {
    spiSensingInfo->status = SPI_STAT_IDLE;
    spiSensingInfo->sensorCnt = 0;
    if (spiSensingInfo->busId == SPI3_BUS_FLAG)
    {
      spi3Sens_buf.exg1Data_read = 0;
      spi3Sens_buf.exg2Data_read = 0;
    }
    else
    {
      SPI_busGatherDataDone_cb(spiSensingInfo->busId);
    }
  }
  else if (spiSensingInfo->sensorCnt < spiSensingInfo->sensorLen)
  {
    uint8_t waitingDmaRxcb = 0;
    /* Return of 1 = waiting on response from DMA operation. Return of 0 = move onto next sensor. */
    while ((waitingDmaRxcb = SpiSens_sensorNext(spiSensingInfo)) == 0)
    {
      spiSensingInfo->sensorCnt++;

      if (spiSensingInfo->sensorCnt == spiSensingInfo->sensorLen)
      {
        spiSensingInfo->status = SPI_STAT_IDLE;
        spiSensingInfo->sensorCnt = 0;

        /* SPI3 complete callback is ha           */
        if (spiSensingInfo->busId == SPI3_BUS_FLAG)
        {
          spi3Sens_buf.exg1Data_read = 0;
          spi3Sens_buf.exg2Data_read = 0;
        }
        else
        {
          SPI_busGatherDataDone_cb(spiSensingInfo->busId);
        }
        break;
      }
    }
  }
  else
  {
    while (1)
    {
      //TODO handle this differently
      Board_ledToggle(LED_ALL);
      HAL_Delay(100);
    }
  }
}

uint8_t SpiSens_sensorNext(SPITypeDef *spiSensingInfo)
{
  uint8_t retVal = 0;
  HAL_StatusTypeDef halRet = 0xFF;
  gConfigBytes *configBytes = ShimConfig_getStoredConfig();

  switch (spiSensingInfo->sensorList[spiSensingInfo->sensorCnt])
  {
  case SPI1_LSM6DSV_GYRO_AND_ACCEL:
    if (!lsm6dsv_is_drdy_int_enabled() || LSM6DSV_DRDY)
    {
      spiSensingInfo->status = SPI_STAT_LSM6DSV_GYRO_AND_ACCEL_GET;
      halRet = lsm6dsv_gyro_accel_get(spi1Sens_buf.lsm6dsvGyroAndAccelBuf);
      retVal = 1;
    }
    break;
  case SPI1_LSM6DSV_ACCEL_ONLY:
    if (!lsm6dsv_is_drdy_int_enabled() || LSM6DSV_DRDY)
    {
      spiSensingInfo->status = SPI_STAT_LSM6DSV_ACCEL_GET;
      halRet = lsm6dsv_accel_get(spi1Sens_buf.lsm6dsvAccelBuf);
      retVal = 1;
    }
    break;
  case SPI1_LSM6DSV_GYRO_ONLY:
    if (!lsm6dsv_is_drdy_int_enabled() || LSM6DSV_DRDY)
    {
      spiSensingInfo->status = SPI_STAT_LSM6DSV_GYRO_GET;
      halRet = lsm6dsv_gyro_get(spi1Sens_buf.lsm6dsvGyroBuf);
      retVal = 1;
    }
    break;
  case SPI1_ADXL371_ACCEL:
    if (adxl371_is_data_rdy() == 1)
    {
      spiSensingInfo->status = SPI_STAT_ADXL371_ACCEL_GET;
      halRet = adxl371_accel_get(spi1Sens_buf.adxl371Buf);
      retVal = 1;
    }
    break;
  case SPI1_BMP390_PRESSURE_TEMP:
    if (!bmp3_is_drdy_int_enabled() || BMP390_INT)
    {
      spiSensingInfo->status = SPI_STAT_BMP390_PRESSURE_TEMPERATURE_GET;
      halRet = bmp3_pressure_temperature_get(spi1Sens_buf.bmp390Buf);
      retVal = 1;
    }
    break;
  case SPI1_ADS7028_INT_EXP0:
    spiSensingInfo->status = SPI_STAT_ADS7028_INT_EXP0_GET;
    configureAutoSequenceChannel(AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH0_ENABLED);
    halRet = ads7028_dataGetDma(spi1Sens_buf.ads2078Buf);
    retVal = 1;
    break;
  case SPI1_ADS7028_INT_EXP1:
    spiSensingInfo->status = SPI_STAT_ADS7028_INT_EXP1_GET;
    configureAutoSequenceChannel(AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH1_ENABLED);
    halRet = ads7028_dataGetDma(spi1Sens_buf.ads2078Buf);
    retVal = 1;
    break;
  case SPI1_ADS7028_INT_EXP2:
    spiSensingInfo->status = SPI_STAT_ADS7028_INT_EXP2_GET;
    configureAutoSequenceChannel(AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH2_ENABLED);
    halRet = ads7028_dataGetDma(spi1Sens_buf.ads2078Buf);
    retVal = 1;
    break;
  case SPI1_ADS7028_INT_EXP3:
    spiSensingInfo->status = SPI_STAT_ADS7028_INT_EXP3_GET;
    configureAutoSequenceChannel(AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH3_ENABLED);
    halRet = ads7028_dataGetDma(spi1Sens_buf.ads2078Buf);
    retVal = 1;
    break;
  case SPI1_ADS7028_EXT_EXP0:
    spiSensingInfo->status = SPI_STAT_ADS7028_EXT_EXP0_GET;
    configureAutoSequenceChannel(AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH4_ENABLED);
    halRet = ads7028_dataGetDma(spi1Sens_buf.ads2078Buf);
    retVal = 1;
    break;
  case SPI1_ADS7028_EXT_EXP1:
    spiSensingInfo->status = SPI_STAT_ADS7028_EXT_EXP1_GET;
    configureAutoSequenceChannel(AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH5_ENABLED);
    halRet = ads7028_dataGetDma(spi1Sens_buf.ads2078Buf);
    retVal = 1;
    break;
  case SPI1_ADS7028_EXT_EXP2:
    spiSensingInfo->status = SPI_STAT_ADS7028_EXT_EXP2_GET;
    configureAutoSequenceChannel(AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH6_ENABLED);
    halRet = ads7028_dataGetDma(spi1Sens_buf.ads2078Buf);
    retVal = 1;
    break;
  case SPI1_ADS7028_VBATT_SENSE:
    spiSensingInfo->status = SPI_STAT_ADS7028_VBATT_GET;
    configureAutoSequenceChannel(AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH7_ENABLED);
    halRet = ads7028_dataGetDma(spi1Sens_buf.ads2078Buf);
    retVal = 1;
    break;

  case SPI2_LIS2DW12_ACCEL:
#if defined(LIS2DW12_INT1_Pin)
    if (!lis2dw12_is_drdy_int_enabled() || LIS2DW12_INT1)
    {
#endif
      spiSensingInfo->status = SPI_STAT_LIS2DW12_ACCEL_GET;
      halRet = lis2dw12_accel_get(spi2Sens_buf.lis2dw12AccelBuf);
      retVal = 1;
#if defined(LIS2DW12_INT1_Pin)
    }
#endif
    break;
  case SPI2_LIS3MDL_MAG:
    if (LIS3MDL_DRDY)
    {
      spiSensingInfo->status = SPI_STAT_LIS3MDL_MAG_GET;
      halRet = lis3mdl_mag_get(spi2Sens_buf.lis3mdlMagBuf);
      retVal = 1;
    }
    break;

  default:
    break;
  }

  /* If a DMA issue occured, return 0 to skip to next sensor */
  if (halRet != 0xFF && halRet != HAL_OK)
  {
    //printf("SPI Bus: %d, DMA Error: %d, Stage: %d\n", spiSensingInfo->busId,
    //    halRet, spiSensingInfo->status);

    retVal = 0;
    spiSensingInfo->status = SPI_STAT_IDLE;
  }

  return retVal;
}

void SPI1_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  switch (spi1Sens.sensorList[spi1Sens.sensorCnt])
  {
  case SPI1_LSM6DSV_GYRO_AND_ACCEL:
    lsm6dsv_unselectDevice();
    memcpy(sensing.dataBuf + sensing.ptr.gyro,
        &spi1Sens_buf.lsm6dsvGyroAndAccelBuf[SPI_DMA_TXRX_OFFSET],
        sizeof(spi1Sens_buf.lsm6dsvGyroAndAccelBuf) - SPI_DMA_TXRX_OFFSET);
    break;
  case SPI1_LSM6DSV_ACCEL_ONLY:
    lsm6dsv_unselectDevice();
    memcpy(sensing.dataBuf + sensing.ptr.accel1,
        &spi1Sens_buf.lsm6dsvAccelBuf[SPI_DMA_TXRX_OFFSET],
        sizeof(spi1Sens_buf.lsm6dsvAccelBuf) - SPI_DMA_TXRX_OFFSET);
    break;
  case SPI1_LSM6DSV_GYRO_ONLY:
    lsm6dsv_unselectDevice();
    memcpy(sensing.dataBuf + sensing.ptr.gyro,
        &spi1Sens_buf.lsm6dsvGyroBuf[SPI_DMA_TXRX_OFFSET],
        sizeof(spi1Sens_buf.lsm6dsvGyroBuf) - SPI_DMA_TXRX_OFFSET);
    break;
  case SPI1_ADXL371_ACCEL:
    adxl371_unselectDevice();
    memcpy(sensing.dataBuf + sensing.ptr.accel3,
        &spi1Sens_buf.adxl371Buf[SPI_DMA_TXRX_OFFSET],
        sizeof(spi1Sens_buf.adxl371Buf) - SPI_DMA_TXRX_OFFSET);
    break;
  case SPI1_BMP390_PRESSURE_TEMP:
    if (bmp3_is_drdy_int_enabled())
    {
      /* Read chip status registers to reset interrupt pin */
      bmp3_read_sensor_status();
    }
    bmp3_unselectDevice();
    memcpy(sensing.dataBuf + sensing.ptr.pressure,
        &spi1Sens_buf.bmp390Buf[SPI_DMA_TXRX_OFFSET + 1],
        sizeof(spi1Sens_buf.bmp390Buf) - SPI_DMA_TXRX_OFFSET - 1);
    break;
  case SPI1_ADS7028_INT_EXP0:
  case SPI1_ADS7028_INT_EXP1:
  case SPI1_ADS7028_INT_EXP2:
  case SPI1_ADS7028_INT_EXP3:
  case SPI1_ADS7028_EXT_EXP0:
  case SPI1_ADS7028_EXT_EXP1:
  case SPI1_ADS7028_EXT_EXP2:
  case SPI1_ADS7028_VBATT_SENSE:
    uint8_t dataBufIndex = 0;

    /* Original ADC data is MSB order and left-aligned whereas Shimmer normally
     * uses LSB order right-aligned */
    uint16_t adcTempbuf = (((uint16_t) spi1Sens_buf.ads2078Buf[0]) << 4
                              | spi1Sens_buf.ads2078Buf[1] >> 4)
        & 0x0FFF;

    switch (spi1Sens.sensorList[spi1Sens.sensorCnt])
    {
    case SPI1_ADS7028_INT_EXP0:
      dataBufIndex = sensing.ptr.intADC0;
      break;
    case SPI1_ADS7028_INT_EXP1:
      dataBufIndex = sensing.ptr.intADC1;
      break;
    case SPI1_ADS7028_INT_EXP2:
      dataBufIndex = sensing.ptr.intADC2;
      break;
    case SPI1_ADS7028_INT_EXP3:
      if (ShimConfig_isGSREnabled())
      {
        dataBufIndex = sensing.ptr.gsr;
      }
      else
      {
        dataBufIndex = sensing.ptr.intADC3;
      }
      break;
    case SPI1_ADS7028_EXT_EXP0:
      dataBufIndex = sensing.ptr.extADC0;
      break;
    case SPI1_ADS7028_EXT_EXP1:
      dataBufIndex = sensing.ptr.extADC1;
      break;
    case SPI1_ADS7028_EXT_EXP2:
      dataBufIndex = sensing.ptr.extADC2;
      break;
    case SPI1_ADS7028_VBATT_SENSE:
      dataBufIndex = sensing.ptr.batteryAnalog;
      break;
    default:
      break;
    }

    sensing.dataBuf[dataBufIndex + 0] = adcTempbuf & 0xFF;
    sensing.dataBuf[dataBufIndex + 1] = (adcTempbuf >> 8) & 0xFF;

    if (spi1Sens.sensorList[spi1Sens.sensorCnt] == SPI1_ADS7028_INT_EXP3
        && ShimConfig_isGSREnabled())
    {
      GSR_range(&sensing.dataBuf[dataBufIndex]);
    }

    stopAds7028Conversions();
    break;
  default:
    break;
  }

  spi1Sens.status = SPI_STAT_IDLE;
  SpiSensing(&spi1Sens, SPI_NEXT_SENSOR);
}

void SPI2_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  switch (spi2Sens.sensorList[spi2Sens.sensorCnt])
  {
  case SPI2_LIS2DW12_ACCEL:
    lis2dw12_unselectDevice();
    memcpy(sensing.dataBuf + sensing.ptr.accel2,
        &spi2Sens_buf.lis2dw12AccelBuf[SPI_DMA_TXRX_OFFSET],
        sizeof(spi2Sens_buf.lis2dw12AccelBuf) - SPI_DMA_TXRX_OFFSET);
    break;
  case SPI2_LIS3MDL_MAG:
    lis3mdl_unselectDevice();
    memcpy(sensing.dataBuf + sensing.ptr.mag2,
        &spi2Sens_buf.lis3mdlMagBuf[SPI_DMA_TXRX_OFFSET],
        sizeof(spi2Sens_buf.lis3mdlMagBuf) - SPI_DMA_TXRX_OFFSET);
    break;
  default:
    break;
  }

  spi2Sens.status = SPI_STAT_IDLE;
  SpiSensing(&spi2Sens, SPI_NEXT_SENSOR);
}

void SPI3_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  //gConfigBytes *configBytes = ShimConfig_getStoredConfig();

#if !IS_EXG_DATA_READ_BLOCKING
  switch (spi3Sens.sensorList[spi3Sens.sensorCnt])
  {
  case SPI3_ADS1292R_EXG1:
    ADS1292_chip1CsEnable(0);
    ///Board_EXG_CHIP1_CS(1);
    ADS1292_readDataComplete();
    break;
  case SPI3_ADS1292R_EXG2:
    ADS1292_chip2CsEnable(0);
    //Board_EXG_CHIP2_CS(1);
    ADS1292_readDataComplete();
    break;
  default:
    break;
  }

  //spi3Sens.status = SPI_STAT_IDLE;
  //SpiSensing(&spi3Sens, SPI_NEXT_SENSOR);

  spi3Sens.sensorCnt++;
  if (spi3Sens.sensorCnt == spi3Sens.sensorLen)
  {
    spi3Sens.sensorCnt = 0;
  }
#endif
}

//void SPI3_TxCpltCallback(SPI_HandleTypeDef *hspi)
//{
//  EXG_spiTxIsr();
//}
//
//void SPI3_RxCpltCallback(SPI_HandleTypeDef *hspi)
//{
//  EXG_spiRxIsr();
//}

#elif defined(SHIMMER4_SDK)
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    EXG_spiTxIsr();
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    EXG_spiRxIsr();
  }
}
#endif

#if defined(SHIMMER3R)
void SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
#elif defined(SHIMMER4_SDK)
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
#endif
{
  __NOP();
  __NOP();
  __NOP();
  __NOP();
  __NOP();
}

#if defined(SHIMMER3R)
bool areSpiChannelsEnabled(void)
{
  return (spi1Sens.sensorLen + spi2Sens.sensorLen + spi3Sens.sensorLen) > 0 ? true : false;
}

void ads7028_configureChannels(uint8_t *channel_contents_ptr)
{
  gConfigBytes *configBytes = ShimConfig_getStoredConfig();
  memset(&spiAdc, 0x00, sizeof(spiAdc));

  //Select channel  and enable as per config
  //Internal ADC 0
  if (configBytes->chEnIntADC0)
  {
    *channel_contents_ptr++ = INTERNAL_ADC_0;
    sensing.nbrSpiChans += 1;
    sensing.ptr.intADC0 = sensing.dataLen;
    sensing.dataLen += 2;
    spiAdc.sensorList[spiAdc.sensorLen++] = INTERNAL_ADC_0;
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADS7028_INT_EXP0;
  }
  //Internal ADC 1
  if (configBytes->chEnIntADC1)
  {
    *channel_contents_ptr++ = INTERNAL_ADC_1;
    sensing.nbrSpiChans += 1;
    sensing.ptr.intADC1 = sensing.dataLen;
    sensing.dataLen += 2;
    spiAdc.sensorList[spiAdc.sensorLen++] = INTERNAL_ADC_1;
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADS7028_INT_EXP1;
  }
  //Internal ADC 2
  if (configBytes->chEnIntADC2)
  {
    *channel_contents_ptr++ = INTERNAL_ADC_2;
    sensing.nbrSpiChans += 1;
    sensing.ptr.intADC2 = sensing.dataLen;
    sensing.dataLen += 2;
    spiAdc.sensorList[spiAdc.sensorLen++] = INTERNAL_ADC_2;
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADS7028_INT_EXP2;
  }
  //Internal ADC 3
  if (configBytes->chEnIntADC3 || configBytes->chEnGsr)
  {
    if (configBytes->chEnGsr)
    {
      *channel_contents_ptr++ = GSR_RAW;
      sensing.ptr.gsr = sensing.dataLen;
      spiAdc.sensorList[spiAdc.sensorLen++] = GSR_RAW;
    }
    else
    {
      *channel_contents_ptr++ = INTERNAL_ADC_3;
      sensing.ptr.intADC3 = sensing.dataLen;
      spiAdc.sensorList[spiAdc.sensorLen++] = INTERNAL_ADC_3;
    }
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADS7028_INT_EXP3;
    sensing.nbrSpiChans += 1;
    sensing.dataLen += 2;
  }
  //External ADC 0
  if (configBytes->chEnExtADC0)
  {
    *channel_contents_ptr++ = EXTERNAL_ADC_0;
    sensing.nbrSpiChans += 1;
    sensing.ptr.extADC0 = sensing.dataLen;
    sensing.dataLen += 2;
    spiAdc.sensorList[spiAdc.sensorLen++] = EXTERNAL_ADC_0;
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADS7028_EXT_EXP0;
  }
  //External ADC 1
  if (configBytes->chEnExtADC1)
  {
    *channel_contents_ptr++ = EXTERNAL_ADC_1;
    sensing.nbrSpiChans += 1;
    sensing.ptr.extADC1 = sensing.dataLen;
    sensing.dataLen += 2;
    spiAdc.sensorList[spiAdc.sensorLen++] = EXTERNAL_ADC_1;
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADS7028_EXT_EXP1;
  }
  //External ADC 2
  if (configBytes->chEnExtADC2)
  {
    *channel_contents_ptr++ = EXTERNAL_ADC_2;
    sensing.nbrSpiChans += 1;
    sensing.ptr.extADC2 = sensing.dataLen;
    sensing.dataLen += 2;
    spiAdc.sensorList[spiAdc.sensorLen++] = EXTERNAL_ADC_2;
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADS7028_EXT_EXP2;
  }
  if (configBytes->chEnVBattery)
  {
    *channel_contents_ptr++ = VBATT;
    sensing.nbrSpiChans += 1;
    sensing.ptr.batteryAnalog = sensing.dataLen;
    sensing.dataLen += 2;
    spiAdc.sensorList[spiAdc.sensorLen++] = VBATT;
    spi1Sens.sensorList[spi1Sens.sensorLen++] = SPI1_ADS7028_VBATT_SENSE;
  }
}
#endif

/* USER CODE END 1 */
