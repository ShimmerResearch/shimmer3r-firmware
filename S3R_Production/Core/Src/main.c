/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "adc.h"
#include "crc.h"
#include "gpdma.h"
#include "i2c.h"
#include "icache.h"
#include "mdf.h"
#include "memorymap.h"
#include "rng.h"
#include "rtc.h"
#include "sdmmc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "s4.h"
#include "s4__cfg.h"

#define TIM_MEASURE_START time_start = SysTick->VAL
#define TIM_MEASURE_END time_end = SysTick->VAL;            \
  time_diff = time_start - time_end

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t dcID[16];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
/* USER CODE BEGIN PFP */

void Init(void);
uint32_t FullTest(void);

//TODO move out of here
#if IS_SHIMMER3R
void btInitialise(void);
void btFactoryResetViaFw(void);
void btCommWithDiffBaudRates(bool isInit, uint8_t reset_cnt);
void setBtConnectionState(bool state);
bool isBtConnected(void);
//TODO move to "s4_adc.c"
void rgb_led_lwr_color(uint8_t red, uint8_t green, uint8_t blue);
void rgb_led_upr_color(uint8_t red, uint8_t green, uint8_t blue);
#endif

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

STATTypeDef stat;
//SENSINGTypeDef *pSensing;

volatile uint32_t time_start, time_end, time_diff;


extern UART_HandleTypeDef *huartBt;

