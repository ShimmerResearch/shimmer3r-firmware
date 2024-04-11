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
/*
Infomem: using flash memory as the InfoMem (EEPROM) of Shimmer4
must do mass erase before each write, thus need to update the whole flash 
sector whenever any byte needs to be changed.
Normally leave FLASH_SECTOR_0 (start from 0x8000000) for boot straps and 
use FLASH_SECTOR_1 as InfoMem, main() starts from FLASH_SECTOR_2.

1MByte flash for stm32f746IGK6 : 8 sectors:
FLASH_SECTOR_0    0x008000    32kB
FLASH_SECTOR_1    0x008000    32kB
FLASH_SECTOR_2    0x008000    32kB
FLASH_SECTOR_3    0x008000    32kB
FLASH_SECTOR_4    0x010000    128kB
FLASH_SECTOR_5    0x020000    256kB
FLASH_SECTOR_6    0x020000    256kB
FLASH_SECTOR_7    0x020000    256kB
To use FLASH_SECTOR_1 as InfoMem (EEPROM) for example, 
need to setup Flash as InfoMem in Keil:
Options for Target-> Target tab -> ROM area: 
default  on-chip start       size     startup
yes      IROM1    0x8000000   0x08000   yes
yes      IROM1    0x8010000   0xf0000   

or

1MByte flash for stm32f746NGH6U : 12 sectors:
name              size        size
FLASH_SECTOR_0    0x004000    16kB
FLASH_SECTOR_1    0x004000    16kB
FLASH_SECTOR_2    0x004000    16kB
FLASH_SECTOR_3    0x004000    16kB
FLASH_SECTOR_4    0x010000    64kB
FLASH_SECTOR_5    0x020000    128kB
FLASH_SECTOR_6    0x020000    128kB
FLASH_SECTOR_7    0x020000    128kB
FLASH_SECTOR_8    0x020000    128kB
FLASH_SECTOR_9    0x020000    128kB
FLASH_SECTOR_10   0x020000    128kB
FLASH_SECTOR_11   0x020000    128kB
To use FLASH_SECTOR_1 as InfoMem (EEPROM) for example, 
need to setup Flash as InfoMem in Keil:
Options for Target-> Target tab -> ROM area: 
default  on-chip start       size     startup
yes      IROM1    0x8000000   0x04000   yes
yes      IROM1    0x8008000   0xf8000   

*/

#include<stdint.h>
#include <string.h>
#include "stm32u5xx_hal.h"
#include "../S4_App/s4__cfg.h"

#ifndef HAL_INFOMEM_H
#define HAL_INFOMEM_H

#define INFOMEM_SECTOR           FLASH_SECTOR_1
#define INFOMEM_OFFSET           0x08008000
#define INFOMEM_SIZE_MAX         0x8000

#define INFOMEM_MASK_RAM         0x0001
#define INFOMEM_MASK_CALIB       0x0002
#define INFOMEM_MASK_ALL         0xFFFF


#define INFOMEM_CONFIG_OFFSET    INFOMEM_OFFSET
#define INFOMEM_CONFIG_SIZE      0x200
#define INFOMEM_CALIB_OFFSET     INFOMEM_CONFIG_OFFSET + INFOMEM_CONFIG_SIZE
#define INFOMEM_CALIB_SIZE       0x400
#define INFOMEM_TEST_OFFSET      INFOMEM_CALIB_OFFSET + INFOMEM_CALIB_SIZE
#define INFOMEM_TEST_SIZE        0x200

#define INFOMEM_SIZE_TOTAL       INFOMEM_CONFIG_SIZE + INFOMEM_CALIB_SIZE + INFOMEM_TEST_SIZE


//returns 0 if successful, 1 if failure

void InfoMem_init(void);
   
//void InfoMem_initRam(uint8_t* buf);

//void InfoMem_initCalib(uint8_t* buf);
   

void InfoMem_update(void);

void InfoMem_updateFrom(uint8_t * buf) ;

uint8_t InfoMem_readRam(uint8_t *buf, uint16_t addr, uint16_t size);

uint8_t InfoMem_readCalib(uint8_t *buf, uint16_t addr, uint16_t size);


//returns 0 if successful, 1 if failure
extern uint8_t InfoMem_test(void);

#endif
