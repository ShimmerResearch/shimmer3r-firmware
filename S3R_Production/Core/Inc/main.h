/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

  /* Private includes ----------------------------------------------------------*/
  /* USER CODE BEGIN Includes */

#include "shimmer_definitions.h"

  /* USER CODE END Includes */

  /* Exported types ------------------------------------------------------------*/
  /* USER CODE BEGIN ET */

  /* USER CODE END ET */

  /* Exported constants --------------------------------------------------------*/
  /* USER CODE BEGIN EC */

  /* USER CODE END EC */

  /* Exported macro ------------------------------------------------------------*/
  /* USER CODE BEGIN EM */

  /* USER CODE END EM */

  /* Exported functions prototypes ---------------------------------------------*/
  void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_LWR_BLU_Pin             GPIO_PIN_5
#define LED_LWR_BLU_GPIO_Port       GPIOE
#define LED_LWR_RD_Pin              GPIO_PIN_3
#define LED_LWR_RD_GPIO_Port        GPIOE
#define DOCK_TXD_Pin                GPIO_PIN_6
#define DOCK_TXD_GPIO_Port          GPIOB
#define CS_BMP390_Pin               GPIO_PIN_6
#define CS_BMP390_GPIO_Port         GPIOD
#define GPIO_INTERNAL2_Pin          GPIO_PIN_5
#define GPIO_INTERNAL2_GPIO_Port    GPIOD
#define SDMMC1_CMD_Pin              GPIO_PIN_2
#define SDMMC1_CMD_GPIO_Port        GPIOD
#define SDMMC1_D3_Pin               GPIO_PIN_11
#define SDMMC1_D3_GPIO_Port         GPIOC
#define LED_LWR_GR_Pin              GPIO_PIN_4
#define LED_LWR_GR_GPIO_Port        GPIOE
#define LIS3MDL_DRDY_Pin            GPIO_PIN_2
#define LIS3MDL_DRDY_GPIO_Port      GPIOE
#define GPIO_INTERNAL1_Pin          GPIO_PIN_4
#define GPIO_INTERNAL1_GPIO_Port    GPIOD
#define SW_FLASH_Pin                GPIO_PIN_1
#define SW_FLASH_GPIO_Port          GPIOD
#define SDMMC1_CK_Pin               GPIO_PIN_12
#define SDMMC1_CK_GPIO_Port         GPIOC
#define SDMMC1_D2_Pin               GPIO_PIN_10
#define SDMMC1_D2_GPIO_Port         GPIOC
#define CS_LIS3MDL_Pin              GPIO_PIN_6
#define CS_LIS3MDL_GPIO_Port        GPIOE
#define GPIO_INTERNAL0_Pin          GPIO_PIN_13
#define GPIO_INTERNAL0_GPIO_Port    GPIOC
#define SW_GSR_Pin                  GPIO_PIN_0
#define SW_GSR_GPIO_Port            GPIOE
#define SW_SENSE_Pin                GPIO_PIN_3
#define SW_SENSE_GPIO_Port          GPIOD
#define SD_DETECT_N_Pin             GPIO_PIN_0
#define SD_DETECT_N_GPIO_Port       GPIOD
#define CHG_STAT1_Pin               GPIO_PIN_0
#define CHG_STAT1_GPIO_Port         GPIOF
#define DOCK_RXD_Pin                GPIO_PIN_7
#define DOCK_RXD_GPIO_Port          GPIOB
#define CS_LIS2DW12_Pin             GPIO_PIN_7
#define CS_LIS2DW12_GPIO_Port       GPIOD
#define USB_VBUS_Pin                GPIO_PIN_9
#define USB_VBUS_GPIO_Port          GPIOA
#define CS_HIGH_G_Pin               GPIO_PIN_10
#define CS_HIGH_G_GPIO_Port         GPIOA
#define SW_MIC_Pin                  GPIO_PIN_8
#define SW_MIC_GPIO_Port            GPIOA
#define CHG_STAT2_Pin               GPIO_PIN_1
#define CHG_STAT2_GPIO_Port         GPIOF
#define SW_BT_Pin                   GPIO_PIN_7
#define SW_BT_GPIO_Port             GPIOC
#define SDMMC1_D1_Pin               GPIO_PIN_9
#define SDMMC1_D1_GPIO_Port         GPIOC
#define SDMMC1_D0_Pin               GPIO_PIN_8
#define SDMMC1_D0_GPIO_Port         GPIOC
#define CS_LSM6DSV_Pin              GPIO_PIN_2
#define CS_LSM6DSV_GPIO_Port        GPIOC
#define VBAT_SENSE_Pin              GPIO_PIN_3
#define VBAT_SENSE_GPIO_Port        GPIOC
#define BT_CP_ROLE_Pin              GPIO_PIN_6
#define BT_CP_ROLE_GPIO_Port        GPIOC
#define DOCK_DETECT_Pin             GPIO_PIN_1
#define DOCK_DETECT_GPIO_Port       GPIOA
#define BT_CONNECTION_Pin           GPIO_PIN_14
#define BT_CONNECTION_GPIO_Port     GPIOD
#define BT_RST_Pin                  GPIO_PIN_13
#define BT_RST_GPIO_Port            GPIOD
#define BT_CYSPP_Pin                GPIO_PIN_15
#define BT_CYSPP_GPIO_Port          GPIOD
#define LED_UPR_RD_Pin              GPIO_PIN_0
#define LED_UPR_RD_GPIO_Port        GPIOA
#define DETECT_N_Pin                GPIO_PIN_5
#define DETECT_N_GPIO_Port          GPIOC
#define BT_HOST_WAKE_Pin            GPIO_PIN_8
#define BT_HOST_WAKE_GPIO_Port      GPIOE
#define LIS2MDL_DRDY_Pin            GPIO_PIN_10
#define LIS2MDL_DRDY_GPIO_Port      GPIOE
#define SW_SENSE_IO_Pin             GPIO_PIN_12
#define SW_SENSE_IO_GPIO_Port       GPIOE
#define BT_RXD_Pin                  GPIO_PIN_9
#define BT_RXD_GPIO_Port            GPIOD
#define BT_CTS_Pin                  GPIO_PIN_11
#define BT_CTS_GPIO_Port            GPIOD
#define BT_RTS_Pin                  GPIO_PIN_12
#define BT_RTS_GPIO_Port            GPIOD
#define BSL_TX_Pin                  GPIO_PIN_2
#define BSL_TX_GPIO_Port            GPIOA
#define GPIO_ADC_INT_EXP3_Pin       GPIO_PIN_2
#define GPIO_ADC_INT_EXP3_GPIO_Port GPIOB
#define LSM6DSV_INT1_Pin            GPIO_PIN_7
#define LSM6DSV_INT1_GPIO_Port      GPIOE
#define LED_UPR_GR_Pin              GPIO_PIN_10
#define LED_UPR_GR_GPIO_Port        GPIOB
#define BSL_RX_Pin                  GPIO_PIN_3
#define BSL_RX_GPIO_Port            GPIOA
#define USER_BTN_Pin                GPIO_PIN_6
#define USER_BTN_GPIO_Port          GPIOA
#define USER_BTN_EXTI_IRQn          EXTI6_IRQn
#define GPIO_ADC_INT_EXP2_Pin       GPIO_PIN_1
#define GPIO_ADC_INT_EXP2_GPIO_Port GPIOB
#define BMP390_INT_Pin              GPIO_PIN_11
#define BMP390_INT_GPIO_Port        GPIOE
#define LED_UPR_BLU_Pin             GPIO_PIN_11
#define LED_UPR_BLU_GPIO_Port       GPIOB
#define LIS2DW12_INT1_Pin           GPIO_PIN_12
#define LIS2DW12_INT1_GPIO_Port     GPIOB
#define BT_TXD_Pin                  GPIO_PIN_8
#define BT_TXD_GPIO_Port            GPIOD
#define SW_SD_MCU_DOCK_Pin          GPIO_PIN_4
#define SW_SD_MCU_DOCK_GPIO_Port    GPIOC
#define GPIO_ADC_INT_EXP1_Pin       GPIO_PIN_0
#define GPIO_ADC_INT_EXP1_GPIO_Port GPIOB
#define GPIO_EXTERNAL_Pin           GPIO_PIN_9
#define GPIO_EXTERNAL_GPIO_Port     GPIOE
#define BT_LP_MODE_Pin              GPIO_PIN_10
#define BT_LP_MODE_GPIO_Port        GPIOD

