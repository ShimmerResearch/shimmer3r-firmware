/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    app_usbx_device.h
 * @author  MCD Application Team
 * @brief   USBX Device applicative header file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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
#ifndef __APP_USBX_DEVICE_H__
#define __APP_USBX_DEVICE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "ux_api.h"
#include "ux_dcd_stm32.h"
#include "ux_device_cdc_acm.h"
#include "ux_device_descriptors.h"
#include "ux_device_msc.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb_otg.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/

/* USER CODE BEGIN EC */

/* Exported constants --------------------------------------------------------*/
/* USBX memory pool. With UX_SLAVE_REQUEST_DATA_MAX_LENGTH = 32KB and
 * core-managed endpoint buffers, the stack allocates one ~32KB buffer per
 * active endpoint plus framework/string/class overhead. ThreadX byte-pool
 * fragmentation + per-block headers means the practical need is noticeably
 * higher than the raw sum, so we allocate 384KB to be comfortable. RAM
 * budget is fine on STM32U5A5xJ (2.5MB SRAM). */
#define UX_DEVICE_APP_MEM_POOL_SIZE 1024 * 640
  //#define USBX_DEVICE_MEMORY_STACK_SIZE 1024 * 64

  /* USER CODE END EC */

  /* Exported macro ------------------------------------------------------------*/
  /* USER CODE BEGIN EM */

  /* USER CODE END EM */

  /* Exported functions prototypes ---------------------------------------------*/
  UINT MX_USBX_Device_Init(VOID);

  /* USER CODE BEGIN EFP */

  /* Exported functions prototypes ---------------------------------------------*/
  VOID USBX_Device_Process(VOID);
  VOID USBX_APP_Device_Init(VOID);
  VOID USBX_APP_Device_DeInit(VOID);
  UINT MX_USBX_Device_DeInit(VOID);
  bool USBX_IsInitialised(void);

  /* USER CODE END EFP */

  /* Private defines -----------------------------------------------------------*/
  /* USER CODE BEGIN PD */

  /* USER CODE END PD */

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /* __APP_USBX_DEVICE_H__ */
