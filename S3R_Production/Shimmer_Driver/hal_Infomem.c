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

#include "hal_Infomem.h"

gConfigBytes *infoMem_p_storedConfig;
uint8_t *infoMem_p_shimmerCalib_ram;

#if HAL_TEST_INFOMEM
uint8_t test_infomem_wr[INFOMEM_CONFIG_SIZE];
uint8_t test_infomem_rd[INFOMEM_CONFIG_SIZE];
#endif

#if defined(SHIMMER3R)
FLASH_EraseInitTypeDef pEraseInit = { .TypeErase = FLASH_TYPEERASE_PAGES,
  .Banks = INFOMEM_FLASH_BANK,
  .Page = INFOMEM_PAGE_OFFSET,
  .NbPages = INFOMEM_NUM_OF_PAGES };
#endif

void InfoMem_init(void)
{
  infoMem_p_storedConfig = S4Ram_getStoredConfig();
  infoMem_p_shimmerCalib_ram = ShimmerCalib_getRam();
}

//void InfoMem_initRam(uint8_t* buf){
//   infoMem_p_storedConfig = buf;
//}
//void InfoMem_initCalib(uint8_t* buf){
//   infoMem_p_shimmerCalib_ram = buf;
//}

void InfoMem_update()
{
  uint16_t j;
#if defined(SHIMMER3R)
  /* Disable instruction cache prior to internal cacheable memory update */
  if (HAL_ICACHE_Disable() != HAL_OK)
  {
    Error_Handler();
  }
#endif
  HAL_StatusTypeDef status = HAL_FLASH_Unlock();
  if (status != HAL_OK)
  {
    Error_Handler();
  }
#if defined(SHIMMER3R)
  uint32_t PageError;
  //TODO check PageError
  status |= HAL_FLASHEx_Erase(&pEraseInit, &PageError);
#elif defined(SHIMMER4_SDK)
  status |= FLASH_Erase_Sector(INFOMEM_SECTOR, FLASH_VOLTAGE_RANGE_3);
  status |= FLASH_WaitForLastOperation((uint32_t) 500);
#endif

  if (infoMem_p_storedConfig != 0)
  {
#if defined(SHIMMER3R)
    for (j = 0; j < INFOMEM_CONFIG_SIZE; j += QUAD_WORD_BYTE_SIZE)
    {
      status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, INFOMEM_CONFIG_OFFSET + j,
          (uint32_t) (infoMem_p_storedConfig->rawBytes + j));
    }
#elif defined(SHIMMER4_SDK)
    for (j = 0; j < INFOMEM_CONFIG_SIZE; j += 4)
    {
      //FLASH_TYPEPROGRAM_BYTE requires around 0x10000 clk cycles
      status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, INFOMEM_CONFIG_OFFSET + j,
          *(uint32_t *) (infoMem_p_storedConfig->rawBytes + j));
    }
//for(j = 0; j < INFOMEM_RAM_SIZE; j++){
//   status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, INFOMEM_RAM_OFFSET+j, infoMem_p_storedConfig[j]);
//}
#endif
  }
  if (infoMem_p_shimmerCalib_ram != 0)
  {
#if defined(SHIMMER3R)
    for (j = 0; j < INFOMEM_CALIB_SIZE; j += QUAD_WORD_BYTE_SIZE)
    {
      status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
          INFOMEM_CALIB_OFFSET + j, (uint32_t) (infoMem_p_shimmerCalib_ram + j));
    }
#elif defined(SHIMMER4_SDK)
    for (j = 0; j < INFOMEM_CALIB_SIZE; j += 4)
    {
      //FLASH_TYPEPROGRAM_BYTE requires around 0x10000 clk cycles
      status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, INFOMEM_CALIB_OFFSET + j,
          *(uint32_t *) (infoMem_p_shimmerCalib_ram + j));
    }
#endif
  }

#if HAL_TEST_INFOMEM
#if defined(SHIMMER3R)
  for (j = 0; j < INFOMEM_TEST_SIZE; j += QUAD_WORD_BYTE_SIZE)
  {
    status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
        INFOMEM_TEST_OFFSET + j, (uint32_t) (test_infomem_wr + j));
  }
