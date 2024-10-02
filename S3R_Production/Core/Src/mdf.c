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

#include "hal_Board.h"
uint8_t data[100] ={0} ;

#define MDF_DECIMATION_RATIO(__FREQUENCY__) \
  ((__FREQUENCY__) == (AUDIO_FREQUENCY_8K)) ? (512U) \
  : ((__FREQUENCY__) == (AUDIO_FREQUENCY_11K)) ? (256U) \
  : ((__FREQUENCY__) == (AUDIO_FREQUENCY_16K)) ? (176U) \
  : ((__FREQUENCY__) == (AUDIO_FREQUENCY_22K)) ? (128U) \
  : ((__FREQUENCY__) == (AUDIO_FREQUENCY_32K)) ? (88U) \
  : ((__FREQUENCY__) == (AUDIO_FREQUENCY_44K)) ? (64U) \
  : ((__FREQUENCY__) == (AUDIO_FREQUENCY_48K)) ? (44U) : (128U)

MDF_DmaConfigTypeDef mdfDmaConfig;
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
  MdfHandle4.Init.SerialInterface.Mode = MDF_SITF_MANCHESTER_FALLING_MODE;
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
  MdfFilterConfig4.DiscardSamples = 1;
  /* USER CODE BEGIN MDF1_Init 2 */

  MdfFilterConfig4.ReshapeFilter.Activation      = DISABLE;
  MdfFilterConfig4.ReshapeFilter.DecimationRatio = MDF_RSF_DECIMATION_RATIO_4;
  MdfFilterConfig4.HighPassFilter.Activation      = DISABLE;
  MdfFilterConfig4.HighPassFilter.CutOffFrequency = MDF_HPF_CUTOFF_0_000625FPCM;
  MdfFilterConfig4.Integrator.Activation     = DISABLE;
  MdfFilterConfig4.Integrator.Value          = 4U;
  MdfFilterConfig4.Integrator.OutputDivision = MDF_INTEGRATOR_OUTPUT_NO_DIV;
  MdfFilterConfig4.SoundActivity.Activation           = DISABLE;
  MdfFilterConfig4.SoundActivity.Mode                 = MDF_SAD_VOICE_ACTIVITY_DETECTOR;
  MdfFilterConfig4.SoundActivity.FrameSize            = MDF_SAD_8_PCM_SAMPLES;
  MdfFilterConfig4.SoundActivity.Hysteresis           = DISABLE;
  MdfFilterConfig4.SoundActivity.SoundTriggerEvent    = MDF_SAD_ENTER_DETECT;
  MdfFilterConfig4.SoundActivity.DataMemoryTransfer   = MDF_SAD_NO_MEMORY_TRANSFER;
  MdfFilterConfig4.SoundActivity.MinNoiseLevel        = 0U;
  MdfFilterConfig4.SoundActivity.HangoverWindow       = MDF_SAD_HANGOVER_4_FRAMES;
  MdfFilterConfig4.SoundActivity.LearningFrames       = MDF_SAD_LEARNING_2_FRAMES;
  MdfFilterConfig4.SoundActivity.AmbientNoiseSlope    = 0U;
  MdfFilterConfig4.SoundActivity.SignalNoiseThreshold = MDF_SAD_SIGNAL_NOISE_18DB;
  MdfFilterConfig4.AcquisitionMode = MDF_MODE_ASYNC_CONT;
  MdfFilterConfig4.FifoThreshold   = MDF_FIFO_THRESHOLD_NOT_EMPTY;
  MdfFilterConfig4.DiscardSamples  = 1U;
  MdfFilterConfig4.Trigger.Source  = MDF_FILTER_TRIG_TRGO;
  MdfFilterConfig4.Trigger.Edge    = MDF_FILTER_TRIG_RISING_EDGE;
  MdfFilterConfig4.SnapshotFormat  = MDF_SNAPSHOT_23BITS;

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
    PeriphClkInit.Mdf1ClockSelection = RCC_MDF1CLKSOURCE_MSIK;
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

    /* MDF1 interrupt Init */
    HAL_NVIC_SetPriority(MDF1_FLT4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(MDF1_FLT4_IRQn);
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

  mdfDmaConfig.Address = (uint32_t)data;
  mdfDmaConfig.DataLength = 10U;
 mdfDmaConfig.MsbOnly = ENABLE;
  HAL_MDF_AcqStart_DMA(&MdfHandle4, &MdfFilterConfig4, &mdfDmaConfig);
}
void getData(int32_t * buf)
{
 // HAL_MDF_PollForAcq(&MdfHandle4, 500);
  HAL_MDF_GetAcqValue(&MdfHandle4, buf);
}
void HAL_MDF_AcqCpltCallback(MDF_HandleTypeDef *hmdf)
{
  printf("here");
}
/* USER CODE END 1 */
