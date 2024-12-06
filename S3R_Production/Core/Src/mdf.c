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
#include <PCM/pcm_config.h>
#include <PCM/pcm_lowlevel.h>

/* USER CODE BEGIN 0 */

#include "gpdma.h"
#include "hal_Board.h"

int16_t micDataBuffer[DEFAULT_AUDIO_IN_BUFFER_SIZE];
MDF_DmaConfigTypeDef micDmaConfig;
int Mic_CountSkip;
/* USER CODE END 0 */

MDF_HandleTypeDef AdfHandle0;
MDF_FilterConfigTypeDef AdfFilterConfig0;

/* ADF1 init function */
void MX_ADF1_Init(void)
{

  /* USER CODE BEGIN ADF1_Init 0 */

  /* USER CODE END ADF1_Init 0 */

  /* USER CODE BEGIN ADF1_Init 1 */
  Board_SW_MIC(1);
  AdfHandle0.Init.CommonParam.OutputClock.Trigger.Edge = MDF_CLOCK_TRIG_RISING_EDGE;
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
  AdfFilterConfig0.DecimationRatio = 24;
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
  AdfFilterConfig0.Trigger.Edge = MDF_FILTER_TRIG_RISING_EDGE;
  AdfFilterConfig0.SnapshotFormat = MDF_SNAPSHOT_23BITS;
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
    PeriphClkInit.PLL3.PLL3FRACN = 0.0;
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
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_ADF1;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* ADF1 interrupt Init */
    HAL_NVIC_SetPriority(ADF1_IRQn, 0, 0);
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
    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_3 | GPIO_PIN_4);

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
  HAL_MDF_MspDeInit(&MdfHandle4);
#else
  HAL_MDF_MspDeInit(&AdfHandle0);
#endif
  Board_SW_MIC(0);
}

void micDmaStart(void)
{
  micDmaConfig.Address = (uint32_t) &micDataBuffer[0];
  micDmaConfig.DataLength = (DEFAULT_AUDIO_IN_BUFFER_SIZE * 2U);
  micDmaConfig.MsbOnly = ENABLE;

  if (HAL_MDF_AcqStart_DMA(&AdfHandle0, &AdfFilterConfig0, &micDmaConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_MDF_GenerateTrgo(&AdfHandle0) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_MDF_AcqCpltCallback(MDF_HandleTypeDef *hmdf)
{
  /*  __NOP();
    __NOP(); */
  if (Mic_CountSkip < 64)
  {
    ++Mic_CountSkip;
    return;
  }
  for (uint32_t j = 0U;
       j < ((AUDIO_IN_SAMPLING_FREQUENCY / 1000U) * N_MS_PER_INTERRUPT); j++)
  {
    int32_t Z = ((micDataBuffer[j + ((AUDIO_IN_SAMPLING_FREQUENCY / 1000U) * N_MS_PER_INTERRUPT)])
                    * 10U /*(int32_t)(PCM.Volume)*/)
        / 100;
    micDataBuffer[j] = (uint16_t) SaturaLH(Z, -32760, 32760);
  }
  HAL_MDF_AcqStop_DMA(hmdf);
}

void HAL_MDF_AcqHalfCpltCallback(MDF_HandleTypeDef *hmdf)
{
  if (Mic_CountSkip < 64)
  {
    ++Mic_CountSkip;
    return;
  }
  for (uint32_t j = 0U;
       j < ((AUDIO_IN_SAMPLING_FREQUENCY / 1000U) * N_MS_PER_INTERRUPT); j++)
  {
    int32_t Z = (micDataBuffer[j] * 10U /*(int32_t)(PCM.Volume)*/) / 100;
    micDataBuffer[j] = (uint16_t) SaturaLH(Z, -32760, 32760);
  }
}

uint8_t isMicrophoneEnabled(void)
{
  return S4Ram_getStoredConfig()->chEnMicrophone;
}

/* USER CODE END 1 */
