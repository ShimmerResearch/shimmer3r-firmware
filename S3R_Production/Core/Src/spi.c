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

SPI_HandleTypeDef *hspiExg;
//static SENSINGTypeDef *pSensing;

void (*SPI_gatherDataDone_cb)(void);

#if defined(SHIMMER3R)
bool spi1BusChipPwrFlags[SPI1_CHIP_QTY];
bool spi2BusChipPwrFlags[SPI2_CHIP_QTY];
bool spi3BusChipPwrFlags[SPI3_CHIP_QTY];

#endif

/* USER CODE END 0 */

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef handle_GPDMA1_Channel5;
DMA_HandleTypeDef handle_GPDMA1_Channel4;

/* SPI1 init function */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

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
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
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

  /* USER CODE END SPI1_Init 2 */

}
/* SPI2 init function */
void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_1LINE;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
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

  /* USER CODE END SPI2_Init 2 */

}
/* SPI3 init function */
void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

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

  /* USER CODE END SPI3_Init 2 */

}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(spiHandle->Instance==SPI1)
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
    PE13     ------> SPI1_SCK
    PE14     ------> SPI1_MISO
    PE15     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* GPDMA1_REQUEST_SPI1_TX Init */
    handle_GPDMA1_Channel5.Instance = GPDMA1_Channel5;
    handle_GPDMA1_Channel5.Init.Request = GPDMA1_REQUEST_SPI1_TX;
    handle_GPDMA1_Channel5.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel5.Init.Direction = DMA_MEMORY_TO_PERIPH;
    handle_GPDMA1_Channel5.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel5.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel5.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel5.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel5.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel5.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel5.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel5.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
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
    handle_GPDMA1_Channel4.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel4.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel4.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel4.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel4.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel4.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel4.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
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
  else if(spiHandle->Instance==SPI2)
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
    PB15     ------> SPI2_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI2_MspInit 1 */

  /* USER CODE END SPI2_MspInit 1 */
  }
  else if(spiHandle->Instance==SPI3)
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
    /**SPI3 GPIO Configuration
    PB3 (JTDO/TRACESWO)     ------> SPI3_SCK
    PB4 (NJTRST)     ------> SPI3_MISO
    PB5     ------> SPI3_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI3_MspInit 1 */

  /* USER CODE END SPI3_MspInit 1 */
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{

  if(spiHandle->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspDeInit 0 */

  /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PE13     ------> SPI1_SCK
    PE14     ------> SPI1_MISO
    PE15     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmatx);
    HAL_DMA_DeInit(spiHandle->hdmarx);

    /* SPI1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI1_IRQn);
  /* USER CODE BEGIN SPI1_MspDeInit 1 */

  /* USER CODE END SPI1_MspDeInit 1 */
  }
  else if(spiHandle->Instance==SPI2)
  {
  /* USER CODE BEGIN SPI2_MspDeInit 0 */

  /* USER CODE END SPI2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI2_CLK_DISABLE();

    /**SPI2 GPIO Configuration
    PB13     ------> SPI2_SCK
    PB15     ------> SPI2_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13|GPIO_PIN_15);

  /* USER CODE BEGIN SPI2_MspDeInit 1 */

  /* USER CODE END SPI2_MspDeInit 1 */
  }
  else if(spiHandle->Instance==SPI3)
  {
  /* USER CODE BEGIN SPI3_MspDeInit 0 */

  /* USER CODE END SPI3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();

    /**SPI3 GPIO Configuration
    PB3 (JTDO/TRACESWO)     ------> SPI3_SCK
    PB4 (NJTRST)     ------> SPI3_MISO
    PB5     ------> SPI3_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5);

  /* USER CODE BEGIN SPI3_MspDeInit 1 */

  /* USER CODE END SPI3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void SPI_init(void){
   hspiExg = &hspi1;
   //pSensing = S4Sens_getSensing();
}
uint8_t SPI_test(void){
   uint8_t ret_val = 0;
   EXG_init(hspiExg);
   ret_val |= EXG_test();
   return ret_val;
}

void SPI_configureChannels(){
   uint8_t *channel_contents_ptr = sensing.cc+sensing.ccLen;
   uint8_t nbr_spi_chans = 0;
   //ExG (spi)
   if ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
         (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) ||
         (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT) ||
         (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT)) {
      if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) {
         *channel_contents_ptr++ = EXG_ADS1292R_1_STATUS;
         *channel_contents_ptr++ = EXG_ADS1292R_1_CH1_24BIT;
         *channel_contents_ptr++ = EXG_ADS1292R_1_CH2_24BIT;
         nbr_spi_chans += 3;
         S4Ram_storedConfigSetByte(NV_SENSORS2, S4Ram_storedConfigGetByte(NV_SENSORS2) & ~SENSOR_EXG1_16BIT);
         sensing.ptr.exg1 = sensing.dataLen;
         sensing.dataLen += 7;
      } else if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT) {
         *channel_contents_ptr++ = EXG_ADS1292R_1_STATUS;
         *channel_contents_ptr++ = EXG_ADS1292R_1_CH1_16BIT;
         *channel_contents_ptr++ = EXG_ADS1292R_1_CH2_16BIT;
         nbr_spi_chans += 3;
         sensing.ptr.exg1 = sensing.dataLen;
         sensing.dataLen += 5;
      }
      if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) {
         *channel_contents_ptr++ = EXG_ADS1292R_2_STATUS;
         *channel_contents_ptr++ = EXG_ADS1292R_2_CH1_24BIT;
         *channel_contents_ptr++ = EXG_ADS1292R_2_CH2_24BIT;
         nbr_spi_chans += 3;
         S4Ram_storedConfigSetByte(NV_SENSORS2, S4Ram_storedConfigGetByte(NV_SENSORS2) & ~SENSOR_EXG2_16BIT);
         sensing.ptr.exg2 = sensing.dataLen;
         sensing.dataLen += 7;
      } else if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT) {
         *channel_contents_ptr++ = EXG_ADS1292R_2_STATUS;
         *channel_contents_ptr++ = EXG_ADS1292R_2_CH1_16BIT;
         *channel_contents_ptr++ = EXG_ADS1292R_2_CH2_16BIT;
         nbr_spi_chans += 3;
         sensing.ptr.exg2 = sensing.dataLen;
         sensing.dataLen += 5;
      }
   }
   sensing.ccLen += nbr_spi_chans;
   sensing.nbrDigiChans += nbr_spi_chans;
}
void SPI_startSensing(){
   static uint8_t temp_exg_buf[11];
   //ExG (SPI)
   if ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
       (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) ||
       (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT) ||
       (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT)) {
      EXG_init(hspiExg);
      if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT ||
          S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT) {
         S4Ram_storedConfigGet(temp_exg_buf, NV_EXG_ADS1292R_1_CONFIG1, 10);
         EXG_writeRegs(0, ADS1292R_CONFIG1, 10, temp_exg_buf);
         EXG_readRegs(0, 0, 11, temp_exg_buf);// can read back to check if write is done successfully
         __NOP();
         __NOP();
         __NOP();
      }
      if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT ||
          S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT) {
         HAL_Delay(100);   //100ms
         EXG_setRdatac(1, 0);
         S4Ram_storedConfigGet(temp_exg_buf, NV_EXG_ADS1292R_2_CONFIG1, 10);
         EXG_writeRegs(1, ADS1292R_CONFIG1, 10, temp_exg_buf);
         EXG_readRegs(1, 0, 11, temp_exg_buf);
         EXG_enableChip2(1);
         __NOP();
         __NOP();
         __NOP();
      }
      //probably turning on internal reference, so wait for it to settle
      HAL_Delay(100);   //100ms

      //probably setting the PGA gain so cancel the channel offset
      if (((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
           (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT)) &&
           (S4Ram_storedConfigGetByte(NV_EXG_ADS1292R_1_RESP2) & 0x80)) {
         EXG_offsetCal(0);
      }
      if (((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) ||
           (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT)) &&
           (S4Ram_storedConfigGetByte(NV_EXG_ADS1292R_2_RESP2) & 0x80)) {
         EXG_offsetCal(1);
      }

      if (((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
           (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT)) &&
          ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) ||
           (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT))) {
         EXG_start(2);
      } else if ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
                 (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT)) {
         EXG_start(0);
      } else {
         EXG_start(1);
      }
   }
}
//void SPI_startSensing(){
//   //ExG (SPI)
//   if ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
//       (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) ||
//       (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT) ||
//       (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT)) {
//      EXG_init(hspiExg);
//      if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT ||
//          S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT) {
//         S4Ram_storedConfigGet(temp_exg_buf, NV_EXG_ADS1292R_1_CONFIG1, 10);
//         EXG_writeRegs(0, ADS1292R_CONFIG1, 10, temp_exg_buf);
//         EXG_readRegs(0, 0, 11, temp_exg_buf);// can read back to check if write is done successfully
//         __NOP();
//         __NOP();
//         __NOP();
//         //probably turning on internal reference, so wait for it to settle
//         HAL_Delay(100);   //100ms
//         if (S4Ram_storedConfigGetByte(NV_EXG_ADS1292R_1_RESP2) & 0x80) {
//            EXG_offsetCal(0);
//         }
//         EXG_start(0);
//      }
//      HAL_Delay(100);   //100ms
//      if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT ||
//          S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT) {
//         EXG_setRdatac(1, 0);
//         S4Ram_storedConfigGet(temp_exg_buf, NV_EXG_ADS1292R_2_CONFIG1, 10);
//         EXG_writeRegs(1, ADS1292R_CONFIG1, 10, temp_exg_buf);
//         EXG_readRegs(1, 0, 11, temp_exg_buf);
//         __NOP();
//         __NOP();
//         __NOP();
//         //probably turning on internal reference, so wait for it to settle
//         HAL_Delay(100);   //100ms
//         EXG_enableChip2(1);
//         if (S4Ram_storedConfigGetByte(NV_EXG_ADS1292R_2_RESP2) & 0x80) {
//            EXG_offsetCal(1);
//         }
//         EXG_start(1);
//      }
//
//      ADS1292_enableDrdyInterrupts(ADS1292_DRDY_INT_CHIP1);
//      ADS1292_enableDrdyInterrupts(ADS1292_DRDY_INT_CHIP2);
//
//   }
//}
void SPI_pollSensors(){
   //exg
   if ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
       (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT)) {
      EXG_readData(0, 0, sensing.dataBuf + sensing.ptr.exg1);
   }
   if ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) ||
       (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT)) {
      EXG_readData(1, 0, sensing.dataBuf + sensing.ptr.exg2);
   }
}
void SPI_stopSensing(){
//   HAL_NVIC_EnableIRQ(EXTI3_IRQn);
//   HAL_NVIC_EnableIRQ(EXTI4_IRQn);
   if((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) ||
      (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT)) {
      EXG_stop(1);     //probably not needed
   }
   if((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
      (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT)) {
      EXG_stop(0);     //probably not needed
   }
//   if((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG1_24BIT) ||
//      (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_EXG2_24BIT) ||
//      (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG1_16BIT) ||
//      (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_EXG2_16BIT)) {
      EXG_powerOff();
//   }
   //HAL_SPI_MspDeInit(hspiExg);//this may save .2-.3 mA?
}

void SPI_gatherDataCb(void (*done_cb)(void)){
   SPI_gatherDataDone_cb = done_cb;
   EXG_gatherDataInit(SpiStep2Start);
}

void SPI_gatherDataStart(void){
   SpiStep1Start();
}

void SpiStep1Start(void){  EXG_gatherDataStart();}
void SpiStep2Start(void){  SpiStep3Start();}
void SpiStep3Start(void){  SpiStepDone();}
void SpiStepDone(void){    SPI_gatherDataDone_cb();}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
   if (hspi->Instance == SPI1) {
      EXG_spiTxIsr();
   }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
   if (hspi->Instance == SPI1) {
      EXG_spiRxIsr();
   }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi){
   __NOP();
   __NOP();
   __NOP();
   __NOP();
   __NOP();
}

#if defined(SHIMMER3R)
void set_power_spi1_bus(bool state, SPI1_CHIP_INDEX chipIndex)
{
  bool stateToSet = false;
  spi1BusChipPwrFlags[chipIndex] = state;

  for (uint8_t i = 0; i < sizeof(spi1BusChipPwrFlags); i++)
  {
    // If any chips should be on, set power on.
    if (spi1BusChipPwrFlags[i])
    {
      stateToSet = true;
      break;
    }
  }

  //TODO check polarity
  HAL_GPIO_WritePin(SW_SPI1_GPIO_Port, SW_SPI1_Pin, stateToSet ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void set_power_spi2_bus(bool state, SPI2_CHIP_INDEX chipIndex)
{
  bool stateToSet = false;
  spi2BusChipPwrFlags[chipIndex] = state;

  for (uint8_t i = 0; i < sizeof(spi2BusChipPwrFlags); i++)
  {
    // If any chips should be on, set power on.
    if (spi2BusChipPwrFlags[i])
    {
      stateToSet = true;
      break;
    }
  }

  //TODO check polarity
  HAL_GPIO_WritePin(SW_SPI2_GPIO_Port, SW_SPI2_Pin, stateToSet ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
#endif

/* USER CODE END 1 */
