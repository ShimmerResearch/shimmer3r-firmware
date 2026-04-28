/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    usb_otg.h
 * @brief   This file contains all the function prototypes for
 *          the usb_otg.c file
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
#ifndef __USB_OTG_H__
#define __USB_OTG_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* USER CODE BEGIN Includes */
#include <shimmer_include.h>
  /* USER CODE END Includes */

  extern PCD_HandleTypeDef hpcd_USB_OTG_HS;

  /* USER CODE BEGIN Private defines */

  /**
   * USB bus speed selection.  The STM32U5 OTG_HS peripheral with the
   * embedded HS PHY can enumerate either at High-Speed (480 Mbps) or be
   * forced to Full-Speed (12 Mbps) via PCD_SPEED_FULL.  The selected
   * value feeds both hpcd_USB_OTG_HS.Init.speed and the FIFO allocation
   * in USBX_APP_Device_Init().  Set this BEFORE calling USB_init().
   */
  typedef enum
  {
    USB_SPEED_HIGH = 0, /*!< USB 2.0 High-Speed (480 Mbps) */
    USB_SPEED_FULL = 1, /*!< USB 2.0 Full-Speed (12 Mbps) – default */
  } USB_Speed_t;

  /* USER CODE END Private defines */
  void MX_USB_OTG_HS_PCD_Init(void);

  /* USER CODE BEGIN Prototypes */

  /** Select the USB speed used on the next USB_init(). */
  void USB_setSpeed(USB_Speed_t speed);
  /** Return the currently-selected USB speed. */
  USB_Speed_t USB_getSpeed(void);

#if SUPPORT_SR48_6_0
  void MX_USB_OTG_HS_PCD_Init_NoVbusSense(void);
  void HAL_PCD_MspInit_NoVbusSense(PCD_HandleTypeDef *hpcd);
  void HAL_PCD_MspDeInit_NoVbusSense(PCD_HandleTypeDef *hpcd);
#endif
  void USB_init(void);
  void USB_deinit(void);

  /* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USB_OTG_H__ */
