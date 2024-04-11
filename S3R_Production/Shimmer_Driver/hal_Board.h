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

#if defined(SHIMMER3R)
#define LED_RED_GPIO GPIOG
#define LED_RED_PIN  LED_LWR_RD_Pin
#define LED_GR0_GPIO LED_LWR_RD_GPIO_Port
#define LED_GR0_PIN  LED_LWR_GR_Pin
#define LED_YEL_GPIO LED_LWR_GR_GPIO_Port
#define LED_YEL_PIN  LED_LWR_BLU_Pin
#define LED_GR1_GPIO LED_UPR_GR_GPIO_Port
#define LED_GR1_PIN  LED_UPR_GR_Pin
#define LED_BLU_GPIO LED_UPR_BLU_GPIO_Port
#define LED_BLU_PIN  LED_UPR_BLU_Pin

#define ECG_CS_GPIO  GPIO_ADC_INT_EXP1_GPIO_Port
#define ECG_CS_PIN   GPIO_ADC_INT_EXP1_Pin
#define RESP_CS_GPIO GPIO_ADC_INT_EXP0_GPIO_Port
#define RESP_CS_PIN  GPIO_ADC_INT_EXP0_Pin

#define GSR_RANGE_A0_Pin  GPIO_ADC_INT_EXP3_Pin
#define GSR_RANGE_A1_Pin  GPIO_ADC_INT_EXP2_Pin

#define SW_STRAIN_GAUGE_Pin GPIO_INTERNAL1_Pin
#define SW_STRAIN_GAUGE_GPIO_Port GPIO_INTERNAL1_GPIO_Port
#define SW_BRIDGE_AMP_Pin GPIO_INTERNAL0_Pin
#define SW_BRIDGE_AMP_GPIO_Port GPIO_INTERNAL0_GPIO_Port
#define SW_PPG_EN_Pin GPIO_INTERNAL1_Pin
#define SW_PPG_EN_GPIO_Port GPIO_INTERNAL1_GPIO_Port

#define ADC_CHANNEL_ACCEL_X ADC_CHANNEL_0
#define ADC_CHANNEL_ACCEL_Y ADC_CHANNEL_1
#define ADC_CHANNEL_ACCEL_Z ADC_CHANNEL_2
#define ADC_CHANNEL_VBATT ADC_CHANNEL_VBAT

#if defined(SHIMMER3R)
#define ADC_CHANNEL_EXT_A0 ADC_CHANNEL_9
#define ADC_CHANNEL_EXT_A1 ADC_CHANNEL_11
#define ADC_CHANNEL_EXT_A2 ADC_CHANNEL_12
#define ADC_CHANNEL_INT_A0 ADC_CHANNEL_10
#define ADC_CHANNEL_INT_A1 ADC_CHANNEL_15
#define ADC_CHANNEL_INT_A2 ADC_CHANNEL_16
#define ADC_CHANNEL_INT_A3 ADC_CHANNEL_17
#elif defined(SHIMMER4_SDK)
//TODO check Shimmer4 channel numbers
#define ADC_CHANNEL_EXT_A0 ADC_CHANNEL_9
#define ADC_CHANNEL_EXT_A1 ADC_CHANNEL_10
#define ADC_CHANNEL_EXT_A2 ADC_CHANNEL_11
#define ADC_CHANNEL_INT_A0 ADC_CHANNEL_12
#define ADC_CHANNEL_INT_A1 ADC_CHANNEL_15
#define ADC_CHANNEL_INT_A2 ADC_CHANNEL_16
#define ADC_CHANNEL_INT_A3 ADC_CHANNEL_17
#endif

#elif defined(SHIMMER4_SDK)

#define LED_RED_GPIO GPIOG
#define LED_RED_PIN  LED_RD_Pin
#define LED_GR0_GPIO GPIOC
#define LED_GR0_PIN  LED_GR_Pin
#define LED_YEL_GPIO GPIOC
#define LED_YEL_PIN  LED_YE_Pin
#define LED_GR1_GPIO LED_GR1_GPIO_Port
#define LED_GR1_PIN  LED_GR1_Pin
#define LED_BLU_GPIO LED_BLU_GPIO_Port
#define LED_BLU_PIN  LED_BLU_Pin

#define ECG_CS_GPIO  ECG_NSEL_GPIO_Port
#define ECG_CS_PIN   ECG_NSEL_Pin
#define RESP_CS_GPIO RESP_NSEL_GPIO_Port
#define RESP_CS_PIN  RESP_NSEL_Pin

#define GSR_RANGE_A0_Pin  GPIO_INTERNAL3_Pin
#define GSR_RANGE_A1_Pin  GPIO_INTERNAL2_Pin