void Init() {
   Board_ledOn(LED_ALL);
   memset((uint8_t*)&stat, 0, sizeof(STATTypeDef));
   stat.battStatLed = LED_YELLOW;
   stat.isConfiguring = 1;

   // ==== 0.86ma ====
#if !IS_SHIMMER3R
   TIM_init();
#endif
   InfoMem_init();
   S4_Task_init();
   S4Sens_init();
   // ==== 0.86ma ====
   //SD_init();
   //GPIO_init();
   I2C_init();
   S4_ADC_init();
   SPI_init();
   // ==== 0.86ma ====
   DockUart_init();
   DockUart_disable();
   // ==== 0.89ma ====
   S4_RTC_Init();
   // ==== 24.60ma ====
   Board_delayMicrosInit();
   DockUart_interruptCheck();
   SD_insertedCheck();
   //GPIO_userButtonCheck();
#if IS_SHIMMER3R
   setCrcHandleToUse(getCrcHandle());
   btCommsProtocolInit(setTaskNewBtCmdToProcess);
 //  btFactoryResetViaFw();
   btInitialise();
#else
   BtUart_init();
#endif
   S4Ram_init();
   ShimmerCalib_init();
   // ==== 13.8ma ====
#if FULL_TEST_MODE
   FullTest();
#endif
   //BT_disable(huartBt);
   DockUart_enable();
   stat.isConfiguring = 0;
   S4Sens_stopPeripherals();
   S4_RTC_WakeUpSetSlow();
   Board_ledOff(LED_ALL);
//   while(1){
//      //__NOP();
//      Power_StopUntilInterrupt();
//      //Power_SleepUntilInterrupt();
//   }
   //Power_StopUntilInterrupt();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  uint32_t i = 0;
  while(i++ < 1000000);

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the System Power */
  SystemPower_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_I2C2_Init();
  MX_RNG_Init();
  MX_RTC_Init();
  MX_SDMMC1_SD_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC2_Init();
  MX_USB_OTG_HS_PCD_Init();
  MX_USART2_UART_Init();
  MX_ICACHE_Init();
  MX_CRC_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_MDF1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
#if USE_FATFS
  MX_FATFS_Init();
#endif
#if !USE_USBX
  MX_USB_DEVICE_Init();
#endif

  Init();
  //Task_set(TASK_STARTSENSING);

//  SD_test();
//  SD_test_alternative();

  //TODO move to "s4_adc.c"
  //https://www.youtube.com/watch?v=GBr6bQ-PzV8
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  i = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    S4_Task_manage();

//    HAL_UART_Transmit(&huart2, "E", 1, 0xFFFF);
    rgb_led_lwr_color(i, i, i);
    i+=5;
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
   */
  HAL_PWREx_DisableUCPDDeadBattery();

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/* USER CODE BEGIN 4 */

STATTypeDef * GetStatus(){
   return &stat;
}

uint32_t FullTest(void) {
   //uint32_t test_result = 0;

   stat.testResult += I2C_test();

   stat.testResult += SD_test()<<6;

   stat.testResult += (!stat.isBtPoweredOn)<<7;

   stat.testResult += InfoMem_test()<<8;

   stat.testResult += SPI_test()<<16;

   return stat.testResult ;
}

//TODO move out of here
#if IS_SHIMMER3R
void btInitialise(void)
{
  printf("\r\nBT init start\r\n");

  // 20 * 100ms = 2s per baud rate attempt
  btCommWithDiffBaudRates(true, 20U);

  printf("BT init end\r\n");
}

void btFactoryResetViaFw(void)
{
  printf("\r\nBT factory reset start\r\n");

  // 50 * 100ms = 5s per baud rate attempt
  btCommWithDiffBaudRates(false, 50U);

  // Abort transfer operations to release UART for subsequent requests.
  HAL_StatusTypeDef status = HAL_UART_Abort(&huart3);

  printf("BT factory reset end\r\n");
}

void btCommWithDiffBaudRates(bool isInit, uint8_t reset_cnt)
{
  uint8_t failCount = 0U;
  uint32_t baudToTry = BAUD_TO_USE;

  setBtLpMode(false);

  printf("Attempting %lu Baud\r\n", baudToTry);
  usartBtUpdate(baudToTry, baudToTry==115200? 0:FLOW_CONTROL);

  if (isInit)
  {
    btInit();
  }
  else
  {
    btFactoryResetInit();
  }

  while ((isBtInitCmdsRunning() && !isBtIsInitialised())
      || (isBtFactoryResetCmdsRunning() && !isBtIsFactoryResetted()))
  {
//    HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    /* Insert delay 100 ms */
    HAL_Delay(100);

    if (isEzsBaudRateDelayPending())
    {
      /* Delay or arbitrary value. As a guide, the EZ-Serial user guide states that it takes ~150 ms for a "chipset reset and boot process". */
      HAL_Delay(500);
      incrementBtInitCmdsStep();
      btInitCommands();
    }
    else if (isEzsFactoryRebootDelayPending())
    {
      /* Experimentally found to be ~ 2.75s. */
      HAL_Delay(3000);
      incrementBtFactoryResetCmdsStep();
      btFactoryResetCommands();
    }

    if (!(reset_cnt--))
    {
      failCount++;

      if (failCount <= 4)
      {
        if (failCount == 1)
        {
          baudToTry = 115200;
        }
        else if (failCount == 2)
        {
          baudToTry = 460800;
        }
        else if (failCount == 3)
        {
          baudToTry = 2000000;
        }
        else if (failCount == 4)
        {
          baudToTry = 500000;
        }

        printf("Attempting %lu Baud\r\n", baudToTry);
        usartBtUpdate(baudToTry, baudToTry==115200? 0:FLOW_CONTROL);
      }
      else
      {
        printf("Operation failed, performing system reset\r\n");
        // software POR reset
        NVIC_SystemReset();
      }

      if (isInit)
      {
        btInit();
      }
      else
      {
        btFactoryResetInit();
      }

      reset_cnt = 50U;
    }
  }
  setBtLpMode(true);
}

void setBtConnectionState(bool state)
{
  stat.isBtConnected = state;
//  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, state? GPIO_PIN_SET:GPIO_PIN_RESET);
}

bool isBtConnected(void)
{
  return stat.isBtConnected;
}

//TODO move to "s4_adc.c"
void rgb_led_lwr_color(uint8_t red, uint8_t green, uint8_t blue)
{
  htim3.Instance->CCR1 = red;
  htim3.Instance->CCR2 = green;
  htim3.Instance->CCR3 = blue;
}

//TODO move to "s4_adc.c"
void rgb_led_upr_color(uint8_t red, uint8_t green, uint8_t blue)
{
  htim3.Instance->CCR2 = red;
  htim3.Instance->CCR3 = green;
  htim3.Instance->CCR4 = blue;
}

#endif

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
