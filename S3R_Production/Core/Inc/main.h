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
#define SD_PC_SWITCH_Pin GPIO_PIN_2
#define SD_PC_SWITCH_GPIO_Port GPIOE
#define LED_LWR_RD_Pin GPIO_PIN_3
#define LED_LWR_RD_GPIO_Port GPIOE
#define LED_LWR_GR_Pin GPIO_PIN_4
#define LED_LWR_GR_GPIO_Port GPIOE
#define LED_LWR_BLU_Pin GPIO_PIN_5
#define LED_LWR_BLU_GPIO_Port GPIOE
#define BT_CYSPP_Pin GPIO_PIN_6
#define BT_CYSPP_GPIO_Port GPIOE
#define BT_CYSPP_EXTI_IRQn EXTI6_IRQn
#define USER_BTN_N_Pin GPIO_PIN_13
#define USER_BTN_N_GPIO_Port GPIOC
#define USER_BTN_N_EXTI_IRQn EXTI13_IRQn
#define MIC_SD_Pin GPIO_PIN_0
#define MIC_SD_GPIO_Port GPIOC
#define MIC_CK_Pin GPIO_PIN_1
#define MIC_CK_GPIO_Port GPIOC
#define SW_MIC_Pin GPIO_PIN_2
#define SW_MIC_GPIO_Port GPIOC
#define CS_LIS3DML_Pin GPIO_PIN_3
#define CS_LIS3DML_GPIO_Port GPIOC
#define CS_LSM6DSV_Pin GPIO_PIN_0
#define CS_LSM6DSV_GPIO_Port GPIOA
#define DOCK_DETECT_Pin GPIO_PIN_1
#define DOCK_DETECT_GPIO_Port GPIOA
#define DOCK_DETECT_EXTI_IRQn EXTI1_IRQn
#define DOCK_TXD_Pin GPIO_PIN_2
#define DOCK_TXD_GPIO_Port GPIOA
#define DOCK_RXD_Pin GPIO_PIN_3
#define DOCK_RXD_GPIO_Port GPIOA
#define GPIO_ADC_INT_EXP3_Pin GPIO_PIN_4
#define GPIO_ADC_INT_EXP3_GPIO_Port GPIOA
#define GPIO_ADC_INT_EXP2_Pin GPIO_PIN_5
#define GPIO_ADC_INT_EXP2_GPIO_Port GPIOA
#define GPIO_ADC_INT_EXP1_Pin GPIO_PIN_6
#define GPIO_ADC_INT_EXP1_GPIO_Port GPIOA
#define GPIO_ADC_INT_EXP0_Pin GPIO_PIN_7
#define GPIO_ADC_INT_EXP0_GPIO_Port GPIOA
#define GPIO_ADC_EXT_EXP2_Pin GPIO_PIN_0
#define GPIO_ADC_EXT_EXP2_GPIO_Port GPIOB
#define GPIO_ADC_EXT_EXP1_Pin GPIO_PIN_1
#define GPIO_ADC_EXT_EXP1_GPIO_Port GPIOB
#define GPIO_ADC_EXT_EXP0_Pin GPIO_PIN_2
#define GPIO_ADC_EXT_EXP0_GPIO_Port GPIOB
#define CHG_STAT1_Pin GPIO_PIN_7
#define CHG_STAT1_GPIO_Port GPIOE
#define CHG_STAT1_EXTI_IRQn EXTI7_IRQn
#define CHG_STAT2_Pin GPIO_PIN_8
#define CHG_STAT2_GPIO_Port GPIOE
#define CHG_STAT2_EXTI_IRQn EXTI8_IRQn
#define LSM303AH_INT1_XL_Pin GPIO_PIN_9
#define LSM303AH_INT1_XL_GPIO_Port GPIOE
#define LSM303AH_INT_MAG_DRDY_Pin GPIO_PIN_10
#define LSM303AH_INT_MAG_DRDY_GPIO_Port GPIOE
#define BMP390_INT_Pin GPIO_PIN_11
#define BMP390_INT_GPIO_Port GPIOE
#define SW_SPI1_Pin GPIO_PIN_12
#define SW_SPI1_GPIO_Port GPIOE
#define SW_SPI2_Pin GPIO_PIN_14
#define SW_SPI2_GPIO_Port GPIOB
#define BT_TXD_Pin GPIO_PIN_8
#define BT_TXD_GPIO_Port GPIOD
#define BT_RXD_Pin GPIO_PIN_9
#define BT_RXD_GPIO_Port GPIOD
#define BT_LP_MODE_Pin GPIO_PIN_10
#define BT_LP_MODE_GPIO_Port GPIOD
#define BT_CTS_Pin GPIO_PIN_11
#define BT_CTS_GPIO_Port GPIOD
#define BT_RTS_Pin GPIO_PIN_12
#define BT_RTS_GPIO_Port GPIOD
#define LED_UPR_RD_Pin GPIO_PIN_13
#define LED_UPR_RD_GPIO_Port GPIOD
#define BT_CONNECTION_Pin GPIO_PIN_14
#define BT_CONNECTION_GPIO_Port GPIOD
#define BT_CONNECTION_EXTI_IRQn EXTI14_IRQn
#define BT_HOST_WAKE_Pin GPIO_PIN_15
#define BT_HOST_WAKE_GPIO_Port GPIOD
#define BT_HOST_WAKE_EXTI_IRQn EXTI15_IRQn
#define BT_CP_ROLE_Pin GPIO_PIN_6
#define BT_CP_ROLE_GPIO_Port GPIOC
#define BT_DEVICE_WAKE_Pin GPIO_PIN_7
#define BT_DEVICE_WAKE_GPIO_Port GPIOC
#define SDMMC1_D0_Pin GPIO_PIN_8
#define SDMMC1_D0_GPIO_Port GPIOC
#define SDMMC1_D1_Pin GPIO_PIN_9
#define SDMMC1_D1_GPIO_Port GPIOC
#define SW_BT_Pin GPIO_PIN_8
#define SW_BT_GPIO_Port GPIOA
#define SDMMC1_D2_Pin GPIO_PIN_10
#define SDMMC1_D2_GPIO_Port GPIOC
#define SDMMC1_D3_Pin GPIO_PIN_11
#define SDMMC1_D3_GPIO_Port GPIOC
#define SD_CLK_Pin GPIO_PIN_12
#define SD_CLK_GPIO_Port GPIOC
#define SD_DETECT_N_Pin GPIO_PIN_0
#define SD_DETECT_N_GPIO_Port GPIOD
#define SD_DETECT_N_EXTI_IRQn EXTI0_IRQn
#define SW_FLASH_Pin GPIO_PIN_1
#define SW_FLASH_GPIO_Port GPIOD
#define SD_CMD_Pin GPIO_PIN_2
#define SD_CMD_GPIO_Port GPIOD
#define GPIO_INTERNAL0_Pin GPIO_PIN_3
#define GPIO_INTERNAL0_GPIO_Port GPIOD
#define GPIO_INTERNAL1_Pin GPIO_PIN_4
#define GPIO_INTERNAL1_GPIO_Port GPIOD
#define GPIO_INTERNAL2_Pin GPIO_PIN_5
#define GPIO_INTERNAL2_GPIO_Port GPIOD
#define CS_BMP390_Pin GPIO_PIN_6
#define CS_BMP390_GPIO_Port GPIOD
#define CS_LSM303AH_Pin GPIO_PIN_7
#define CS_LSM303AH_GPIO_Port GPIOD
#define CS_HIGH_G_Pin GPIO_PIN_5
#define CS_HIGH_G_GPIO_Port GPIOB
#define BSL_TXD_Pin GPIO_PIN_6
#define BSL_TXD_GPIO_Port GPIOB
#define BSL_RXD_Pin GPIO_PIN_7
#define BSL_RXD_GPIO_Port GPIOB
#define LED_UPR_GR_Pin GPIO_PIN_8
#define LED_UPR_GR_GPIO_Port GPIOB
#define LED_UPR_BLU_Pin GPIO_PIN_9
#define LED_UPR_BLU_GPIO_Port GPIOB
#define SW_I2C2_Pin GPIO_PIN_0
#define SW_I2C2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
