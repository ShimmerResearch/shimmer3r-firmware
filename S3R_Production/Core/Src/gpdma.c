/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    gpdma.c
 * @brief   This file provides code for the configuration
 *          of the GPDMA instances.
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
#include "gpdma.h"

/* USER CODE BEGIN 0 */

#include "linked_list.h"

extern DMA_QListTypeDef ADCQueue;
extern DMA_QListTypeDef MicQueue;
/* USER CODE END 0 */

DMA_HandleTypeDef handle_GPDMA1_Channel3;
DMA_HandleTypeDef handle_GPDMA1_Channel2;

/* GPDMA1 init function */
void MX_GPDMA1_Init(void)
{

  /* USER CODE BEGIN GPDMA1_Init 0 */

  /* USER CODE END GPDMA1_Init 0 */

  /* Peripheral clock enable */
  __HAL_RCC_GPDMA1_CLK_ENABLE();

  /* GPDMA1 interrupt Init */
  HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel3_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel4_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel5_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel8_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel8_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel9_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel9_IRQn);
  HAL_NVIC_SetPriority(GPDMA1_Channel10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(GPDMA1_Channel10_IRQn);

  /* USER CODE BEGIN GPDMA1_Init 1 */

  /* USER CODE END GPDMA1_Init 1 */
  handle_GPDMA1_Channel3.Instance = GPDMA1_Channel3;
  handle_GPDMA1_Channel3.InitLinkedList.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
  handle_GPDMA1_Channel3.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
  handle_GPDMA1_Channel3.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
  handle_GPDMA1_Channel3.InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
  handle_GPDMA1_Channel3.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
  if (HAL_DMAEx_List_Init(&handle_GPDMA1_Channel3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel3, DMA_CHANNEL_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }
  handle_GPDMA1_Channel2.Instance = GPDMA1_Channel2;
  handle_GPDMA1_Channel2.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
  handle_GPDMA1_Channel2.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
  handle_GPDMA1_Channel2.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
  handle_GPDMA1_Channel2.InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
  handle_GPDMA1_Channel2.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;
  if (HAL_DMAEx_List_Init(&handle_GPDMA1_Channel2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel2, DMA_CHANNEL_NPRIV) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN GPDMA1_Init 2 */

  /* USER CODE END GPDMA1_Init 2 */
}

/* USER CODE BEGIN 1 */

void linkedListConfig(ADC_HandleTypeDef *hadc)
{
  MX_ADCQueue_Config();
  __HAL_LINKDMA(hadc, DMA_Handle, handle_GPDMA1_Channel2);
  if (HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel2, &ADCQueue) != HAL_OK)
  {
    Error_Handler();
  }
}

void micLinklistConfig(MDF_HandleTypeDef *MdfHandle)
{
  MX_MicQueue_Config();
  __HAL_LINKDMA(MdfHandle, hdma, handle_GPDMA1_Channel3);
  if (HAL_DMAEx_List_LinkQ(&handle_GPDMA1_Channel3, &MicQueue) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE END 1 */
