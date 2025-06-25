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

#include "hal_Board.h"

#include "stm32u5xx_hal.h"

#if defined(SHIMMER3R)

static uint8_t ledStateLwrRed = 0, ledStateLwrGreen = 0, ledStateLwrBlue = 0;
static uint8_t ledStateUprRed = 0, ledStateUprGreen = 0, ledStateUprBlue = 0;

TIM_HandleTypeDef *htimLwrLedsPtr;
TIM_HandleTypeDef *htimUprLedsPtr;
TIM_HandleTypeDef *htimLedBlinkPtr;

volatile uint8_t sensePwrFlags = 0;

static void updateLedState(led_mode updateMode, uint8_t ledMask);

//https://www.youtube.com/watch?v=GBr6bQ-PzV8
void Board_ledTimersStart(TIM_HandleTypeDef *htimLwrLeds,
    TIM_HandleTypeDef *htimUprLeds,
    TIM_HandleTypeDef *htimLedBlink)
{
  htimLwrLedsPtr = htimLwrLeds;
  htimUprLedsPtr = htimUprLeds;
  htimLedBlinkPtr = htimLedBlink;

  HAL_TIM_PWM_Start(htimLwrLedsPtr, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(htimLwrLedsPtr, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(htimLwrLedsPtr, TIM_CHANNEL_3);

  HAL_TIM_PWM_Start(htimUprLedsPtr, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(htimUprLedsPtr, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(htimUprLedsPtr, TIM_CHANNEL_4);

  startLedBlinkTimer();
}

void startLedBlinkTimer(void)
{
  ShimLeds_resetCounters();

  // Align the blink timer to the next odd second to synchronize across devices.
  S4_RTC_t rtc;
  uint8_t target_sec;

  // Get current RTC time
  S4_RTC_GetDateTime(&rtc);

//  // Target second is the next even second
//  target_sec = (rtc.seconds % 2 == 0) ? rtc.seconds + 2 : rtc.seconds + 1;
  // Target second is the next odd second
  target_sec = (rtc.seconds % 2 == 1) ? rtc.seconds + 2 : rtc.seconds + 1;

  target_sec %= 60;

  // Wait until RTC seconds match target and subseconds are near the top (e.g., > 32760)
  // Subsecond buffer required here to account for code execution and readout delays.
  do {
      S4_RTC_GetDateTime(&rtc);
  } while (rtc.seconds != target_sec || rtc.subseconds < 32760);

  // Now start the blink timer
  HAL_TIM_Base_Start_IT(htimLedBlinkPtr);
  shimmerStatus.timerBlinkEnabled = 1;
}

void stopLedBlinkTimer(void)
{
  HAL_TIM_Base_Stop(htimLedBlinkPtr);
  shimmerStatus.timerBlinkEnabled = 0;

  //TODO turn off all LEDs to reset their state?
}

void rgb_led_lwr_color(uint8_t red, uint8_t green, uint8_t blue)
{
  if (htimLwrLedsPtr)
  {
    htimLwrLedsPtr->Instance->CCR1 = red;
    htimLwrLedsPtr->Instance->CCR2 = green;
    htimLwrLedsPtr->Instance->CCR3 = blue;
  }
}

void rgb_led_upr_color(uint8_t red, uint8_t green, uint8_t blue)
{
  if (htimUprLedsPtr)
  {
    htimUprLedsPtr->Instance->CCR1 = red;
    htimUprLedsPtr->Instance->CCR3 = green;
    htimUprLedsPtr->Instance->CCR4 = blue;
  }
}

void Board_ledLwrSetColour(uint32_t ledMask)
{
  ledStateLwrRed = ledMask >> 16;
  ledStateLwrGreen = ledMask >> 8;
  ledStateLwrBlue = ledMask;
  rgb_led_lwr_color(ledStateLwrRed, ledStateLwrGreen, ledStateLwrBlue);
}

void Board_ledLwrSetColourRgb(int16_t red, int16_t green, int16_t blue)
{
  if (red != -1)
  {
    ledStateLwrRed = red;
  }
  if (green != -1)
  {
    ledStateLwrGreen = green;
  }
  if (blue != -1)
  {
    ledStateLwrBlue = blue;
  }
  rgb_led_lwr_color(ledStateLwrRed, ledStateLwrGreen, ledStateLwrBlue);
}

void Board_ledLwrToggleColourRgb(uint32_t ledMask)
{
  if (ledMask & LED_RGB_RED)
  {
    ledStateLwrRed = ledStateLwrRed ? LED_PWM_OFF : LED_PWM_ON;
  }
  if (ledMask & LED_RGB_GREEN)
  {
    ledStateLwrGreen = ledStateLwrGreen ? LED_PWM_OFF : LED_PWM_ON;
  }
  if (ledMask & LED_RGB_BLUE)
  {
    ledStateLwrBlue = ledStateLwrBlue ? LED_PWM_OFF : LED_PWM_ON;
  }
  rgb_led_lwr_color(ledStateLwrRed, ledStateLwrGreen, ledStateLwrBlue);
}

void Board_ledUprSetColour(uint32_t ledMask)
{
  ledStateUprRed = ledMask >> 16;
  ledStateUprGreen = ledMask >> 8;
  ledStateUprBlue = ledMask;
  rgb_led_upr_color(ledStateUprRed, ledStateUprGreen, ledStateUprBlue);
}

void Board_ledUprSetColourRgb(int16_t red, int16_t green, int16_t blue)
{
  if (red != -1)
  {
    ledStateUprRed = red;
  }
  if (green != -1)
  {
    ledStateUprGreen = green;
  }
  if (blue != -1)
  {
    ledStateUprBlue = blue;
  }
  rgb_led_upr_color(ledStateUprRed, ledStateUprGreen, ledStateUprBlue);
}

void Board_ledUprToggleColourRgb(uint32_t ledMask)
{
  if (ledMask & LED_RGB_RED)
  {
    ledStateUprRed = ledStateUprRed ? LED_PWM_OFF : LED_PWM_ON;
  }
  if (ledMask & LED_RGB_GREEN)
  {
    ledStateUprGreen = ledStateUprGreen ? LED_PWM_OFF : LED_PWM_ON;
  }
  if (ledMask & LED_RGB_BLUE)
  {
    ledStateUprBlue = ledStateUprBlue ? LED_PWM_OFF : LED_PWM_ON;
  }
  rgb_led_upr_color(ledStateUprRed, ledStateUprGreen, ledStateUprBlue);
}

static void updateLedState(led_mode updateMode, uint8_t ledMask)
{
  if (updateMode == LED_MODE_OFF || updateMode == LED_MODE_ON)
  {
    uint8_t valueToSet = updateMode == LED_MODE_ON ? LED_PWM_ON : LED_PWM_OFF;

    if (ledMask == LED_ALL)
    {
      ledStateLwrRed = valueToSet;
      ledStateLwrGreen = valueToSet;
      ledStateLwrBlue = valueToSet;
      ledStateUprRed = valueToSet;
      ledStateUprGreen = valueToSet;
      ledStateUprBlue = valueToSet;
    }
    else
    {
      if (ledMask & LED_LWR_RED)
      {
        ledStateLwrRed = valueToSet;
      }
      if (ledMask & LED_LWR_GREEN)
      {
        ledStateLwrGreen = valueToSet;
      }
      if (ledMask & LED_LWR_YELLOW)
      {
        ledStateLwrRed = valueToSet;
        ledStateLwrGreen = valueToSet;
      }
      if (ledMask & LED_UPR_GREEN)
      {
        ledStateUprGreen = valueToSet;
      }
      if (ledMask & LED_UPR_BLUE)
      {
        ledStateUprBlue = valueToSet;
      }
    }
  }
  else if (updateMode == LED_MODE_TOGGLE)
  {
    if (ledMask == LED_ALL)
    {
      ledStateLwrRed = ledStateLwrRed == LED_PWM_ON ? 0 : LED_PWM_ON;
      ledStateLwrGreen = ledStateLwrGreen == LED_PWM_ON ? 0 : LED_PWM_ON;
      ledStateLwrBlue = ledStateLwrBlue == LED_PWM_ON ? 0 : LED_PWM_ON;
      ledStateUprRed = ledStateUprRed == LED_PWM_ON ? 0 : LED_PWM_ON;
      ledStateUprGreen = ledStateUprGreen == LED_PWM_ON ? 0 : LED_PWM_ON;
      ledStateUprBlue = ledStateUprBlue == LED_PWM_ON ? 0 : LED_PWM_ON;
    }
    else
    {
      if (ledMask & LED_LWR_RED)
      {
        ledStateLwrRed = ledStateLwrRed == LED_PWM_ON ? 0 : LED_PWM_ON;
      }
      if (ledMask & LED_LWR_GREEN)
      {
        ledStateLwrGreen = ledStateLwrGreen == LED_PWM_ON ? 0 : LED_PWM_ON;
      }
      if (ledMask & LED_LWR_YELLOW)
      {
        ledStateLwrRed = ledStateLwrRed == LED_PWM_ON ? 0 : LED_PWM_ON;
        ledStateLwrGreen = ledStateLwrGreen == LED_PWM_ON ? 0 : LED_PWM_ON;
      }
      if (ledMask & LED_UPR_GREEN)
      {
        ledStateUprGreen = ledStateUprGreen == LED_PWM_ON ? 0 : LED_PWM_ON;
      }
      if (ledMask & LED_UPR_BLUE)
      {
        ledStateUprBlue = ledStateUprBlue == LED_PWM_ON ? 0 : LED_PWM_ON;
      }
    }
  }
}

uint8_t Board_isLedOnUprBlue(void)
{
  return ledStateUprBlue == LED_PWM_ON ? 1 : 0;
}

uint8_t Board_isLedOnUprGreen(void)
{
  return ledStateUprGreen == LED_PWM_ON ? 1 : 0;
}

#endif

#if defined(SHIMMER3R)
/**
 * @brief  Turn on LEDs.
 * @param  ledMask   Use values defined in HAL_board.h for the LEDs to turn on
 * @return none
 */
void Board_ledOn(uint8_t ledMask)
{
  updateLedState(LED_MODE_ON, ledMask);
  rgb_led_lwr_color(ledStateLwrRed, ledStateLwrGreen, ledStateLwrBlue);
  rgb_led_upr_color(ledStateUprRed, ledStateUprGreen, ledStateUprBlue);
}
#elif if defined(SHIMMER4_SDK)
void Board_ledOn(uint8_t ledMask)
{
  if (ledMask & LED_LWR_RED)
  {
    HAL_GPIO_WritePin(LED_RED_GPIO, LED_RED_PIN, GPIO_PIN_RESET); //red
  }
  if (ledMask & LED_LWR_GREEN)
  {
    HAL_GPIO_WritePin(LED_GR0_GPIO, LED_GR0_PIN, GPIO_PIN_RESET); //green0
  }
  if (ledMask & LED_LWR_YELLOW)
  {
    HAL_GPIO_WritePin(LED_YEL_GPIO, LED_YEL_PIN, GPIO_PIN_RESET); //orange
  }
  if (ledMask & LED_UPR_GREEN)
  {
    HAL_GPIO_WritePin(LED_GR1_GPIO, LED_GR1_PIN, GPIO_PIN_RESET); //green1
  }
  if (ledMask & LED_UPR_BLUE)
  {
    HAL_GPIO_WritePin(LED_BLU_GPIO, LED_BLU_PIN, GPIO_PIN_RESET); //blue
  }
}
#endif

/**
 * @brief  Turn off LEDs
 * @param  ledMask   Use values defined in HAL_board.h for the LEDs to turn off
 * @return none
 */
#if defined(SHIMMER3R)
void Board_ledOff(uint8_t ledMask)
{
  updateLedState(LED_MODE_OFF, ledMask);
  rgb_led_lwr_color(ledStateLwrRed, ledStateLwrGreen, ledStateLwrBlue);
  rgb_led_upr_color(ledStateUprRed, ledStateUprGreen, ledStateUprBlue);
}
#elif if defined(SHIMMER4_SDK)
void Board_ledOff(uint8_t ledMask)
{
  if (ledMask & LED_LWR_RED)
  {
    HAL_GPIO_WritePin(LED_RED_GPIO, LED_RED_PIN, GPIO_PIN_SET); //red
  }
  if (ledMask & LED_LWR_GREEN)
  {
    HAL_GPIO_WritePin(LED_GR0_GPIO, LED_GR0_PIN, GPIO_PIN_SET); //green0
  }
  if (ledMask & LED_LWR_YELLOW)
  {
    HAL_GPIO_WritePin(LED_YEL_GPIO, LED_YEL_PIN, GPIO_PIN_SET); //orange
  }
  if (ledMask & LED_UPR_GREEN)
  {
    HAL_GPIO_WritePin(LED_GR1_GPIO, LED_GR1_PIN, GPIO_PIN_SET); //green1
  }
  if (ledMask & LED_UPR_BLUE)
  {
    HAL_GPIO_WritePin(LED_BLU_GPIO, LED_BLU_PIN, GPIO_PIN_SET); //blue
  }
}
#endif

/**
 * @brief  Toggle LEDs
 * @param  ledMask   Use values defined in HAL_board.h for the LEDs to toggle
 * @return none
 */
#if defined(SHIMMER3R)
void Board_ledToggle(uint8_t ledMask)
{
  updateLedState(LED_MODE_TOGGLE, ledMask);
  rgb_led_lwr_color(ledStateLwrRed, ledStateLwrGreen, ledStateLwrBlue);
  rgb_led_upr_color(ledStateUprRed, ledStateUprGreen, ledStateUprBlue);
}
#elif if defined(SHIMMER4_SDK)
void Board_ledToggle(uint8_t ledMask)
{
  if (ledMask & LED_LWR_RED)
  {
    HAL_GPIO_TogglePin(LED_RED_GPIO, LED_RED_PIN); //red
  }
  if (ledMask & LED_LWR_GREEN)
  {
    HAL_GPIO_TogglePin(LED_GR0_GPIO, LED_GR0_PIN); //green0
  }
  if (ledMask & LED_LWR_YELLOW)
  {
    HAL_GPIO_TogglePin(LED_YEL_GPIO, LED_YEL_PIN); //orange
  }
  if (ledMask & LED_UPR_GREEN)
  {
    HAL_GPIO_TogglePin(LED_GR1_GPIO, LED_GR1_PIN); //green1
  }
  if (ledMask & LED_UPR_BLUE)
  {
    HAL_GPIO_TogglePin(LED_BLU_GPIO, LED_BLU_PIN); //blue
  }
}
#endif

/**
 * @brief  SD power cycle
 * @param  none
 * @return none
 */
void Board_sdPowerCycle(void)
{
  Board_detectN(1);
  Board_setSdPower(0);
  Board_setDockAccessToSd(0);
  HAL_Delay(120);
  Board_setSdPower(1);
  HAL_Delay(50);
  ShimSd_mount(0);
  ShimSd_mount(1);
}

/**
 * @brief  SD control to PC side
 * @param  none
 * @return none
 */
void Board_sd2Pc(void)
{
  //Board_sdPowerCycle();

  ///* ADC pins are shared with two dat pins, ensure both are inputs */
  //HAL_GPIO_DeInit(GPIO_ADC_EXT_EXP0_GPIO_Port, GPIO_ADC_EXT_EXP0_Pin);
  //HAL_GPIO_DeInit(GPIO_ADC_EXT_EXP1_GPIO_Port, GPIO_ADC_EXT_EXP1_Pin);

  //Board_detectN(1);
  HAL_Delay(120);
  Board_setSdPower(0);
  Board_setDockAccessToSd(1);
  //Board_detectN(GPIO_PIN_RESET);
  HAL_Delay(120);
  Board_setSdPower(1);
  HAL_Delay(50);
  Board_detectN(0);
  ShimSd_mount(0);

  mmc1DeInit();
}

/**
 * @brief  SD control to ARM side
 * @param  none
 * @return none
 */
void Board_sd2Arm(void)
{
  Board_detectN(1);
  HAL_Delay(120);
  Board_setSdPower(0);
  Board_setDockAccessToSd(0);
  HAL_Delay(120);
  Board_setSdPower(1);
  HAL_Delay(50);

  MX_SDMMC1_SD_Init();
#if USE_FATFS
  MX_FATFS_Init();
#endif

  ShimSd_mount(0);
  ShimSd_mount(1);
}

/***************************************************************************/
/**
 * @brief  SD power on/off, toggling pin SW_FLASH
 * @param  power_on
 * @return none
 ******************************************************************************/
void Board_setSdPower(uint8_t state)
{
  Board_SW_FLASH(state);
  shimmerStatus.sdPowerOn = state;
}

void Board_setDockAccessToSd(uint8_t mcu0dock1)
{
  Board_sdMcu0Dock1(mcu0dock1);
  shimmerStatus.sdMcu0Pc1 = mcu0dock1;

  //TODO remove below when functionality is working
  //if (mcu0dock1)
  //{
  //  send_test_report("SD to Dock\r\n");
  //}
  //else
  //{
  //  send_test_report("SD to MCU\r\n");
  //}
}

//void Board_detectN(uint8_t on) {
//  if(on){
//     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12,GPIO_PIN_SET);
//  } else{
//     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12,GPIO_PIN_RESET);
//  }
//}

/**
 * use while loop to do delay microseconds
 */
uint32_t multiplier;

void Board_delayMicrosInit(void)
{
  /* While loop takes 2 cycles */
  /* For 1 us delay, we need to divide with 2M */
  multiplier = HAL_RCC_GetSysClockFreq() / 2000000; //HAL_RCC_GetSysClockFreq? HAL_RCC_GetHCLKFreq?
}

void Board_delayMicros(uint32_t micros)
{
  /* Multiply micros with multipler */
  /* Substract 10 */
  micros = (micros * multiplier - 10);
  /* 4 cycles for one loop */
  while (micros--)
    ;
}

#if defined(SHIMMER3R)
void Board_enableSensingPower(sense_pwr_flg_t flag, uint8_t state)
{
  uint8_t originalState = sensePwrFlags;
  if (state)
  {
    sensePwrFlags |= flag;
  }
  else
  {
    sensePwrFlags &= ~flag;
  }

  /* take action if state has changed */
  if ((originalState == 0 && sensePwrFlags != 0)
      || (originalState != 0 && sensePwrFlags == 0))
  {
    Board_SW_PV_SENSE(state);
    Board_SW_PV_SENSE_IO(state);

    //delay to allow voltage to settle after turning on ADC & IMUs etc.
    if (state)
    {
      HAL_Delay(50); //Arbitrary delay to allow chips to power up
    }
    /* TODO can't call HAL_Delay from interrupt (i.e. battery voltage ADC callback) so commenting this out until we decide if it's needed */
    //else
    //{
    //  HAL_Delay(210); //ADXL371 needs 200ms to power down. No harm for other chips.
    //}
  }
}
#endif

void Board_setExpansionBrdPower(uint8_t state)
{
  if (ShimBrd_isExpBrdId(EXP_BRD_GSR_UNIFIED))
  {
    Board_SW_PPG_POWER(state);
  }
  else
  {
    //TODO
  }
  shimmerStatus.pinPvExt = state;
}

void resetGsrPwrAndRange(void)
{
#if SUPPORT_SR48_6_0
  if (ShimBrd_isBoardSr48_6_0())
  {
    Board_SR48_6_0_SW_GSR(0);
  }
#endif
  GSR_setActiveResistor(HW_RES_40K);
}
