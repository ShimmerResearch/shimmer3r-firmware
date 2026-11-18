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

#include <stdint.h>

#ifndef min
#define min(a, b) ((a < b) ? a : b)
#endif

//7 bit address I2C address
//lower 3 bits are highest order bits of memory address
#define CAT24C16_ADDR                0x50

#define CAT24C16_PAGE_SIZE           16
#define CAT24C16_BLOCK_SIZE          256
#define CAT24C16_READ_SIZE           128
#define CAT24C16_TOTAL_SIZE          2048
#define CAT24C16_TEST_OFFSET         16
#define CAT24C16_TEST_SIZE           (8 * CAT24C16_PAGE_SIZE)

//Write cycle time, tWR: 5 ms max (CAT24C16 datasheet, A.C. Characteristics).
//The device does not acknowledge its address until a write cycle has ended.
#define CAT24C16_WRITE_CYCLE_MS      5

//How many times CAT24C16_test() tries to put the test region back
#define CAT24C16_TEST_RESTORE_TRIES  3

#define CAT24C16_OUT_OF_BOUNDS_ERROR 1000

enum CAT24C16_TEST
{
  EEPROM_TEST_PASS = 0,
  EEPROM_TEST_FAIL_INITIAL_BACKUP,
  EEPROM_TEST_FAIL_WRITING_BUF,
  EEPROM_TEST_FAIL_READING_BUF,
  EEPROM_TEST_FAIL_BUF_COMPARISON,
  //Every pass succeeded but the test region could not be put back
  EEPROM_TEST_FAIL_RESTORING_BACKUP,
};

//OR'd into EEPROM_TEST_FAIL_WRITING_BUF, _READING_BUF or _BUF_COMPARISON when,
//after that failure, the test region could not be put back either. Its bytes
//then hold test data, not what was there before the test.
#define EEPROM_TEST_FAIL_NOT_RESTORED 0x80

typedef int32_t (*cat24c16dev_write_ptr)(void *, uint16_t, uint16_t, uint8_t *);
typedef int32_t (*cat24c16dev_read_ptr)(void *, uint16_t, uint16_t, uint8_t *);
typedef void (*cat24c16dev_mdelay_ptr)(uint32_t millisec);
typedef uint32_t (*cat24c16dev_sys_tick_ptr)(void);

typedef struct
{
  /** Component mandatory fields **/
  cat24c16dev_write_ptr write;
  cat24c16dev_read_ptr read;
  /** Component optional fields **/
  cat24c16dev_mdelay_ptr mdelay;
  /** Customizable optional pointer **/
  void *handle;
  cat24c16dev_sys_tick_ptr sys_tick;
} cat24c16dev_ctx_t;

//Read from the CAT24C16 EEPROM
//address = starting address to read from
//length = number of bytes to read
//outBuffer = location to put read bytes
//Note the CAT24C16 had 2048 bytes of storage
//So address + length must be <= 2048
int32_t CAT24C16_read(cat24c16dev_ctx_t *ctx, uint16_t address, uint16_t length, uint8_t *outBuffer);

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
int32_t CAT24C16_write(cat24c16dev_ctx_t *ctx, uint16_t address, uint16_t length, uint8_t *data);

//Write/read test over CAT24C16_TEST_SIZE bytes from CAT24C16_TEST_OFFSET.
//The bytes there are backed up first and, once anything has been written,
//put back and verified whatever the outcome.
//Returns EEPROM_TEST_PASS or an enum CAT24C16_TEST failure, with
//EEPROM_TEST_FAIL_NOT_RESTORED set if the region could not be put back.
uint8_t CAT24C16_test(cat24c16dev_ctx_t *ctx);

#endif
