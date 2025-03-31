/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32u5xx_it.c
 * @brief   Interrupt Service Routines.
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
#include "stm32u5xx_it.h"
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "shimmer_definitions.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern MDF_HandleTypeDef AdfHandle0;
extern DMA_HandleTypeDef handle_GPDMA1_Channel11;
extern DMA_HandleTypeDef handle_GPDMA1_Channel2;
extern DMA_HandleTypeDef handle_GPDMA1_Channel10;
extern I2C_HandleTypeDef hi2c1;
extern RTC_HandleTypeDef hrtc;
extern SD_HandleTypeDef hsd1;
extern DMA_HandleTypeDef handle_GPDMA1_Channel5;
extern DMA_HandleTypeDef handle_GPDMA1_Channel4;
extern DMA_HandleTypeDef handle_GPDMA1_Channel7;
extern DMA_HandleTypeDef handle_GPDMA1_Channel6;
extern DMA_HandleTypeDef handle_GPDMA1_Channel9;
extern DMA_HandleTypeDef handle_GPDMA1_Channel8;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;
extern TIM_HandleTypeDef htim6;
extern DMA_HandleTypeDef handle_GPDMA1_Channel1;
extern DMA_HandleTypeDef handle_GPDMA1_Channel0;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern PCD_HandleTypeDef hpcd_USB_OTG_HS;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
 * @brief This function handles Prefetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32U5xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32u5xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles RTC non-secure interrupt.
 */
void RTC_IRQHandler(void)
{
  /* USER CODE BEGIN RTC_IRQn 0 */

  /* USER CODE END RTC_IRQn 0 */
  HAL_RTC_AlarmIRQHandler(&hrtc);
  HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
  /* USER CODE BEGIN RTC_IRQn 1 */

  /* USER CODE END RTC_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line1 interrupt.
 */
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */

  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

/**
 * @brief This function handles EXTI Line6 interrupt.
 */
void EXTI6_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI6_IRQn 0 */

  /* USER CODE END EXTI6_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(USER_BTN_Pin);
  /* USER CODE BEGIN EXTI6_IRQn 1 */

  /* USER CODE END EXTI6_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 0 global interrupt.
 */
void GPDMA1_Channel0_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel0_IRQn 0 */

  /* USER CODE END GPDMA1_Channel0_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel0);
  /* USER CODE BEGIN GPDMA1_Channel0_IRQn 1 */

  /* USER CODE END GPDMA1_Channel0_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 1 global interrupt.
 */
void GPDMA1_Channel1_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel1_IRQn 0 */

  /* USER CODE END GPDMA1_Channel1_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel1);
  /* USER CODE BEGIN GPDMA1_Channel1_IRQn 1 */

  /* USER CODE END GPDMA1_Channel1_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 2 global interrupt.
 */
void GPDMA1_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel2_IRQn 0 */

  /* USER CODE END GPDMA1_Channel2_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel2);
  /* USER CODE BEGIN GPDMA1_Channel2_IRQn 1 */

  /* USER CODE END GPDMA1_Channel2_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 4 global interrupt.
 */
void GPDMA1_Channel4_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel4_IRQn 0 */

  /* USER CODE END GPDMA1_Channel4_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel4);
  /* USER CODE BEGIN GPDMA1_Channel4_IRQn 1 */

  /* USER CODE END GPDMA1_Channel4_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 5 global interrupt.
 */
void GPDMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel5_IRQn 0 */

  /* USER CODE END GPDMA1_Channel5_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel5);
  /* USER CODE BEGIN GPDMA1_Channel5_IRQn 1 */

  /* USER CODE END GPDMA1_Channel5_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 6 global interrupt.
 */
void GPDMA1_Channel6_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel6_IRQn 0 */

  /* USER CODE END GPDMA1_Channel6_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel6);
  /* USER CODE BEGIN GPDMA1_Channel6_IRQn 1 */

  /* USER CODE END GPDMA1_Channel6_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 7 global interrupt.
 */
void GPDMA1_Channel7_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel7_IRQn 0 */

  /* USER CODE END GPDMA1_Channel7_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel7);
  /* USER CODE BEGIN GPDMA1_Channel7_IRQn 1 */

  /* USER CODE END GPDMA1_Channel7_IRQn 1 */
}

/**
 * @brief This function handles ADC1_2 global interrupt.
 */
