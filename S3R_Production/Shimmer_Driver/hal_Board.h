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
 


#include<stdint.h>
#include "s4__cfg.h"

#ifndef HAL_BOARD_H
#define HAL_BOARD_H

#define LED_RED 	   0x01 // == BATT_LOW
#define LED_YELLOW   0x02 // == BATT_MID
#define LED_GREEN0	0x04 // == BATT_HIGH
#define LED_ALL_BATT (LED_RED + LED_YELLOW + LED_GREEN0)
#define LED_GREEN1	0x08
#define LED_BLUE	   0x10
#define LED_ALL	   0xFF
   

extern void Board_ledOn(uint8_t ledMask);
extern void Board_ledOff(uint8_t ledMask);
extern void Board_ledToggle(uint8_t ledMask);

extern void Board_sd2Pc(void);
extern void Board_sd2Arm(void);
//extern void Board_sdPower(uint8_t on);
extern void Board_sdPowerCycle(void);

extern void Board_delayMicrosInit(void);
extern void Board_delayMicros(uint32_t micros);

// pin/gpios might be different for sdk and Shimmer4, or any later developed boards
#ifdef SHIMMER4_SDK_CONFIG_H

#define LED_RED_GPIO GPIOG
#define LED_RED_PIN  CHG_LED_RD_Pin
#define LED_GR0_GPIO GPIOC
#define LED_GR0_PIN  CHG_LED_GR_Pin
#define LED_YEL_GPIO GPIOC
#define LED_YEL_PIN  CHG_LED_YE_Pin
#define LED_GR1_GPIO LED_GR1_GPIO_Port
#define LED_GR1_PIN  LED_GR1_Pin
#define LED_BLU_GPIO LED_BLU_GPIO_Port
#define LED_BLU_PIN  LED_BLU_Pin

#define ECG_CS_GPIO  ECG_NSEL_GPIO_Port
#define ECG_CS_PIN   ECG_NSEL_Pin
#define RESP_CS_GPIO RESP_NSEL_GPIO_Port
#define RESP_CS_PIN  RESP_NSEL_Pin

//exp_reset_n is used by RESETN of exg*2 and VCC of eeprom
#define Board_SW_EXP(x)       HAL_GPIO_WritePin(EXP_RESET_N_GPIO_Port, EXP_RESET_N_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)
#define Board_SW_I2C(x)       HAL_GPIO_WritePin(SW_I2C_GPIO_Port, SW_I2C_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)

#define Board_ExG_CS(x)       HAL_GPIO_WritePin(GPIOA,        GPIO_PIN_4,   x?GPIO_PIN_SET:GPIO_PIN_RESET)
#define Board_ECG_CS(x)       HAL_GPIO_WritePin(ECG_CS_GPIO,  ECG_CS_PIN,   x?GPIO_PIN_SET:GPIO_PIN_RESET)  
#define Board_RESP_CS(x)      HAL_GPIO_WritePin(RESP_CS_GPIO, RESP_CS_PIN,  x?GPIO_PIN_SET:GPIO_PIN_RESET)  

#define Board_arm0pc1(x)      HAL_GPIO_WritePin(GPIOG, EXT_MEM_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)  //EXT_MEM: 0 for arm, 1 for pc
#define Board_sdPower(x)      HAL_GPIO_WritePin(SW_FLASH_GPIO_Port, SW_FLASH_Pin, x?GPIO_PIN_SET:GPIO_PIN_RESET)
#define Board_detectN(x)      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12,  x?GPIO_PIN_SET:GPIO_PIN_RESET)
#endif //SHIMMER4_SDK_CONFIG_H


#ifdef SHIMMER4_CONFIG_H


#endif //SHIMMER4_CONFIG_H

#endif
