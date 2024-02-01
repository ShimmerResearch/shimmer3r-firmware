/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "tim.h"

/* USER CODE BEGIN 0 */
//#include "s4_led.h"

TIM_HandleTypeDef *htimSensing;
TIM_HandleTypeDef *htimStarter;
TIM_HandleTypeDef *htimBlink;


/* USER CODE END 0 */

/*TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;*/

/* TIM1 init function */
void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}
/* TIM2 init function */
void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 10799;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 1000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.Pulse = 2000;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}
/* TIM3 init function */
void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 26366;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 8191;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}
/* TIM4 init function */
void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 10799;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 999;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/*void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle) // commented for porting
{

  if(tim_baseHandle->Instance==TIM1)
  {
   USER CODE BEGIN TIM1_MspInit 0

   USER CODE END TIM1_MspInit 0
     TIM1 clock enable
    __HAL_RCC_TIM1_CLK_ENABLE();
   USER CODE BEGIN TIM1_MspInit 1

   USER CODE END TIM1_MspInit 1
  }
  else if(tim_baseHandle->Instance==TIM2)
  {
   USER CODE BEGIN TIM2_MspInit 0

   USER CODE END TIM2_MspInit 0
     TIM2 clock enable
    __HAL_RCC_TIM2_CLK_ENABLE();

     TIM2 interrupt Init
    HAL_NVIC_SetPriority(TIM2_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
   USER CODE BEGIN TIM2_MspInit 1

   USER CODE END TIM2_MspInit 1
  }
  else if(tim_baseHandle->Instance==TIM3)
  {
   USER CODE BEGIN TIM3_MspInit 0

   USER CODE END TIM3_MspInit 0
     TIM3 clock enable
    __HAL_RCC_TIM3_CLK_ENABLE();

     TIM3 interrupt Init
    HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
   USER CODE BEGIN TIM3_MspInit 1

   USER CODE END TIM3_MspInit 1
  }
  else if(tim_baseHandle->Instance==TIM4)
  {
   USER CODE BEGIN TIM4_MspInit 0

   USER CODE END TIM4_MspInit 0
     TIM4 clock enable
    __HAL_RCC_TIM4_CLK_ENABLE();

     TIM4 interrupt Init
    HAL_NVIC_SetPriority(TIM4_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
   USER CODE BEGIN TIM4_MspInit 1

   USER CODE END TIM4_MspInit 1
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM1)
  {
   USER CODE BEGIN TIM1_MspDeInit 0

   USER CODE END TIM1_MspDeInit 0
     Peripheral clock disable
    __HAL_RCC_TIM1_CLK_DISABLE();
   USER CODE BEGIN TIM1_MspDeInit 1

   USER CODE END TIM1_MspDeInit 1
  }
  else if(tim_baseHandle->Instance==TIM2)
  {
   USER CODE BEGIN TIM2_MspDeInit 0

   USER CODE END TIM2_MspDeInit 0
     Peripheral clock disable
    __HAL_RCC_TIM2_CLK_DISABLE();

     TIM2 interrupt Deinit
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
   USER CODE BEGIN TIM2_MspDeInit 1

   USER CODE END TIM2_MspDeInit 1
  }
  else if(tim_baseHandle->Instance==TIM3)
  {
   USER CODE BEGIN TIM3_MspDeInit 0

   USER CODE END TIM3_MspDeInit 0
     Peripheral clock disable
    __HAL_RCC_TIM3_CLK_DISABLE();

     TIM3 interrupt Deinit
    HAL_NVIC_DisableIRQ(TIM3_IRQn);
   USER CODE BEGIN TIM3_MspDeInit 1

   USER CODE END TIM3_MspDeInit 1
  }
  else if(tim_baseHandle->Instance==TIM4)
  {
   USER CODE BEGIN TIM4_MspDeInit 0

   USER CODE END TIM4_MspDeInit 0
     Peripheral clock disable
    __HAL_RCC_TIM4_CLK_DISABLE();

     TIM4 interrupt Deinit
    HAL_NVIC_DisableIRQ(TIM4_IRQn);
   USER CODE BEGIN TIM4_MspDeInit 1

   USER CODE END TIM4_MspDeInit 1
  }
}*/

/* USER CODE BEGIN 1 */

void TIM_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

}
/* TIM2 init function */
void TIM_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = HAL_RCC_GetHCLKFreq()/20000-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 1000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.Pulse = 2000;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

}
/* TIM3 init function */
void TIM_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = HAL_RCC_GetHCLKFreq()/8192-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 8191;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

}
/* TIM4 init function */
void TIM_TIM4_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = HAL_RCC_GetHCLKFreq()/20000-1;//  = HAL_RCC_GetHCLKFreq()/1000-1
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 999;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

void TIM_init(void){   
   // init handlers
   
   // Weibo note: timer1 interrupt is different than others, more to investigate
   TIM_TIM2_Init();
   TIM_TIM3_Init();
   TIM_TIM4_Init();
   
   htimSensing = &htim3;
   htimStarter = &htim2;
   htimBlink = &htim4;
   
   //TIM_initBlink();
}


   
void TIM_initBlink(void){
   HAL_TIM_OC_Start_IT(htimStarter, TIM_CHANNEL_1);
   HAL_TIM_OC_Start_IT(htimStarter, TIM_CHANNEL_2);   
   
   HAL_TIM_Base_Stop_IT(htimBlink);   
   htimBlink->Init.Period = 999;
   HAL_TIM_Base_Init(htimBlink);
   HAL_TIM_Base_Start_IT(htimBlink);
}



void TIM_startSensing(void){   
  // htimSensing->Init.Period = sensing.clkInterval4096-1;
   HAL_TIM_Base_Init(htimSensing);
   HAL_TIM_Base_Start_IT(htimSensing);

   htimStarter->Instance->CCR2 = 10000;
}


void TIM_stopSensing(void){   
   htimStarter->Instance->CCR2 = 2000;
   HAL_TIM_Base_Stop(htimSensing);
   HAL_TIM_Base_Stop_IT(htimSensing);
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
   if (htim->Instance == htimSensing->Instance) {
#if SENS_CLK_RTC0TIM1
      S4Sens_gatherData();
#endif
   } else if (htim->Instance == htimStarter->Instance) {

   } else if (htim->Instance == htimBlink->Instance) {
     // S4Led_Blink();
   }
}
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
   if (htim->Instance == htimStarter->Instance) {
      if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
//            uint16_t current_ccr = htimStarter->Instance->CCR1;
//            current_ccr += 1000;
//            current_ccr %= 10000;
//            htimStarter->Instance->CCR1 = current_ccr;
         
         
//         Board_ledOn(LED_GREEN0);
//         if(stat.badFile){
//            Board_ledOn(LED_YELLOW);
//         }
//         Task_set(TASK_BATTREAD);

      }
      if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
//         if ((!stat.isDocked) || sensing.en) {
//            Board_ledOff(LED_GREEN0);
//            Board_ledOff(LED_YELLOW);
//         }
      }
      if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
      }
   }


}

/* USER CODE END 1 */