/* USER CODE BEGIN Private defines */

/* TODO including these expansion board pin definitions here because CubeMX
 * randomly stopped automatically generating them */
#define GPIO_ADC_EXT_EXP0_Pin       GPIO_PIN_4
#define GPIO_ADC_EXT_EXP0_GPIO_Port GPIOA

#define GPIO_ADC_EXT_EXP1_Pin       GPIO_PIN_1
#define GPIO_ADC_EXT_EXP1_GPIO_Port GPIOC

#define GPIO_ADC_EXT_EXP2_Pin       GPIO_PIN_7
#define GPIO_ADC_EXT_EXP2_GPIO_Port GPIOA

#define GPIO_ADC_INT_EXP0_Pin       GPIO_PIN_5
#define GPIO_ADC_INT_EXP0_GPIO_Port GPIOA

#if SR48_6_0_PATCH_DOCK_DETECT
#define BOOT0_USER_BTN_Pin       GPIO_PIN_3
#define BOOT0_USER_BTN_GPIO_Port GPIOH
#define BOOT0_USER_BTN_EXTI_IRQn EXTI3_IRQn
#endif

#ifdef SR48_6_0
#define SR48_6_0_LED_UPR_GR_Pin        GPIO_PIN_2
#define SR48_6_0_LED_UPR_GR_GPIO_Port  GPIOA
#define SR48_6_0_LED_UPR_BLU_Pin       GPIO_PIN_3
#define SR48_6_0_LED_UPR_BLU_GPIO_Port GPIOA

#define SR48_6_0_CHG_STAT1_Pin         GPIO_PIN_10
#define SR48_6_0_CHG_STAT1_GPIO_Port   GPIOB
#define SR48_6_0_CHG_STAT2_Pin         GPIO_PIN_11
#define SR48_6_0_CHG_STAT2_GPIO_Port   GPIOB
#endif

  /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
