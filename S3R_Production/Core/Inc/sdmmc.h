/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    sdmmc.h
 * @brief   This file contains all the function prototypes for
 *          the sdmmc.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SDMMC_H__
#define __SDMMC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* USER CODE BEGIN Includes */

  /* USER CODE END Includes */

  extern SD_HandleTypeDef hsd1;

  /* USER CODE BEGIN Private defines */
  typedef enum {
      OWNER_IDLE = 0,
      OWNER_FATFS,
      OWNER_USB
  } sdOwner_t;
  /* USER CODE END Private defines */

  void MX_SDMMC1_SD_Init(void);

  /* USER CODE BEGIN Prototypes */

  void mmc1DeInit(void);
  void printSdCardInfo(char *outputStr);
  void printSdCardSize(char *outputStr);
  void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd1);
  void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd1);
  void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd1);
  HAL_StatusTypeDef HAL_SD_SharedRead(sdOwner_t requester, uint8_t *pData, uint32_t addr, uint32_t blocks);
  HAL_StatusTypeDef HAL_SD_SharedWrite(sdOwner_t requester, uint8_t *pData, uint32_t addr, uint32_t blocks);

  /* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __SDMMC_H__ */
