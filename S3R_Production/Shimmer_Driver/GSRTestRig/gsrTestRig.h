/*Copyright (c) 2017, Shimmer Research, Ltd.
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
 * @author Sam O'Mahony
 * @date December, 2017
 *
 */

#ifndef GSR_TEST_RIG_H
#define GSR_TEST_RIG_H

#include <stdint.h>

#include "stm32u5xx.h"

/* Each bit of the 8-bit serial word corresponds to one switch of the device. A
 * Logic 1 in the bit position turns the switch on, and a Logic 0 turns the
 * switch off. */
#define ADG715_SWITCH_RESET 0x00 //All switches open
#define ADG715_SETTING      0xF0 //Switches S5-8 closed - U1.1 in series
#define ADG715_A_B_PARALLEL \
  0xF1 //Switches S1 and S5-8 closed - U1.1 & U1.2 in parallel
#define ADG715_ALL_SERIES       0b01011010 //Switches S2, S4, S5, S7 closed

#define ADG715_S1               (1 << 0)
#define ADG715_S2               (1 << 1)
#define ADG715_S3               (1 << 2)
#define ADG715_S4               (1 << 3)
#define ADG715_S5               (1 << 4)
#define ADG715_S6               (1 << 5)
#define ADG715_S7               (1 << 6)
#define ADG715_S8               (1 << 7)

#define RDAC1                   0x00 //Wiper 1 address
#define RDAC2                   0x80 //Wiper 2 address

#define ADDR_U1_AD5242          0x2D //7 bit address I2C address
#define ADDR_U2_AD5242          0x2C //7 bit address I2C address
#define ADDR_U3_ADG715          0x48 //7 bit address I2C address

#define AD5242_1M_RAB           1000000L
#define AD5242_1M_TOTAL_RES     (AD5242_1M_RAB + 60)
#define ADG715_1M_TOTAL_RES_MIN 3906 //3966.25-60
#define TOTAL_RES_2_PARALLEL    500000
#define TOTAL_RES_3_PARALLEL    333333

//Function definitions:
HAL_StatusTypeDef setADG715SwitchMode(uint8_t value);
void digital_pot(void);
HAL_StatusTypeDef gsrTestRigInit(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef setGsrTestRigResistance(uint32_t resistance);
HAL_StatusTypeDef
setGsrTestRig(uint8_t switchMode, uint8_t u1Rdac1, uint8_t u1Rdac2, uint8_t u2Rdac1, uint8_t u2Rdac2);
uint8_t calculateRdacValue(uint32_t resistance);

void run_gsrTestRig(uint8_t *config);
void digital_pot(void);

#endif //GSR_TEST_RIG_H
