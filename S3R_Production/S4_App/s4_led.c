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
#include "shimmer_definitions.h"
#include "log_and_stream_externs.h"

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
  if (batteryStatus.battStatLedFlash)
  {
    batt_led = cntBlink % 2 ? batteryStatus.battStatLedCharging : LED_RGB_ALL_OFF;
  }
  else
  {
    if (shimmerStatus.docked)
    {
      batt_led = batteryStatus.battStatLedCharging;
    }
    else if (cntBlink == 0)
    {
      batt_led = batteryStatus.battStatLed;
    }
  }
  Board_ledLwrSetColour(batt_led);

  if (shimmerStatus.buttonPressed)
  {
#if defined(SHIMMER3R)
    Board_ledLwrSetColourRgb(-1, LED_PWM_ON, -1);
#elif defined(SHIMMER4_SDK)
    Board_ledOn(LED_GREEN_LWR);
#endif
  }

  if (!shimmerStatus.docked && (shimmerStatus.sdBadFile || !shimmerStatus.sdInserted)
      && S4Ram_getStoredConfig()->sdErrorEnable)
  {
    //Alternate Red/Yellow for SD error
    if (cntBlink % 2)
    {
#if defined(SHIMMER3R)
      Board_ledUprSetColourRgb(LED_PWM_ON, LED_PWM_ON, LED_PWM_OFF); //Yellow
#elif defined(SHIMMER4_SDK)
      Board_ledOn(LED_BLUE_UPR);
      Board_ledOff(LED_GREEN_UPR);
#endif
    }
    else
    {
#if defined(SHIMMER3R)
      Board_ledUprSetColourRgb(LED_PWM_ON, LED_PWM_OFF, LED_PWM_OFF); //Red
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
    if (shimmerStatus.configuring)
    {
      greenUprStateToSet = (cntBlink % 2) ? 0 : 1;
    }
    else if (shimmerStatus.sdLogging)
    {
      greenUprStateToSet = (cntBlink >= 10) ? 0 : 1;
    }
#if defined(SHIMMER4_SDK)
    else if (shimmerStatus.isSdInserted)
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
    if (isBtIsInitialised())
    {
      if (!shimmerStatus.btConnected)
      {
        blueUprStateToSet = (cntBlink == 0) ? 1 : 0;
      }
      else if (shimmerStatus.btStreaming)
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
    //if (shimmerStatus.isStreaming && shimmerStatus.isLogging)
    //{
    //
    //}
    //else if (shimmerStatus.isStreaming)
    //{
    //  if(!(cntBlink % 10))
    //  {
    //    blueUprStateToSet = isLedOnUprBlue()? 0:1;
    //  }
    //}
    //else if (shimmerStatus.isLogging)
    //{
    //  if(!(cntBlink % 10))
    //  {
    //    greenUprStateToSet = isLedOnUprGreen()? 0:1;
    //  }
    //}

    Board_ledUprSetColourRgb(LED_PWM_OFF, greenUprStateToSet ? LED_PWM_ON : LED_PWM_OFF,
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
    if (!(shimmerStatus.isSensing || shimmerStatus.isConfiguring))
#endif
    {
      S4_Task_set(TASK_BATTREAD);
    }
  }
#endif
  //DockedCheck();//every 0.1s

#endif //USE_DEFAULT_LED
}