void ADC1_2_IRQHandler(void)
{
  /* USER CODE BEGIN ADC1_2_IRQn 0 */
  if (hadc1.Instance != NULL && hadc2.Instance != NULL)
  {
    /* USER CODE END ADC1_2_IRQn 0 */
    HAL_ADC_IRQHandler(&hadc1);
    HAL_ADC_IRQHandler(&hadc2);
    /* USER CODE BEGIN ADC1_2_IRQn 1 */
  }
  else if (hadc1.Instance != NULL)
  {
    HAL_ADC_IRQHandler(&hadc1);
  }
  else if (hadc2.Instance != NULL)
  {
    HAL_ADC_IRQHandler(&hadc2);
  }
  /* USER CODE END ADC1_2_IRQn 1 */
}

/**
 * @brief This function handles TIM6 global interrupt.
 */
void TIM6_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_IRQn 0 */

  /* USER CODE END TIM6_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_IRQn 1 */

  /* USER CODE END TIM6_IRQn 1 */
}

/**
 * @brief This function handles I2C1 Event interrupt.
 */
void I2C1_EV_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_EV_IRQn 0 */

  /* USER CODE END I2C1_EV_IRQn 0 */
  HAL_I2C_EV_IRQHandler(&hi2c1);
  /* USER CODE BEGIN I2C1_EV_IRQn 1 */

  /* USER CODE END I2C1_EV_IRQn 1 */
}

/**
 * @brief This function handles I2C1 Error interrupt.
 */
void I2C1_ER_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_ER_IRQn 0 */

  /* USER CODE END I2C1_ER_IRQn 0 */
  HAL_I2C_ER_IRQHandler(&hi2c1);
  /* USER CODE BEGIN I2C1_ER_IRQn 1 */

  /* USER CODE END I2C1_ER_IRQn 1 */
}

/**
 * @brief This function handles SPI1 global interrupt.
 */
void SPI1_IRQHandler(void)
{
  /* USER CODE BEGIN SPI1_IRQn 0 */

  /* USER CODE END SPI1_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi1);
  /* USER CODE BEGIN SPI1_IRQn 1 */

  /* USER CODE END SPI1_IRQn 1 */
}

/**
 * @brief This function handles SPI2 global interrupt.
 */
void SPI2_IRQHandler(void)
{
  /* USER CODE BEGIN SPI2_IRQn 0 */

  /* USER CODE END SPI2_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi2);
  /* USER CODE BEGIN SPI2_IRQn 1 */

  /* USER CODE END SPI2_IRQn 1 */
}

/**
 * @brief This function handles USART1 global interrupt.
 */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
 * @brief This function handles USART3 global interrupt.
 */
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */

  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */

  /* USER CODE END USART3_IRQn 1 */
}

/**
 * @brief This function handles USB OTG HS global interrupt.
 */
void OTG_HS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_HS_IRQn 0 */

  /* USER CODE END OTG_HS_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_HS);
  /* USER CODE BEGIN OTG_HS_IRQn 1 */

  /* USER CODE END OTG_HS_IRQn 1 */
}

/**
 * @brief This function handles SDMMC1 global interrupt.
 */
void SDMMC1_IRQHandler(void)
{
  /* USER CODE BEGIN SDMMC1_IRQn 0 */

  /* USER CODE END SDMMC1_IRQn 0 */
  HAL_SD_IRQHandler(&hsd1);
  /* USER CODE BEGIN SDMMC1_IRQn 1 */

  /* USER CODE END SDMMC1_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 8 global interrupt.
 */
void GPDMA1_Channel8_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel8_IRQn 0 */

  /* USER CODE END GPDMA1_Channel8_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel8);
  /* USER CODE BEGIN GPDMA1_Channel8_IRQn 1 */

  /* USER CODE END GPDMA1_Channel8_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 9 global interrupt.
 */
void GPDMA1_Channel9_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel9_IRQn 0 */

  /* USER CODE END GPDMA1_Channel9_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel9);
  /* USER CODE BEGIN GPDMA1_Channel9_IRQn 1 */

  /* USER CODE END GPDMA1_Channel9_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 10 global interrupt.
 */
void GPDMA1_Channel10_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel10_IRQn 0 */

  /* USER CODE END GPDMA1_Channel10_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel10);
  /* USER CODE BEGIN GPDMA1_Channel10_IRQn 1 */

  /* USER CODE END GPDMA1_Channel10_IRQn 1 */
}

/**
 * @brief This function handles GPDMA1 Channel 11 global interrupt.
 */
