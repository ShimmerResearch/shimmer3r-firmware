/*
 * Copyright (c) 2016, Shimmer Research, Ltd.
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of Shimmer Research, Ltd. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *    * You may not use or distribute this Software or any derivative works
 *      in any form for commercial purposes with the exception of commercial
 *      purposes when used in conjunction with Shimmer products purchased
 *      from Shimmer or their designated agent or with permission from
 *      Shimmer.
 *      Examples of commercial purposes would be running business
 *      operations, licensing, leasing, or selling the Software, or
 *      distributing the Software for use with commercial products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author Weibo Pan
 * @date May, 2016
 */

#include "hal_Power.h"
#include "../S4_App/s4.h"
#include "stm32u5a5xx.h"
#include "stm32u5xx_hal.h"

extern void SystemClock_Config(void);
extern STATTypeDef * GetStatus(void);

extern void TIM_init(void);

void Power_Standby(void) {
   HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
   // need to clear flag here
   __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SBF | PWR_WAKEUP_ALL_FLAG);
   HAL_PWR_EnterSTANDBYMode();
}


uint8_t Power_StandbyReset(void) {
   /* Check Standby Flag */
   if (__HAL_PWR_GET_FLAG(PWR_FLAG_SBF) != RESET) {
      /* Clear Standby and wakeup flag */
      __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SBF | PWR_WAKEUP_ALL_FLAG);
      /* Reset was from wakeup from standy */
      return 1;
   }
   /* Reset was not from standby */
   return 0;
}
void Power_GpioAnalogConfig (void)
{
   GPIO_InitTypeDef GPIO_InitStruct;

   /* Configure all GPIO as analog to reduce current consumption on non used IOs */
   /* Enable GPIOs clock */

//   __GPIOA_CLK_ENABLE();

   __GPIOB_CLK_ENABLE();
   __GPIOC_CLK_ENABLE();
   __GPIOD_CLK_ENABLE();
   __GPIOE_CLK_ENABLE();
   __GPIOF_CLK_ENABLE();
   __GPIOG_CLK_ENABLE();
   __GPIOH_CLK_ENABLE();
   __GPIOI_CLK_ENABLE();
   __GPIOJ_CLK_ENABLE();

#if defined (SHIMMER4_SDK)
   __GPIOK_CLK_ENABLE();
#endif  /* SHIMMER4_SDK) */
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Pin = GPIO_PIN_ALL;

//   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
   HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
   HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
   HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
   HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
   HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
   HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);
//   HAL_GPIO_Init(GPIOK, &GPIO_InitStruct);

   /* Disable GPIOs clock */


//   __GPIOA_CLK_DISABLE();

   __GPIOB_CLK_DISABLE();
   __GPIOC_CLK_DISABLE();
   __GPIOD_CLK_DISABLE();
   __GPIOE_CLK_DISABLE();
   __GPIOF_CLK_DISABLE();
   __GPIOG_CLK_DISABLE();
   __GPIOH_CLK_DISABLE();
   __GPIOI_CLK_DISABLE();
   __GPIOJ_CLK_DISABLE();

#if defined (SHIMMER4_SDK)
   __GPIOK_CLK_DISABLE();
#endif /* SHIMMER4_SDK) */



#ifndef DEBUG // Something to do with debugging...
   __GPIOA_CLK_ENABLE();
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
   __GPIOA_CLK_DISABLE();
#endif

}
void Power_SleepUntilInterrupt(void) {
  //Original
//   SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
//   __WFI();
//   SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

   //Suggestion 1
   HAL_SuspendTick();
   HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
   HAL_ResumeTick();
}

void Power_StopUntilInterrupt(void) {
   
   HAL_EnableDBGStopMode();
   
//   HAL_PWREx_EnterUnderDriveSTOPMode(PWR_LOWPOWERREGULATOR_UNDERDRIVE_ON, PWR_STOPENTRY_WFI);
   //Power_GpioAnalogConfig(); 
   //HAL_PWREx_EnableFlashPowerDown();
   //HAL_PWREx_EnterUnderDriveSTOPMode(PWR_LOWPOWERREGULATOR_UNDERDRIVE_ON, PWR_STOPENTRY_WFI);
   HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
   
   /* After wakeup, call system init to enable PLL as clock source */
//	SystemInit();
   
//   if(GetStatus()->isBtConnected){
//      SystemClock_Config();
//   }
   
   //SystemClock_Config();
}
