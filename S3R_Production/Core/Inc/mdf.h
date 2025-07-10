/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    mdf.h
 * @brief   This file contains all the function prototypes for
 *          the mdf.c file
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
#ifndef __MDF_H__
#define __MDF_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* USER CODE BEGIN Includes */

  /* USER CODE END Includes */

  extern MDF_HandleTypeDef AdfHandle0;

  extern MDF_FilterConfigTypeDef AdfFilterConfig0;

  /* USER CODE BEGIN Private defines */

#define MAX_BUFFER_SIZE                         32000

#define FACTORY_TEST_MIC_ERROR_CNT_THRESHOLD    500
#define FACTORY_TEST_MIC_PASS                   0xFF
#define FACTORY_TEST_MIC_FAIL_NO_DATA_IN_BUFFER 0xFE

  /* USER CODE END Private defines */

  void MX_ADF1_Init(void);

  /* USER CODE BEGIN Prototypes */

  void MDF1_DeInit(void);
  void micStartSensing(void);
  void HAL_MDF_AcqCpltCallback(MDF_HandleTypeDef *hmdf);
  void HAL_MDF_AcqHalfCpltCallback(MDF_HandleTypeDef *hmdf);
  void micStopSensing(void);
  uint8_t micTest(void);

  /* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __MDF_H__ */
