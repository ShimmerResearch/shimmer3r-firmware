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
 * @author Mark Nolan
 * @date June, 2024
 *
 */

#include "CAT24C16.h"

#include <stdlib.h>
#include <string.h>

int32_t CAT24C16_read(cat24c16dev_ctx_t *ctx, uint16_t address, uint16_t length, uint8_t *outBuffer)
{
  if ((!length) || (length > CAT24C16_TOTAL_SIZE) || (address + length > CAT24C16_TOTAL_SIZE))
  {
    return CAT24C16_OUT_OF_BOUNDS_ERROR;
  }
  return ctx->read(ctx->handle, address, length, outBuffer);
}

int32_t CAT24C16_write(cat24c16dev_ctx_t *ctx, uint16_t address, uint16_t length, uint8_t *data)
{
  if ((!length) || (length > CAT24C16_TOTAL_SIZE) || (address + length > CAT24C16_TOTAL_SIZE))
  {
    return CAT24C16_OUT_OF_BOUNDS_ERROR;
  }

  int32_t result = 0;
  uint8_t addr_hi = CAT24C16_ADDR | (address >> 8);
  //uint8_t addr_lo = address & 0xff;

  uint8_t buf[17], margin, this_write, inc_addr; //17 = PAGE_SIZE+1
  uint16_t mem_ptr, buf_offset;

  inc_addr = 0;
  buf_offset = 0;
  mem_ptr = address;
  this_write = 0;

  while (mem_ptr < address + length)
  {
    //reassign I2C slave address
    if (inc_addr)
    {
      inc_addr = 0;
      //I2C_Set_Slave_Address(++final_addr);
      ++addr_hi;
    }

    //delay

    //bounds check
    margin = CAT24C16_PAGE_SIZE - (mem_ptr % CAT24C16_PAGE_SIZE);
    this_write = min(margin, address + length - mem_ptr);

    //set byte address
    buf[0] = mem_ptr & 0xff;
    //data copy
    memcpy(buf + 1, data + buf_offset, this_write);

    //Write to EEPROM:
    result = ctx->write(ctx->handle, addr_hi << 1, this_write + 1, buf);
    if (result)
    {
      break;
    }
    ctx->mdelay(5);

    mem_ptr += this_write;
    buf_offset += this_write;

    //if reaches edge of flash mem, reassign the slave address
    if (!(mem_ptr % CAT24C16_BLOCK_SIZE))
      inc_addr = 1;
  }

  return result;
}

uint8_t CAT24C16_test(cat24c16dev_ctx_t *ctx)
{
  int32_t result = 0;
  uint16_t i, j = 0;
  uint8_t test_eeprom_backup[CAT24C16_TEST_SIZE];
  uint8_t test_eeprom_wr[CAT24C16_TEST_SIZE];
  uint8_t test_eeprom_rd[CAT24C16_TEST_SIZE];

  result = CAT24C16_read(ctx, CAT24C16_TEST_OFFSET, CAT24C16_TEST_SIZE, test_eeprom_backup);
  if (result)
  {
    return EEPROM_TEST_FAIL_INITIAL_BACKUP;
  }

  while (j++ < 3)
  {
    srand((unsigned) ctx->sys_tick);
    for (i = 0; i < CAT24C16_TEST_SIZE; i++)
    {
      test_eeprom_wr[i] = rand();
    }
    memset(test_eeprom_rd, 0, CAT24C16_TEST_SIZE);
    result = CAT24C16_write(ctx, CAT24C16_TEST_OFFSET, CAT24C16_TEST_SIZE, test_eeprom_wr);
    if (result)
    {
      return EEPROM_TEST_FAIL_WRITING_BUF;
    }
    result = CAT24C16_read(ctx, CAT24C16_TEST_OFFSET, CAT24C16_TEST_SIZE, test_eeprom_rd);
    if (result)
    {
      return EEPROM_TEST_FAIL_READING_BUF;
    }

    if (memcmp(test_eeprom_wr, test_eeprom_rd, CAT24C16_TEST_SIZE))
    {
      return EEPROM_TEST_FAIL_BUF_COMPARISON;
    }
  }

  result = CAT24C16_write(ctx, CAT24C16_TEST_OFFSET, CAT24C16_TEST_SIZE, test_eeprom_backup);
  if (result)
  {
    return EEPROM_TEST_FAIL_RESTORING_BACKUP;
  }

  return EEPROM_TEST_PASS;
}
