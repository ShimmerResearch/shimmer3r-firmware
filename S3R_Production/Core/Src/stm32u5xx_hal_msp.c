/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file         stm32u5xx_hal_msp.c
 * @brief        This file provides code for the MSP Initialization
 *               and de-Initialization codes.
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
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* U5 driver v1.7.0 added EnableVddUSB, EnableVddIO2 and EnableVddA here. We
   * previously had to add in EnableVddA ourselves based on online forums but
   * EnableVddUSB and EnableVddIO2 are also being enabled elsewhere.
   * Additionally EnableVddUSB will not work at this point with the SR48-6-0
   * prototypes due to a fault in the design. Sectioning off the CubeMX changes
   * for the moment.*/
  //#if defined(SR48_6_0_PATCH_VBUS_SENSE)
  //  __HAL_RCC_PWR_CLK_ENABLE();
  //  HAL_PWREx_EnableVddA();
  //#else

  /* USER CODE END MspInit 0 */

  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWREx_EnableVddUSB();
  HAL_PWREx_EnableVddIO2();
  HAL_PWREx_EnableVddA();

  /* System interrupt init*/

  /* USER CODE BEGIN MspInit 1 */

  //#endif

  /* USER CODE END MspInit 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
