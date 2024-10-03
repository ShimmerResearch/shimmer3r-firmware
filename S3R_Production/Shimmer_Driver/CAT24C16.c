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
#include "hal_Board.h"
#include <shimmer_include.h>

I2C_HandleTypeDef *eeprom_hi2c;
HAL_StatusTypeDef cat24c16_result;
#if !IS_CONNECTED_EEPROM
uint8_t eepromContents[512] = { 0 };
#endif

void CAT24C16_init(I2C_HandleTypeDef *hi2c)
{
  eeprom_hi2c = hi2c;
}

void CAT24C16_powerOn(void)
{
  Board_enableSensingPower(1);
  MX_I2C1_Init();
}

void CAT24C16_powerOff(void)
{
  HAL_Delay(5); //5ms to ensure no writes pending
  I2C1_DeInit();
  Board_enableSensingPower(0);
}

void CAT24C16_read(uint16_t address, uint16_t length, uint8_t *outBuffer)
{
  if ((!length) || (length > 2048) || (address + length > 2048))
    return;
  //I2C_Set_Slave_Address(CAT24C16_ADDR|(address>>8));
  uint8_t addr_hi = CAT24C16_ADDR | (address >> 8);
  uint8_t addr_lo = address & 0xff;

  cat24c16_result = HAL_I2C_Master_Transmit(eeprom_hi2c, addr_hi << 1, &addr_lo, 1, 1000);
  cat24c16_result
      = HAL_I2C_Master_Receive(eeprom_hi2c, addr_hi << 1, outBuffer, length, 1000);
}

void CAT24C16_write(uint16_t address, uint16_t length, uint8_t *data)
{
  if ((!length) || (length > 2048) || (address + length > 2048))
    return;

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
    cat24c16_result = HAL_I2C_Master_Transmit(
        eeprom_hi2c, addr_hi << 1, buf, this_write + 1, 1000);
    HAL_Delay(5);

    __NOP();

    mem_ptr += this_write;
    buf_offset += this_write;

    //if reaches edge of flash mem, reassign the slave address
    if (!(mem_ptr % CAT24C16_BLOCK_SIZE))
      inc_addr = 1;
  }
}

//#if HAL_TEST_EEPROM
#include <stdlib.h>

//returns 0 if successful, 1 if failure
uint8_t CAT24C16_test(void)
{
  uint16_t i, j = 0;
  uint8_t ret_val = 0;
  uint8_t test_eeprom_backup[CAT24C16_TEST_SIZE];
  uint8_t test_eeprom_wr[CAT24C16_TEST_SIZE];
  uint8_t test_eeprom_rd[CAT24C16_TEST_SIZE];

  CAT24C16_read(CAT24C16_TEST_OFFSET, CAT24C16_TEST_SIZE, test_eeprom_backup);

  while (j++ < 3)
  {
    srand((unsigned) SysTick->VAL);
    for (i = 0; i < CAT24C16_TEST_SIZE; i++)
    {
      test_eeprom_wr[i] = rand();
    }
    memset(test_eeprom_rd, 0, CAT24C16_TEST_SIZE);
    CAT24C16_write(CAT24C16_TEST_OFFSET, CAT24C16_TEST_SIZE, test_eeprom_wr);
    CAT24C16_read(CAT24C16_TEST_OFFSET, CAT24C16_TEST_SIZE, test_eeprom_rd);

    ret_val = memcmp(test_eeprom_wr, test_eeprom_rd, CAT24C16_TEST_SIZE);
    if (ret_val)
    {
      break;
    }
  }

  CAT24C16_write(CAT24C16_TEST_OFFSET, CAT24C16_TEST_SIZE, test_eeprom_backup);

  return ret_val;
}

//#else
//uint8_t CAT24C16_test(void){
//   // always return success
//   return 0;
//}
//#endif //HAL_TEST_EEPROM

void eepromRead(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf)
{
  eepromReadWrite(dataAddr, dataSize, dataBuf, EEPROM_READ);
}

void eepromWrite(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf)
{
  eepromReadWrite(dataAddr, dataSize, dataBuf, EEPROM_WRITE);
}

void eepromReadWrite(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf, enum EEPROM_RW eepromRW)
{
#if IS_CONNECTED_EEPROM
  CAT24C16_powerOn();

  //EEPROM needs to be updated with latest bt baud rate, configure here
  if (eepromRW == EEPROM_READ)
  {
    CAT24C16_read(dataAddr, dataSize, dataBuf);
  }
  else
  {
    CAT24C16_write(dataAddr, dataSize, dataBuf);
  }

  //Wind down EEPROM and required timing peripherals
  CAT24C16_powerOff();
#else
  if (eepromRW == EEPROM_READ)
  {
    memcpy(dataBuf, &eepromContents[dataAddr], dataSize);
  }
  else
  {
    memcpy(&eepromContents[dataAddr], dataBuf, dataSize);
  }
#endif
}

#if !IS_CONNECTED_EEPROM
void setMockExpansionBrdDetails(void)
{
  eepromContents[0] = EXP_BRD_PROTO3_DELUXE; //0xFF
  eepromContents[1] = 9;                     //0xFF
  eepromContents[2] = 0;                     //0xFF
}
#endif
