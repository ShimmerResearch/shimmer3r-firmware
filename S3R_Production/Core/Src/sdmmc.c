/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    sdmmc.c
 * @brief   This file provides code for the configuration
 *          of the SDMMC instances.
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
#include "sdmmc.h"

/* USER CODE BEGIN 0 */
#include "gpio.h"
#include "stdio.h"

extern STATTypeDef stat;

/* USER CODE END 0 */

SD_HandleTypeDef hsd1;

/* SDMMC1 init function */

void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  //Initialise will fail if an SD card is not detected
  SD_insertedCheck();
  shimmerStatus.badFile = 1;
  if (shimmerStatus.isSdInserted)
  {
  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 2;
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDMMC1_Init 2 */
    else
    {
      shimmerStatus.badFile = 0;
    }
  }
  /* USER CODE END SDMMC1_Init 2 */

}

void HAL_SD_MspInit(SD_HandleTypeDef* sdHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(sdHandle->Instance==SDMMC1)
  {
  /* USER CODE BEGIN SDMMC1_MspInit 0 */

  /* USER CODE END SDMMC1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SDMMC|RCC_PERIPHCLK_CLK48;
    PeriphClkInit.IclkClockSelection = RCC_CLK48CLKSOURCE_HSI48;
    PeriphClkInit.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_CLK48;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* SDMMC1 clock enable */
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**SDMMC1 GPIO Configuration
    PD2     ------> SDMMC1_CMD
    PC11     ------> SDMMC1_D3
    PC12     ------> SDMMC1_CK
    PC10     ------> SDMMC1_D2
    PC9     ------> SDMMC1_D1
    PC8     ------> SDMMC1_D0
    */
    GPIO_InitStruct.Pin = SDMMC1_CMD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(SDMMC1_CMD_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SDMMC1_D3_Pin|SDMMC1_CK_Pin|SDMMC1_D2_Pin|SDMMC1_D1_Pin
                          |SDMMC1_D0_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* SDMMC1 interrupt Init */
    HAL_NVIC_SetPriority(SDMMC1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
  /* USER CODE BEGIN SDMMC1_MspInit 1 */

  /* USER CODE END SDMMC1_MspInit 1 */
  }
}

void HAL_SD_MspDeInit(SD_HandleTypeDef* sdHandle)
{

  if(sdHandle->Instance==SDMMC1)
  {
  /* USER CODE BEGIN SDMMC1_MspDeInit 0 */

  /* USER CODE END SDMMC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SDMMC1_CLK_DISABLE();

    /**SDMMC1 GPIO Configuration
    PD2     ------> SDMMC1_CMD
    PC11     ------> SDMMC1_D3
    PC12     ------> SDMMC1_CK
    PC10     ------> SDMMC1_D2
    PC9     ------> SDMMC1_D1
    PC8     ------> SDMMC1_D0
    */
    HAL_GPIO_DeInit(SDMMC1_CMD_GPIO_Port, SDMMC1_CMD_Pin);

    HAL_GPIO_DeInit(GPIOC, SDMMC1_D3_Pin|SDMMC1_CK_Pin|SDMMC1_D2_Pin|SDMMC1_D1_Pin
                          |SDMMC1_D0_Pin);

    /* SDMMC1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
  /* USER CODE BEGIN SDMMC1_MspDeInit 1 */

  /* USER CODE END SDMMC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void printSdCardInfo(char *outputStr)
{
  HAL_SD_CardCIDTypeDef pCID;
  HAL_SD_GetCardCID(&hsd1, &pCID);

  sprintf(outputStr, "Manufacturer: ");
  switch (pCID.ManufacturerID)
  {
  case 0x000001:
    sprintf(outputStr + strlen(outputStr), "Panasonic");
    break;
  case 0x000002:
    sprintf(outputStr + strlen(outputStr), "Toshiba");
    break;
  case 0x000003:
    sprintf(outputStr + strlen(outputStr), "SanDisk");
    break;
  case 0x00001b:
    sprintf(outputStr + strlen(outputStr), "Samsung");
    break;
  case 0x00001d:
    sprintf(outputStr + strlen(outputStr), "AData");
    break;
  case 0x000027:
    sprintf(outputStr + strlen(outputStr), "Phison");
    break;
  case 0x000028:
    sprintf(outputStr + strlen(outputStr), "Lexar");
    break;
  case 0x000031:
    sprintf(outputStr + strlen(outputStr), "Silicon Power");
    break;
  case 0x000041:
    sprintf(outputStr + strlen(outputStr), "Kingston");
    break;
  case 0x000074:
    sprintf(outputStr + strlen(outputStr), "Transcend");
    break;
  case 0x000076:
    sprintf(outputStr + strlen(outputStr), "Patriot");
    break;
  case 0x000082:
  case 0x00009c:
    sprintf(outputStr + strlen(outputStr), "Sony");
    break;
  default:
    break;
  }

  sprintf(outputStr + strlen(outputStr), ", Size=%.2lfGB",
      (hsd1.SdCard.BlockSize * ((float) hsd1.SdCard.BlockNbr / 1024 / 1024 / 1024)));
  sprintf(outputStr + strlen(outputStr), ", Manufacture Date=%d-%02d",
      (pCID.ManufactDate / 10) + 1990, pCID.ManufactDate % 10);

  sprintf(outputStr + strlen(outputStr), "\r\n");
}

/* USER CODE END 1 */
