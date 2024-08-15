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
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
#define CHG_LED_RD_Pin GPIO_PIN_6
#define CHG_LED_RD_GPIO_Port GPIOE
#define ACCEL_X_Pin GPIO_PIN_0
#define ACCEL_X_GPIO_Port GPIOC
#define ACCEL_Y_Pin GPIO_PIN_1
#define ACCEL_Y_GPIO_Port GPIOC
#define ACCEL_Z_Pin GPIO_PIN_2
#define ACCEL_Z_GPIO_Port GPIOC
#define DOCK_TXD_Pin GPIO_PIN_2
#define DOCK_TXD_GPIO_Port GPIOA
#define Accel_200G_x_Pin GPIO_PIN_5
#define Accel_200G_x_GPIO_Port GPIOA
#define Accel_200G_y_Pin GPIO_PIN_6
#define Accel_200G_y_GPIO_Port GPIOA
#define Accel_200G_z_Pin GPIO_PIN_7
#define Accel_200G_z_GPIO_Port GPIOA
#define BSL_TX_LED_GR1_Pin GPIO_PIN_1
#define BSL_TX_LED_GR1_GPIO_Port GPIOB
#define BSL_RX_LED_BLU_Pin GPIO_PIN_2
#define BSL_RX_LED_BLU_GPIO_Port GPIOB
#define SW_I2C_Pin GPIO_PIN_7
#define SW_I2C_GPIO_Port GPIOE
#define SW_FLASH_Pin GPIO_PIN_8
#define SW_FLASH_GPIO_Port GPIOE
#define BT_HOST_WAKE_Pin GPIO_PIN_11
#define BT_HOST_WAKE_GPIO_Port GPIOE
#define CHG_STAT1_Pin GPIO_PIN_12
#define CHG_STAT1_GPIO_Port GPIOE
#define CHG_STAT2_Pin GPIO_PIN_13
#define CHG_STAT2_GPIO_Port GPIOE
#define SD_DETECT_N_Pin GPIO_PIN_14
#define SD_DETECT_N_GPIO_Port GPIOE
#define SD_DETECT_N_EXTI_IRQn EXTI14_IRQn
#define BTH_TXD_Pin GPIO_PIN_8
#define BTH_TXD_GPIO_Port GPIOD
#define BTH_RXD_Pin GPIO_PIN_9
#define BTH_RXD_GPIO_Port GPIOD
#define BTH_FACTORY_Pin GPIO_PIN_10
#define BTH_FACTORY_GPIO_Port GPIOD
#define BTH_CTS_Pin GPIO_PIN_11
#define BTH_CTS_GPIO_Port GPIOD
#define BTH_RTS_Pin GPIO_PIN_12
#define BTH_RTS_GPIO_Port GPIOD
#define BTH_STATUS_Pin GPIO_PIN_13
#define BTH_STATUS_GPIO_Port GPIOD
#define BTH_RST_Pin GPIO_PIN_14
#define BTH_RST_GPIO_Port GPIOD
#define SW_BT_Pin GPIO_PIN_15
#define SW_BT_GPIO_Port GPIOD
#define CHG_LED_YE_Pin GPIO_PIN_6
#define CHG_LED_YE_GPIO_Port GPIOC
#define CHG_LED_GR_Pin GPIO_PIN_7
#define CHG_LED_GR_GPIO_Port GPIOC
#define SDMMC1_D0_Pin GPIO_PIN_8
#define SDMMC1_D0_GPIO_Port GPIOC
#define SDMMC1_D1_Pin GPIO_PIN_9
#define SDMMC1_D1_GPIO_Port GPIOC
#define USER_BTN_N_Pin GPIO_PIN_8
#define USER_BTN_N_GPIO_Port GPIOA
#define USER_BTN_N_EXTI_IRQn EXTI8_IRQn
#define BSL_TXD_Pin GPIO_PIN_9
#define BSL_TXD_GPIO_Port GPIOA
#define BSL_RXD_Pin GPIO_PIN_10
#define BSL_RXD_GPIO_Port GPIOA
#define SDMMC1_D2_Pin GPIO_PIN_10
#define SDMMC1_D2_GPIO_Port GPIOC
#define SDMMC1_D3_Pin GPIO_PIN_11
#define SDMMC1_D3_GPIO_Port GPIOC
#define SD_CLK_Pin GPIO_PIN_12
#define SD_CLK_GPIO_Port GPIOC
#define BT_DEV_WAKE_Pin GPIO_PIN_0
#define BT_DEV_WAKE_GPIO_Port GPIOD
#define SD_CMD_Pin GPIO_PIN_2
#define SD_CMD_GPIO_Port GPIOD
#define DOCK_DETECT_Pin GPIO_PIN_4
#define DOCK_DETECT_GPIO_Port GPIOD
#define DOCK_DETECT_EXTI_IRQn EXTI4_IRQn
#define DOCK_RXD_Pin GPIO_PIN_6
#define DOCK_RXD_GPIO_Port GPIOD
#define EXP_RESET_N_Pin GPIO_PIN_5
#define EXP_RESET_N_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
