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
#include "hal_Audio.h"
#include "hal_Board.h"

/* USER CODE END 0 */

MDF_HandleTypeDef AdfHandle0;
MDF_FilterConfigTypeDef AdfFilterConfig0;

/* ADF1 init function */
void MX_ADF1_Init(void)
{

  /* USER CODE BEGIN ADF1_Init 0 */
  //AdfFilterConfig0.DecimationRatio = 24 is changed to value of 64 so that mic tests pass
  /* USER CODE END ADF1_Init 0 */

  /* USER CODE BEGIN ADF1_Init 1 */

  Board_setMicPower(1);

  /* USER CODE END ADF1_Init 1 */

  /**
    AdfHandle0 structure initialization and HAL_MDF_Init function call
  */
  AdfHandle0.Instance = ADF1_Filter0;
  AdfHandle0.Init.CommonParam.ProcClockDivider = 1;
  AdfHandle0.Init.CommonParam.OutputClock.Activation = ENABLE;
  AdfHandle0.Init.CommonParam.OutputClock.Pins = MDF_OUTPUT_CLOCK_0;
  AdfHandle0.Init.CommonParam.OutputClock.Divider = 10;
  AdfHandle0.Init.CommonParam.OutputClock.Trigger.Activation = ENABLE;
  AdfHandle0.Init.CommonParam.OutputClock.Trigger.Source = MDF_CLOCK_TRIG_TRGO;
  AdfHandle0.Init.CommonParam.OutputClock.Trigger.Edge = MDF_CLOCK_TRIG_FALLING_EDGE;
  AdfHandle0.Init.SerialInterface.Activation = ENABLE;
  AdfHandle0.Init.SerialInterface.Mode = MDF_SITF_NORMAL_SPI_MODE;
  AdfHandle0.Init.SerialInterface.ClockSource = MDF_SITF_CCK0_SOURCE;
  AdfHandle0.Init.SerialInterface.Threshold = 31;
  AdfHandle0.Init.FilterBistream = MDF_BITSTREAM0_FALLING;
  if (HAL_MDF_Init(&AdfHandle0) != HAL_OK)
  {
    Error_Handler();
  }

  /**
    AdfFilterConfig0 structure initialization

    WARNING : only structure is filled, no specific init function call for filter
  */
  AdfFilterConfig0.DataSource = MDF_DATA_SOURCE_BSMX;
  AdfFilterConfig0.Delay = 0;
  AdfFilterConfig0.CicMode = MDF_ONE_FILTER_SINC5;
  AdfFilterConfig0.DecimationRatio = 64;
  AdfFilterConfig0.Gain = 6;
  AdfFilterConfig0.ReshapeFilter.Activation = ENABLE;
  AdfFilterConfig0.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  AdfFilterConfig0.HighPassFilter.Activation = ENABLE;
  AdfFilterConfig0.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_000625FPCM;
  AdfFilterConfig0.SoundActivity.Activation = DISABLE;
  AdfFilterConfig0.AcquisitionMode = MDF_MODE_SYNC_CONT;
  AdfFilterConfig0.FifoThreshold = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  AdfFilterConfig0.DiscardSamples = 0;
  AdfFilterConfig0.Trigger.Source = MDF_CLOCK_TRIG_TRGO;
  AdfFilterConfig0.Trigger.Edge = MDF_FILTER_TRIG_RISING_EDGE;
  /* USER CODE BEGIN ADF1_Init 2 */
  AdfFilterConfig0.SnapshotFormat = MDF_SNAPSHOT_23BITS;
  AdfFilterConfig0.DecimationRatio = 64;
  /* Sinc5 gain is D^5, so D=64 is ~43 dB hotter than the original D=24.
   * Scale back by 14 x ~3 dB so a typical mic offset no longer saturates. */
  AdfFilterConfig0.Gain = -8;
  /* Decimation, gain and the DMA ring are set per capture by hal_Audio.c, which
   * relinks channel 11 to its own queue (DEV-99) */
  micLinkedListConfig(&AdfHandle0);

  /* USER CODE END ADF1_Init 2 */
}

void HAL_MDF_MspInit(MDF_HandleTypeDef *mdfHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (IS_ADF_INSTANCE(mdfHandle->Instance))
  {
    /* USER CODE BEGIN ADF1_MspInit 0 */
    __HAL_RCC_ADF1_CONFIG(RCC_ADF1CLKSOURCE_PLL3);
    /* USER CODE END ADF1_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADF1;
    PeriphClkInit.Adf1ClockSelection = RCC_ADF1CLKSOURCE_PLL3;
    PeriphClkInit.PLL3.PLL3Source = RCC_PLLSOURCE_HSE;
    PeriphClkInit.PLL3.PLL3M = 1;
    PeriphClkInit.PLL3.PLL3N = 24;
    PeriphClkInit.PLL3.PLL3P = 2;
    PeriphClkInit.PLL3.PLL3Q = 25;
    PeriphClkInit.PLL3.PLL3R = 2;
    PeriphClkInit.PLL3.PLL3RGE = RCC_PLLVCIRANGE_1;
    PeriphClkInit.PLL3.PLL3FRACN = 0;
    PeriphClkInit.PLL3.PLL3ClockOut = RCC_PLL3_DIVQ;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* ADF1 clock enable */
    __HAL_RCC_ADF1_CLK_ENABLE();

    __HAL_RCC_GPIOF_CLK_ENABLE();
    /**ADF1 GPIO Configuration
    PF3     ------> ADF1_CCK0
    PF4     ------> ADF1_SDI0
    */
    GPIO_InitStruct.Pin = MIC_CK_Pin | MIC_SD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_ADF1;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* ADF1 interrupt Init */
    HAL_NVIC_SetPriority(ADF1_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(ADF1_IRQn);
    /* USER CODE BEGIN ADF1_MspInit 1 */

    /* USER CODE END ADF1_MspInit 1 */
  }
}

void HAL_MDF_MspDeInit(MDF_HandleTypeDef *mdfHandle)
{

  if (IS_ADF_INSTANCE(mdfHandle->Instance))
  {
    /* USER CODE BEGIN ADF1_MspDeInit 0 */

    /* USER CODE END ADF1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADF1_CLK_DISABLE();

    /**ADF1 GPIO Configuration
    PF3     ------> ADF1_CCK0
    PF4     ------> ADF1_SDI0
    */
    HAL_GPIO_DeInit(GPIOF, MIC_CK_Pin | MIC_SD_Pin);

    /* ADF1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(ADF1_IRQn);
    /* USER CODE BEGIN ADF1_MspDeInit 1 */

    /* USER CODE END ADF1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void MDF1_DeInit(void)
{
  //TODO remove IF when fully switched from eval board to BGA variant
#ifdef S3R_NUCLEO
  HAL_MDF_DeInit(&MdfHandle4);
#else
  HAL_MDF_DeInit(&AdfHandle0);
#endif
  Board_setMicPower(0);
}

/* GPDMA1 channel 11 finished one block of the capture ring (one interrupt per
 * node; half-transfer is disabled). hal_Audio.c owns the capture. */
void HAL_MDF_AcqCpltCallback(MDF_HandleTypeDef *hmdf)
{
  UNUSED(hmdf);
  AudioCapture_onDmaBlock();
}

void HAL_MDF_AcqHalfCpltCallback(MDF_HandleTypeDef *hmdf)
{
  UNUSED(hmdf);
}

void HAL_MDF_ErrorCallback(MDF_HandleTypeDef *hmdf)
{
  UNUSED(hmdf);
  AudioCapture_onAdfError();
}

/* USER CODE END 1 */