#elif defined(SHIMMER4_SDK)
  for (j = 0; j < INFOMEM_TEST_SIZE; j += 4)
  {
    //FLASH_TYPEPROGRAM_BYTE requires around 0x10000 clk cycles
    status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, INFOMEM_TEST_OFFSET + j,
        *(uint32_t *) (test_infomem_wr + j));
  }
#endif
#endif

  status |= HAL_FLASH_Lock();
#if defined(SHIMMER3R)
  /* Enable instruction cache after the internal cacheable memory update */
  HAL_ICACHE_Enable();
#endif

  if (status != HAL_OK)
  {
    Error_Handler();
  }
}

void InfoMem_updateFrom(uint8_t *buf)
{
  uint16_t j;
  HAL_FLASH_Unlock();
#if defined(SHIMMER3R)
  uint32_t PageError;
  //TODO check PageEror
  HAL_FLASHEx_Erase(&pEraseInit, &PageError);
#elif defined(SHIMMER4_SDK)
  FLASH_Erase_Sector(INFOMEM_SECTOR, FLASH_VOLTAGE_RANGE_3);
  FLASH_WaitForLastOperation((uint32_t) 50000);
#endif

#if defined(SHIMMER3R)
  for (j = 0; j < INFOMEM_CONFIG_SIZE; j += QUAD_WORD_BYTE_SIZE)
  {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, INFOMEM_CONFIG_OFFSET + j,
        (uint32_t) (buf + j));
  }
#elif defined(SHIMMER4_SDK)
  for (j = 0; j < INFOMEM_CONFIG_SIZE; j++)
  {
    //FLASH_TYPEPROGRAM_BYTE requires around 0x10000 clk cycles
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, INFOMEM_CONFIG_OFFSET + j, buf[j]);
  }
#endif
  HAL_FLASH_Lock();
}

uint8_t InfoMem_readRam(uint8_t *buf, uint16_t addr, uint16_t size)
{
  if (addr + size > INFOMEM_CONFIG_SIZE)
    return 1;
  //reading 512 bytes takes 0x4ed clk cpu cycles
  memcpy(buf, (uint8_t *) INFOMEM_CONFIG_OFFSET + addr, size);

  return 0;
}

uint8_t InfoMem_readCalib(uint8_t *buf, uint16_t addr, uint16_t size)
{
  if (addr + size > INFOMEM_CALIB_SIZE)
    return 1;
  //reading 512 bytes takes 0x4ed clk cpu cycles
  memcpy(buf, (uint8_t *) INFOMEM_CALIB_OFFSET + addr, size);

  return 0;
}

#if HAL_TEST_INFOMEM
#include <stdlib.h>

uint8_t InfoMem_readTest(uint8_t *buf, uint16_t addr, uint16_t size)
{
  if (addr + size > INFOMEM_TEST_SIZE)
    return 1;
  //reading 512 bytes takes 0x4ed clk cpu cycles
  memcpy(buf, (uint8_t *) INFOMEM_TEST_OFFSET + addr, size);

  return 0;
}

//returns 0 if successful, 1 if failure
uint8_t InfoMem_test(void)
{
  uint16_t i, j = 0;
  uint8_t ret_val = 0;

  while (j++ < 3)
  {
    srand((unsigned) SysTick->VAL);
    for (i = 0; i < INFOMEM_TEST_SIZE; i++)
    {
      test_infomem_wr[i] = rand();
      test_infomem_rd[i] = 0;
    }
    InfoMem_update();
    InfoMem_readTest(test_infomem_rd, 0, INFOMEM_TEST_SIZE);

    ret_val = memcmp(test_infomem_wr, test_infomem_rd, INFOMEM_TEST_SIZE);
    if (ret_val)
    {
      __NOP();
      break;
    }
  }
  return ret_val;
}
#else
uint8_t InfoMem_test(void)
{
  //always return success
  return 0;
}
#endif //HAL_TEST_INFOMEM

uint8_t InfoMem_write(uint8_t addr, uint8_t *buf, uint16_t size)
{
  InfoMem_update();
}
