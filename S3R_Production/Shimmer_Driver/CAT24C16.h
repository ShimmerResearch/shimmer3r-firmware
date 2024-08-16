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

#ifndef CAT24C16_H
#define CAT24C16_H

#include "stm32u5xx_hal.h"
#include "string.h"
#include <stdint.h>

#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

#define CAT24C16_ADDR        0x50

#define CAT24C16_PAGE_SIZE   16
#define CAT24C16_BLOCK_SIZE  256
#define CAT24C16_READ_SIZE   128
#define CAT24C16_TOTAL_SIZE  2048
#define CAT24C16_TEST_OFFSET 16
#define CAT24C16_TEST_SIZE   128

enum EEPROM_RW
{
  EEPROM_READ = 0,
  EEPROM_WRITE = 1,
};

//pass over the i2c handler pointer
void CAT24C16_init(I2C_HandleTypeDef *hi2c);

//power on the CAT24C16 chip
void CAT24C16_powerOn(void);

//power off the CAT24C16 chip
void CAT24C16_powerOff(void);

//Read from the CAT24C16 EEPROM
//address = starting address to read from
//length = number of bytes to read
//outBuffer = location to put read bytes
//Note the CAT24C16 had 2048 bytes of storage
//So address + length must be <= 2048
void CAT24C16_read(uint16_t address, uint8_t *outBuffer, uint16_t length);

//Write to the CAT24C16 EEPROM
//address = starting address to write to
//length = number of bytes to write
//data = bytes to write
//Note the CAT24C16 had 2048 bytes of storage
//So address + length must be <= 2048
//It takes 5ms per write cycle
//Each write cycle can write up to 16bytes of data
//but only within a 16-byte page (of which there are
//128 in the CAT24C16)
void CAT24C16_write(uint16_t address, uint8_t *data, uint16_t length);

uint8_t CAT24C16_test(void);

void eepromRead(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf);
void eepromWrite(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf);
void eepromReadWrite(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf, enum EEPROM_RW eepromRW);

#endif