void GPDMA1_Channel11_IRQHandler(void)
{
  /* USER CODE BEGIN GPDMA1_Channel11_IRQn 0 */

  /* USER CODE END GPDMA1_Channel11_IRQn 0 */
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel11);
  /* USER CODE BEGIN GPDMA1_Channel11_IRQn 1 */

  /* USER CODE END GPDMA1_Channel11_IRQn 1 */
}

/**
 * @brief This function handles SPI3 global interrupt.
 */
void SPI3_IRQHandler(void)
{
  /* USER CODE BEGIN SPI3_IRQn 0 */

  /* USER CODE END SPI3_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi3);
  /* USER CODE BEGIN SPI3_IRQn 1 */

  /* USER CODE END SPI3_IRQn 1 */
}

/**
 * @brief This function handles ADF interrupt.
 */
void ADF1_IRQHandler(void)
{
  /* USER CODE BEGIN ADF1_IRQn 0 */

  /* USER CODE END ADF1_IRQn 0 */
  HAL_MDF_IRQHandler(&AdfHandle0);
  /* USER CODE BEGIN ADF1_IRQn 1 */

  /* USER CODE END ADF1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* Note the interrupts listed below are not enabled by default in CubeMX and
 * instead are turned on/off when needed by firmware to save power. */

/**
 * @brief This function handles EXTI Line0 interrupt.
 */
void EXTI0_IRQHandler(void)
{
#if SUPPORT_SR48_6_0
  if(ShimBrd_isBoardSr48_6_0())
  {
    HAL_GPIO_EXTI_IRQHandler(SR48_6_0_GPIO_ADC_INT_EXP1_Pin);
  }
#endif
  //HAL_GPIO_EXTI_IRQHandler(SD_DETECT_N_Pin);
}

/**
 * @brief This function handles EXTI Line2 interrupt.
 */
void EXTI2_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(LIS3MDL_DRDY_Pin);
}

/**
 * @brief This function handles EXTI Line3 interrupt.
 */
void EXTI3_IRQHandler(void)
{
#if SUPPORT_SR48_6_0
  if(ShimBrd_isBoardSr48_6_0())
  {
    HAL_GPIO_EXTI_IRQHandler(SR48_6_0_BOOT0_USER_BTN_Pin);
  }
#endif
}

/**
 * @brief This function handles EXTI Line4 interrupt.
 */
void EXTI4_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_INTERNAL1_Pin);
}

/**
 * @brief This function handles EXTI Line5 interrupt.
 */
void EXTI5_IRQHandler(void)
{
#if SUPPORT_SR48_6_0
  if(ShimBrd_isBoardSr48_6_0())
  {
    HAL_GPIO_EXTI_IRQHandler(SR48_6_0_GPIO_ADC_INT_EXP0_Pin);
  }
#endif
  //No plans to use GPIO_INTERNAL2_Pin as interrupt
  //HAL_GPIO_EXTI_IRQHandler(GPIO_INTERNAL2_Pin);
}

/**
 * @brief This function handles EXTI Line7 interrupt.
 */
void EXTI7_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(LSM6DSV_INT1_Pin);
}

/**
 * @brief This function handles EXTI Line8 interrupt.
 */
void EXTI8_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BT_HOST_WAKE_Pin);
}

/**
 * @brief This function handles EXTI Line9 interrupt.
 */
void EXTI9_IRQHandler(void)
{
  //HAL_GPIO_EXTI_IRQHandler(GPIO_EXTERNAL_Pin);
  HAL_GPIO_EXTI_IRQHandler(USB_VBUS_Pin);
}

/**
 * @brief This function handles EXTI Line10 interrupt.
 */
void EXTI10_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(LIS2MDL_DRDY_Pin);
}

/**
 * @brief This function handles EXTI Line11 interrupt.
 */
void EXTI11_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BMP390_INT_Pin);
}

/**
 * @brief This function handles EXTI Line12 interrupt.
 */
void EXTI12_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(LIS2DW12_INT1_Pin);
}

/**
 * @brief This function handles EXTI Line13 interrupt.
 */
void EXTI13_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_INTERNAL0_Pin);
}

/**
 * @brief This function handles EXTI Line14 interrupt.
 */
void EXTI14_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BT_CONNECTION_Pin);
}

/**
 * @brief This function handles EXTI Line15 interrupt.
 */
void EXTI15_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(BT_CYSPP_Pin);
}

/* USER CODE END 1 */
