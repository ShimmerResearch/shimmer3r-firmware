/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    dcache.c
 * @brief   This file provides code for the configuration
 *          of the ICACHE instances.
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
#include "dcache.h"

/* USER CODE BEGIN 0 */
DCACHE_HandleTypeDef dcache1;

/* USER CODE END 0 */

/* ICACHE init function */
void MX_DCACHE_Init(void)
{

  /* USER CODE BEGIN DCACHE_Init 0 */

  /* USER CODE END DCACHE_Init 0 */

  /* USER CODE BEGIN DCACHE_Init 1 */

  /* USER CODE END DCACHE_Init 1 */
  __HAL_RCC_DCACHE1_CLK_ENABLE();
  dcache1.Instance = DCACHE1;
  dcache1.Init.ReadBurstType = DCACHE_READ_BURST_WRAP;
  if (HAL_DCACHE_Init(&dcache1) != HAL_OK)
  {
    /* If DCACHE fails to init, DMA transfers will likely CORRUPT DATA.
       It is safer to stop here during development than to have
       random file system corruption later. */
    Error_Handler();
  }

  /* USER CODE BEGIN DCACHE_Init 2 */

  /* USER CODE END DCACHE_Init 2 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
