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
 
 
 /*
SLEEP mode

Cortex-M4 core is stopped.
Peripherals kept running.
How to exit this mode?
Any peripheral interrupt acknowledged by the nested vectored interrupt controller (NVIC).
Systick timer will also wake up MCU!
After exit, MCU continues where it stopped.

STOP mode

All clocks in 1.2V domain are stopped.
PLL, HSI and HSE RC oscillators disabled.
Internal SRAM and register contents are preserved.
Voltage regulator in low-power mode.
How to exit this mode?
Any EXTI Line (Internal or External) configured in Interrupt/Event mode.
After exit, MCU continue there where it stopped. HSI is used for system core clock, but my library set it back to PLL as source.

STANDBY mode

Cortex-M4 deepsleep mode.
Voltage regulator disabled.
1.2V domain consequently powered off.
PLL, HSI and HSE RC oscillators disabled.
SRAM and register contents are lost except for the RTC registers, RTC backup registers, backup SRAM and Standby circuitry.
How to exit this mode?
Wakeup pin rising edge.
RTC alarm (Alarm A and B) and wakeup.
Tamper and time-stamp event.
External reset in NRST pin.
IWDG reset.
After exit, STM32F4xx reset.

VBAT mode

Main digital supply is turned off.
The circuit is supplied through VBAT pin which should be connected to an external supply voltage (a battery or any other source).
RTC is running in this mode.
How to enter this mode?
If you disable main power and put supply voltage on VBAT pin, then MCU will be in VBAT mode.
How to exit this mode?
Put power supply on main Vdd pins.
*/


#include<stdint.h>

#ifndef HAL_POWER_H
#define HAL_POWER_H

// wait for reset to exit
void Power_Standby(void);


// USAGE:
//   /* Checks if reset was because of wakeup from standby */
//   uint8_t i;
//   if (StandbyReset()) {
//      for (i = 0; i < 10; i++) {
//         /* Toggle LED red to indicate this */
//         HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_5);//red
//         /* Delay */
//         HAL_Delay(100);
//      }
//   }
uint8_t Power_StandbyReset(void);


// wait for any int to exit
void Power_SleepUntilInterrupt(void);


//wait for exti to exit
void Power_StopUntilInterrupt(void); 
void Power_GpioAnalogConfig (void);

#endif
