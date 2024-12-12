/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    mdf.c
 * @brief   This file provides code for the configuration
 *          of the MDF instances.
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
#include "mdf.h"

/* USER CODE BEGIN 0 */
#include "gpdma.h"
#include "hal_Board.h"
#define AUDIO_IN_SAMPLING_FREQUENCY  16000U
#define DEFAULT_AUDIO_IN_BUFFER_SIZE (AUDIO_IN_SAMPLING_FREQUENCY / 1000) * 2U
#define SaturaLH(N, L, H)            (((N) < (L)) ? (L) : (((N) > (H)) ? (H) : (N)))

uint8_t micdata[DEFAULT_AUDIO_IN_BUFFER_SIZE] = { 0 };
uint16_t dataBuffer[DEFAULT_AUDIO_IN_BUFFER_SIZE / 2U];
MDF_DmaConfigTypeDef mdfDmaConfig;
/* USER CODE END 0 */

MDF_HandleTypeDef MdfHandle4;
MDF_FilterConfigTypeDef MdfFilterConfig4;

/* MDF1 init function */
void MX_MDF1_Init(void)
{

  /* USER CODE BEGIN MDF1_Init 0 */

  /* USER CODE END MDF1_Init 0 */
  Board_SW_MIC(1);
  HAL_Delay(100);
  /* USER CODE BEGIN MDF1_Init 1 */

  /* USER CODE END MDF1_Init 1 */

  /**
    MdfHandle4 structure initialization and HAL_MDF_Init function call
  */
  MdfHandle4.Instance = MDF1_Filter4;
  MdfHandle4.Init.CommonParam.InterleavedFilters = 0;
  MdfHandle4.Init.CommonParam.ProcClockDivider = 10;
  MdfHandle4.Init.CommonParam.OutputClock.Activation = ENABLE;
  MdfHandle4.Init.CommonParam.OutputClock.Pins = MDF_OUTPUT_CLOCK_0;
  MdfHandle4.Init.CommonParam.OutputClock.Divider = 10;
  MdfHandle4.Init.CommonParam.OutputClock.Trigger.Activation = ENABLE;
  MdfHandle4.Init.CommonParam.OutputClock.Trigger.Source = MDF_CLOCK_TRIG_TRGO;
  MdfHandle4.Init.CommonParam.OutputClock.Trigger.Edge = MDF_CLOCK_TRIG_FALLING_EDGE;
  MdfHandle4.Init.SerialInterface.Activation = ENABLE;
  MdfHandle4.Init.SerialInterface.Mode = MDF_SITF_NORMAL_SPI_MODE;
  MdfHandle4.Init.SerialInterface.ClockSource = MDF_SITF_CCK0_SOURCE;
  MdfHandle4.Init.SerialInterface.Threshold = 31;
  MdfHandle4.Init.FilterBistream = MDF_BITSTREAM4_RISING;
  if (HAL_MDF_Init(&MdfHandle4) != HAL_OK)
  {
    Error_Handler();
  }

  /**
    MdfFilterConfig4, MdfOldConfig4 and/or MdfScdConfig4 structures initialization

    WARNING : only structures are filled, no specific init function call for filter
  */
  MdfFilterConfig4.DataSource = MDF_DATA_SOURCE_BSMX;
  MdfFilterConfig4.Delay = 0;
  MdfFilterConfig4.CicMode = MDF_ONE_FILTER_SINC5;
  MdfFilterConfig4.DecimationRatio = 24;
  MdfFilterConfig4.Offset = 0;
  MdfFilterConfig4.Gain = 6;
  MdfFilterConfig4.ReshapeFilter.Activation = ENABLE;
  MdfFilterConfig4.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  MdfFilterConfig4.HighPassFilter.Activation = ENABLE;
  MdfFilterConfig4.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_000625FPCM;
  MdfFilterConfig4.Integrator.Activation = DISABLE;
  MdfFilterConfig4.SoundActivity.Activation = DISABLE;
  MdfFilterConfig4.AcquisitionMode = MDF_MODE_SYNC_CONT;
  MdfFilterConfig4.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig4.DiscardSamples = 0;
  MdfFilterConfig4.Trigger.Source = MDF_CLOCK_TRIG_TRGO;
  MdfFilterConfig4.Trigger.Edge = MDF_FILTER_TRIG_RISING_EDGE;
  /* USER CODE BEGIN MDF1_Init 2 */
  mdfDmaConfiguration();
  micLinklistConfig(&MdfHandle4);
  /* USER CODE END MDF1_Init 2 */
}

