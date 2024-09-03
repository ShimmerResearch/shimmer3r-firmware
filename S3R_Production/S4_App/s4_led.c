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

#include "s4_led.h"

uint8_t cntBlink = 0;
uint8_t cnt1 = 0;

extern boot_stage_t getBootStage(void);

void S4Led_Blink(void)
{
  boot_stage_t bootStage = getBootStage();
  if (bootStage != BOOT_STAGE_END)
  {
    switch (bootStage)
    {
    case BOOT_STAGE_I2C:
      Board_ledToggle(LED_RED);
      break;
    case BOOT_STAGE_BLUETOOTH_FAILURE:
      Board_ledToggle(LED_YELLOW);
      break;
    default:
      break;
    }
    return;
  }

#if USE_DEFAULT_LED
  //batt leds:
  uint32_t batt_led = 0;
  if (!stat.isDocked)
  {
    if (cntBlink == 0)
    {
      batt_led = stat.battStatLed;
    }
  }
  else
  {
    if (((stat.battVal[2] & 0x80) == 0) && ((stat.battVal[2] & 0x40) != 0))
    { //charge done
#if defined(SHIMMER3R)
      batt_led = LED_RGB_GREEN;
#elif defined(SHIMMER4_SDK)
      batt_led = LED_GREEN_LWR;
#endif
    }
    else if (((stat.battVal[2] & 0x80) != 0) && ((stat.battVal[2] & 0x40) == 0))
    { //charge in progress
#if defined(SHIMMER3R)
      batt_led = LED_RGB_YELLOW;
#elif defined(SHIMMER4_SDK)
      batt_led = LED_YELLOW_LWR;
#endif
    }
    else if (((stat.battVal[2] & 0x80) != 0) && ((stat.battVal[2] & 0x40) != 0))
    { //stand by
#if defined(SHIMMER3R)
      batt_led = LED_RGB_ALL_OFF;
#elif defined(SHIMMER4_SDK)
      batt_led = LED_ALL_BATT;
#endif
    }
    else
    { //bad battery
#if defined(SHIMMER3R)
      batt_led = LED_RGB_RED;
#elif defined(SHIMMER4_SDK)
      batt_led = LED_RED_LWR;
#endif
    }
  }
#if defined(SHIMMER3R)
  Board_ledLwrSetColour(batt_led);
#elif defined(SHIMMER4_SDK)
  Board_ledOn(batt_led);
  Board_ledOff(LED_ALL_BATT - batt_led);
#endif
  ////yellow = stat1
  ////green0 = stat2
  //Board_ledOff(LED_RED + LED_YELLOW + LED_GREEN0);
  //if((stat.battVal[2]&0x80) == 0){
  //   Board_ledOn(LED_GREEN0);
  //}
  //if((stat.battVal[2]&0x40) == 0){
  //   Board_ledOn(LED_YELLOW);
  //}

  if (stat.testResult)
  {
#if defined(SHIMMER3R)
    Board_ledLwrSetColourRgb((cntBlink % 2) ? LED_PWM_OFF : LED_PWM_ON, -1, -1);
#elif defined(SHIMMER4_SDK)
    if (cntBlink % 2)
    {
      Board_ledOff(LED_RED_LWR);
    }
    else
    {
      Board_ledOn(LED_RED_LWR);
    }
#endif
  }
  if (stat.isButtonPressed)
  {
#if defined(SHIMMER3R)
    Board_ledLwrSetColourRgb(-1, LED_PWM_ON, -1);
#elif defined(SHIMMER4_SDK)
    Board_ledOn(LED_GREEN_LWR);
#endif
  }

  if (!stat.isDocked && (stat.badFile || !stat.isSdInserted)
      && S4Ram_getStoredConfig()->sdErrorEnable)
  {
    if (cntBlink % 2)
    {
#if defined(SHIMMER3R)
      Board_ledUprSetColourRgb(-1, LED_PWM_OFF, LED_PWM_ON);
#elif defined(SHIMMER4_SDK)
      Board_ledOn(LED_BLUE_UPR);
      Board_ledOff(LED_GREEN_UPR);
#endif
    }
    else
    {
#if defined(SHIMMER3R)
      Board_ledUprSetColourRgb(-1, LED_PWM_ON, LED_PWM_OFF);
#elif defined(SHIMMER4_SDK)
      Board_ledOff(LED_BLUE_UPR);
      Board_ledOn(LED_GREEN_UPR);
#endif
    }
  }
  else
  {
    //green1
    uint8_t greenUprStateToSet = 0;
    if (stat.isConfiguring)
    {
      greenUprStateToSet = (cntBlink % 2) ? 0 : 1;
    }
    else if (stat.isSensing)
    {
#if defined(SHIMMER4_SDK)
      greenUprStateToSet = (cntBlink >= 10) ? 0 : 1;
#endif
    }
#if defined(SHIMMER4_SDK)
    else if (stat.isSdInserted)
    {
      greenUprStateToSet = (cntBlink == 0) ? 1 : 0;
    }
#endif
    else
    {
      greenUprStateToSet = 0;
    }

#if defined(SHIMMER4_SDK)
    if (greenUprStateToSet)
    {
      Board_ledOn(LED_GREEN_UPR);
    }
    else
    {
      Board_ledOff(LED_GREEN_UPR);
    }
#endif

    //blue
    uint8_t blueUprStateToSet = 0;
    if (stat.isBtPoweredOn)
    {
      if (!stat.isBtConnected)
      {
        blueUprStateToSet = (cntBlink == 0) ? 1 : 0;
      }
      else if (stat.isStreaming)
      {
        if (!(cntBlink % 10))
        {
          blueUprStateToSet = isLedOnUprBlue() ? 0 : 1;
        }
        else
        {
          blueUprStateToSet = isLedOnUprBlue();
        }
      }
      else
      {
        blueUprStateToSet = 1;
      }
    }
    else
    {
      blueUprStateToSet = 0;
    }

#if defined(SHIMMER4_SDK)
    if (blueUprStateToSet)
    {
      Board_ledOn(LED_BLUE_UPR);
    }
    else
    {
      Board_ledOff(LED_BLUE_UPR);
    }
#endif

#if defined(SHIMMER3R)
    //TODO handle different LED blinks for logging/streaming
    //if (stat.isStreaming && stat.isLogging)
    //{
    //
    //}
    //else if (stat.isStreaming)
    //{
    //  if(!(cntBlink % 10))
    //  {
    //    blueUprStateToSet = isLedOnUprBlue()? 0:1;
    //  }
    //}
    //else if (stat.isLogging)
    //{
    //  if(!(cntBlink % 10))
    //  {
    //    greenUprStateToSet = isLedOnUprGreen()? 0:1;
    //  }
    //}

    Board_ledUprSetColourRgb(-1, greenUprStateToSet ? LED_PWM_ON : LED_PWM_OFF,
        blueUprStateToSet ? LED_PWM_ON : LED_PWM_OFF);
#endif
  }

  if (++cntBlink >= 20)
  {
    cntBlink = 0;
    cnt1++;
    if (cnt1 == 10)
    {
      cnt1 = 0;
      __NOP();
    }
  }
#if defined(SHIMMER4_SDK)
  if (!(cntBlink % 10))
  {
#if USE_VBATT_ALWAYS
    if (!(stat.isSensing || stat.isConfiguring))
#endif
    {
      S4_Task_set(TASK_BATTREAD);
    }
  }
#endif
  //DockedCheck();//every 0.1s

#endif //USE_DEFAULT_LED
}