#define BT_RTS_GPIO_Port BTH_RTS_GPIO_Port
#define BT_RTS_Pin BTH_RTS_Pin

#define USER_BTN_N_GPIO_Port USER_N_GPIO_Port
#define USER_BTN_N_Pin USER_BTN_N_Pin

#define BT_CONNECTION_Pin BTH_STATUS_Pin
#define DOCK_DETECT_Pin DOCK_Pin

#define GPIO_INTERNAL0_Pin GPIO_INTERNAL_Pin

#define SW_I2C_GPIO_Port SW_I2C_GPIO_Port
#define SW_I2C_Pin SW_I2C_Pin

#define SW_I2C2_GPIO_Port SW_I2C_GPIO_Port
#define SW_I2C2_Pin SW_I2C_Pin

#define SD_PC_SWITCH_Pin EXT_MEM_Pin

#define SW_STRAIN_GAUGE_Pin GPIO_INTERNAL4_Pin
#define SW_STRAIN_GAUGE_GPIO_Port GPIOB
#define SW_PPG_EN_Pin PPG_EN_Pin
#define SW_PPG_EN_GPIO_Port GPIOF
#define GPIOF GPIO_INTERNAL1_GPIO_Port

#define ADC_CHANNEL_ACCEL_X ADC_CHANNEL_13
#define ADC_CHANNEL_ACCEL_Y ADC_CHANNEL_14
#define ADC_CHANNEL_ACCEL_Z ADC_CHANNEL_15
#define ADC_CHANNEL_VBATT ADC_CHANNEL_3

//TODO update Ax number to be more generic in the code between hardware
#define ADC_CHANNEL_EXT_A0 ADC_CHANNEL_9
#define ADC_CHANNEL_EXT_A1 ADC_CHANNEL_8
#define ADC_CHANNEL_EXT_A2 ADC_CHANNEL_1
#define ADC_CHANNEL_INT_A0 ADC_CHANNEL_10
#define ADC_CHANNEL_INT_A1 ADC_CHANNEL_11
#define ADC_CHANNEL_INT_A2 ADC_CHANNEL_0
#define ADC_CHANNEL_INT_A3 ADC_CHANNEL_2
#define ADC_CHANNEL_INT_A4 ADC_CHANNEL_12

#endif

//exp_reset_n is used by RESETN of exg*2 and VCC of eeprom
#if defined(SHIMMER3R)
#define Board_SW_I2C(x)       HAL_GPIO_WritePin(SW_I2C2_GPIO_Port, SW_I2C2_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)
#define Board_EXG_RESET_N(x)  HAL_GPIO_WritePin(GPIO_INTERNAL2_GPIO_Port, GPIO_INTERNAL2_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)
#elif defined(SHIMMER4_SDK)
#define Board_SW_EXP(x)       HAL_GPIO_WritePin(EXP_RESET_N_GPIO_Port, EXP_RESET_N_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)
#define Board_SW_I2C(x)       HAL_GPIO_WritePin(SW_I2C_GPIO_Port, SW_I2C_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)
#define Board_EXG_RESET_N(x)  HAL_GPIO_WritePin(SW_I2C_GPIO_Port, SW_I2C_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)
#endif

#define Board_ExG_CS(x)       HAL_GPIO_WritePin(GPIOA,        GPIO_PIN_4,   x?GPIO_PIN_SET:GPIO_PIN_RESET)
#define Board_ECG_CS(x)       HAL_GPIO_WritePin(ECG_CS_GPIO,  ECG_CS_PIN,   x?GPIO_PIN_SET:GPIO_PIN_RESET)  
#define Board_RESP_CS(x)      HAL_GPIO_WritePin(RESP_CS_GPIO, RESP_CS_PIN,  x?GPIO_PIN_SET:GPIO_PIN_RESET)  

#define Board_arm0pc1(x)      HAL_GPIO_WritePin(GPIOG, SW_SD_MCU_DOCK_Pin,  x?GPIO_PIN_SET:GPIO_PIN_RESET)  //EXT_MEM: 0 for arm, 1 for pc
#define Board_sdPower(x)      HAL_GPIO_WritePin(SW_FLASH_GPIO_Port, SW_FLASH_Pin, x?GPIO_PIN_SET:GPIO_PIN_RESET)
#define Board_detectN(x)      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12,  x?GPIO_PIN_SET:GPIO_PIN_RESET)
#endif //SHIMMER4_SDK_CONFIG_H


#ifdef SHIMMER4_CONFIG_H


#endif //SHIMMER4_CONFIG_H

#endif