void HAL_MDF_MspInit(MDF_HandleTypeDef *mdfHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (IS_MDF_INSTANCE(mdfHandle->Instance))
  {
    /* USER CODE BEGIN MDF1_MspInit 0 */

    /* USER CODE END MDF1_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_MDF1;
    PeriphClkInit.Mdf1ClockSelection = RCC_MDF1CLKSOURCE_PLL3;
    PeriphClkInit.PLL3.PLL3Source = RCC_PLLSOURCE_HSE;
    PeriphClkInit.PLL3.PLL3M = 1;
    PeriphClkInit.PLL3.PLL3N = 24;
    PeriphClkInit.PLL3.PLL3P = 2;
    PeriphClkInit.PLL3.PLL3Q = 25;
    PeriphClkInit.PLL3.PLL3R = 2;
    PeriphClkInit.PLL3.PLL3RGE = RCC_PLLVCIRANGE_1;
    PeriphClkInit.PLL3.PLL3FRACN = 0.0;
    PeriphClkInit.PLL3.PLL3ClockOut = RCC_PLL3_DIVQ;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* MDF1 clock enable */
    __HAL_RCC_MDF1_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**MDF1 GPIO Configuration
    PC0     ------> MDF1_SDI4
    PC1     ------> MDF1_CKI4
    PE9     ------> MDF1_CCK0
    */
    GPIO_InitStruct.Pin = MIC_SD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_MDF1;
    HAL_GPIO_Init(MIC_SD_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = MIC_CK_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_MDF1;
    HAL_GPIO_Init(MIC_CK_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_EXTERNAL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_MDF1;
    HAL_GPIO_Init(GPIO_EXTERNAL_GPIO_Port, &GPIO_InitStruct);

    /* MDF1 interrupt Init */
    HAL_NVIC_SetPriority(MDF1_FLT4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(MDF1_FLT4_IRQn);
    /* USER CODE BEGIN MDF1_MspInit 1 */

    /* USER CODE END MDF1_MspInit 1 */
  }
}

void HAL_MDF_MspDeInit(MDF_HandleTypeDef *mdfHandle)
{

  if (IS_MDF_INSTANCE(mdfHandle->Instance))
  {
    /* USER CODE BEGIN MDF1_MspDeInit 0 */

    /* USER CODE END MDF1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_MDF1_CLK_DISABLE();

    /**MDF1 GPIO Configuration
    PC0     ------> MDF1_SDI4
    PC1     ------> MDF1_CKI4
    PE9     ------> MDF1_CCK0
    */
    HAL_GPIO_DeInit(GPIOC, MIC_SD_Pin | MIC_CK_Pin);

    HAL_GPIO_DeInit(GPIO_EXTERNAL_GPIO_Port, GPIO_EXTERNAL_Pin);

    /* MDF1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(MDF1_FLT4_IRQn);
    /* USER CODE BEGIN MDF1_MspDeInit 1 */

    /* USER CODE END MDF1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void MDF1_DeInit(void)
{
  HAL_MDF_MspDeInit(&MdfHandle4);
  Board_SW_MIC(0);
}

void startDataGather(void)
{

  HAL_Delay(100);
  mdfDmaConfiguration();
  if (HAL_MDF_AcqStart_DMA(&MdfHandle4, &MdfFilterConfig4, &mdfDmaConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_MDF_GenerateTrgo(&MdfHandle4) != HAL_OK)
  {
    Error_Handler();
  }
}

void mdfDmaConfiguration(void)
{
  mdfDmaConfig.Address = (uint32_t) &micdata[0];
  mdfDmaConfig.DataLength = (DEFAULT_AUDIO_IN_BUFFER_SIZE * 2U);
  ;
  mdfDmaConfig.MsbOnly = ENABLE;
}

void HAL_MDF_AcqHalfCpltCallback(MDF_HandleTypeDef *hmdf)
{
  UNUSED(hmdf);

  for (uint32_t j = 0U; j < 16U; j++)
  {
    int32_t Z = (micdata[j] * 50) / 100;
    dataBuffer[j] = (uint16_t) SaturaLH(Z, -32760, 32760);
  }
}

void HAL_MDF_AcqCpltCallback(MDF_HandleTypeDef *hmdf)
{
  //printf("here");
  for (uint32_t j = 0U; j < 16U; j++)
  {
    int32_t Z = ((micdata[j + 16]) * 50) / 100;
    dataBuffer[j] = (uint16_t) SaturaLH(Z, -32760, 32760);
  }
  //HAL_MDF_AcqStop_DMA(&MdfHandle4);
  //mdfDmaConfiguration();
  //TODO: Take care of the data here.
}

/* USER CODE END 1 */
