/*
 * Copyright (c) 2013, Shimmer Research, Ltd.
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
 * @date May, 2013
 */

#ifndef S4_TASKLIST_H
#define S4_TASKLIST_H
#include <stdint.h>

#include "s4.h"
#include "s4__cfg.h"

#if USE_FREERTOS
#define S4_Task_init       S4_RTOS_Task_init
#define S4_Task_manage     S4_RTOS_Task_manage
#define S4_Task_getCurrent S4_RTOS_Task_getCurrent
#define S4_Task_clear      S4_RTOS_Task_clear
#define S4_Task_set        S4_RTOS_Task_set
#define S4_Task_getList    S4_RTOS_Task_getList
#else
#define S4_Task_init       S4_NORM_Task_init
#define S4_Task_manage     S4_NORM_Task_manage
#define S4_Task_getCurrent S4_NORM_Task_getCurrent
#define S4_Task_clear      S4_NORM_Task_clear
#define S4_Task_set        S4_NORM_Task_set
#define S4_Task_getList    S4_NORM_Task_getList
#endif //USE_FREERTOS

#define TASK_SIZE 31

typedef enum
{
  TASK_NONE = 0,
  //TASK_TESTSDSTOP    = 0x1,
  //TASK_TESTSDSTART   = 0x1<<1,
  TASK_DOCKSETUP = 0x1 << 2,
  //TASK_SETCONFIG       = 0x1<<2,
  TASK_UARTPROCESS = 0x1 << 3,
  TASK_UARTRESPONSE = 0x1 << 4,
  TASK_BTPROCESS = 0x1 << 5,
  TASK_BTRESPONSE = 0x1 << 6,
  //TASK_CFGCH         = 0x1<<7,
  TASK_RCNODER10 = 0x01 << 8,
  TASK_RCCENTERR1 = 0x01 << 9,
  TASK_SDWRITE = 0x1 << 10,
  TASK_STARTSENSING = 0x1 << 11,
  TASK_STOPSENSING = 0x1 << 12,
  TASK_STREAMDATA = 0x1 << 13,
  //TASK_RADIOTX         = 0x1<<14,
  TASK_SDLOG_CFG_UPDATE = 0x1 << 15,
  TASK_BATT_READ_FROM_ALARM = 0x1 << 16,
#if defined(SHIMMER3R)
  TASK_SAVEDATA = 0x1 << 17,
#elif defined(SHIMMER4_SDK)
//TASK_HRCALC        = 0x1<<17,
#endif
  //TASK_TESTLEDS      = 0x1<<18,
  //TASK_DELAYEDRADIOISR = 0x1<<19,
  TASK_NEXTSENSOR = 0x1 << 20
} TASK_FLAGS;
//return the task id of the current task

void S4_NORM_Task_init(void);

void S4_NORM_Task_manage(void);

uint32_t S4_NORM_Task_getCurrent(void);

//clear the task from the tasklist
void S4_NORM_Task_clear(uint32_t task_id);

//add the task to the tasklist
uint8_t S4_NORM_Task_set(uint32_t task_id);

//return the whole tasklist
uint32_t S4_NORM_Task_getList(void);

uint8_t setTaskNewBtCmdToProcess(void);
void setStartSensing(void);
void setStopSensing(void);

#endif //S4_TASKLIST_H
