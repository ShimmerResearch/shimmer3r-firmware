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

#ifndef S4_RAM_H
#define S4_RAM_H

#include "s4.h"
#include "s4__cfg.h"

#define STOREDCONFIG_SIZE 512

void S4Ram_init(void);   
uint8_t* S4Ram_getStoredConfig(void);
uint8_t* S4Ram_getSdHeadText(void);
void S4Ram_SetDefaultInfomem(void);

uint8_t S4Ram_sdHeadTextSet(const uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_sdHeadTextGet(uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_sdHeadTextGetByte(uint16_t offset);
uint8_t S4Ram_sdHeadTextSetByte(uint16_t offset, uint8_t val);

uint8_t S4Ram_storedConfigSet(const uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_storedConfigGet(uint8_t* buf, uint16_t offset, uint16_t length);
uint8_t S4Ram_storedConfigGetByte(uint16_t offset);
uint8_t S4Ram_storedConfigSetByte(uint16_t offset, uint8_t val);

void S4Ram_btMacAsciiSet(const uint8_t* buf);
void S4Ram_btMacAsciiGet(uint8_t* buf);
void S4Ram_btMacHexSet(const uint8_t* buf);
void S4Ram_btMacHexGet(uint8_t* buf);

void S4Ram_config2SdHead(void);

#endif //S4Ram_H
