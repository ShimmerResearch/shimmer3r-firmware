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

/* USER CODE END 0 */

MDF_HandleTypeDef MdfHandle4;
MDF_FilterConfigTypeDef MdfFilterConfig4;

/* MDF1 init function */
void MX_MDF1_Init(void)
{

  /* USER CODE BEGIN MDF1_Init 0 */

  /* USER CODE END MDF1_Init 0 */

  /* USER CODE BEGIN MDF1_Init 1 */

  /* USER CODE END MDF1_Init 1 */

  /**
    MdfHandle4 structure initialization and HAL_MDF_Init function call
  */
  MdfHandle4.Instance = MDF1_Filter4;
  MdfHandle4.Init.CommonParam.InterleavedFilters = 0;
  MdfHandle4.Init.CommonParam.ProcClockDivider = 1;
  MdfHandle4.Init.CommonParam.OutputClock.Activation = DISABLE;
  MdfHandle4.Init.SerialInterface.Activation = ENABLE;
  MdfHandle4.Init.SerialInterface.Mode = MDF_SITF_LF_MASTER_SPI_MODE;
  MdfHandle4.Init.SerialInterface.ClockSource = MDF_SITF_CKI_SOURCE;
  MdfHandle4.Init.SerialInterface.Threshold = 4;
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
  MdfFilterConfig4.CicMode = MDF_TWO_FILTERS_MCIC_FASTSINC;
  MdfFilterConfig4.DecimationRatio = 2;
  MdfFilterConfig4.Offset = 0;
  MdfFilterConfig4.Gain = 0;
  MdfFilterConfig4.ReshapeFilter.Activation = DISABLE;
  MdfFilterConfig4.HighPassFilter.Activation = DISABLE;
  MdfFilterConfig4.Integrator.Activation = DISABLE;
  MdfFilterConfig4.SoundActivity.Activation = DISABLE;
  MdfFilterConfig4.AcquisitionMode = MDF_MODE_ASYNC_CONT;
  MdfFilterConfig4.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig4.DiscardSamples = 0;
  /* USER CODE BEGIN MDF1_Init 2 */

  /* USER CODE END MDF1_Init 2 */

}

void HAL_MDF_MspInit(MDF_HandleTypeDef* mdfHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(IS_MDF_INSTANCE(mdfHandle->Instance))
  {
  /* USER CODE BEGIN MDF1_MspInit 0 */

  /* USER CODE END MDF1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_MDF1;
    PeriphClkInit.Mdf1ClockSelection = RCC_MDF1CLKSOURCE_PLL1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* MDF1 clock enable */
    __HAL_RCC_MDF1_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**MDF1 GPIO Configuration
    PC0     ------> MDF1_SDI4
    PC1     ------> MDF1_CKI4
    */
    GPIO_InitStruct.Pin = MIC_SD_Pin|MIC_CK_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_MDF1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MDF1_MspInit 1 */

  /* USER CODE END MDF1_MspInit 1 */
  }
}

void HAL_MDF_MspDeInit(MDF_HandleTypeDef* mdfHandle)
{

  if(IS_MDF_INSTANCE(mdfHandle->Instance))
  {
  /* USER CODE BEGIN MDF1_MspDeInit 0 */

  /* USER CODE END MDF1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_MDF1_CLK_DISABLE();

    /**MDF1 GPIO Configuration
    PC0     ------> MDF1_SDI4
    PC1     ------> MDF1_CKI4
    */
    HAL_GPIO_DeInit(GPIOC, MIC_SD_Pin|MIC_CK_Pin);

  /* USER CODE BEGIN MDF1_MspDeInit 1 */

  /* USER CODE END MDF1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
