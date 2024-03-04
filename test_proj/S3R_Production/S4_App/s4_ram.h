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

//#pragma pack(2)

typedef __packed struct
		{

	      //cfg in common

		  uint8_t samplingRateLSB:1;
		  uint8_t samplingRateMSB:1;
		  uint8_t bufferSizeLSB:1;
		  uint8_t bufferSizeMSB:1;
		  uint8_t sensors0:1;
		  uint8_t sensors1:1;
		  uint8_t sensors2:1;
		  uint8_t configSetupByte0:1;
		  uint8_t configSetupByte1:1;
		  uint8_t configSetupByte2:1;
		  uint8_t configSetupByte3:1;
		  uint8_t exgADS1292R_1_CONFIG1:1;
		  uint8_t exgADS1292R_1_CONFIG2:1;
		  uint8_t exgADS1292R_1_LOFF:1;
		  uint8_t exgADS1292R_1_CH1SET:1;
		  uint8_t exgADS1292R_1_CH2SET:1;
		  uint8_t exgADS1292R_1_RLD_SENS:1;
		  uint8_t exgADS1292R_1_LOFF_SENS:1;
		  uint8_t exgADS1292R_1_LOFF_STAT:1;
		  uint8_t exgADS1292R_1_RESP1:1;
		  uint8_t exgADS1292R_1_RESP2:1;
		  uint8_t exgADS1292R_2_CONFIG1:1;
		  uint8_t exgADS1292R_2_CONFIG2:1;
		  uint8_t exgADS1292R_2_LOFF:1;
		  uint8_t exgADS1292R_2_CH1SET:1;
		  uint8_t exgADS1292R_2_CH2SET:1;
		  uint8_t exgADS1292R_2_RLD_SENS:1;
		  uint8_t exgADS1292R_2_LOFF_SENS:1;
		  uint8_t exgADS1292R_2_LOFF_STAT:1;
		  uint8_t exgADS1292R_2_RESP1:1;
		  uint8_t exgADS1292R_2_RESP2:1;
		  uint8_t btCommsBaudRate:1;
		  uint8_t NvDerivedChannels0:1;
		  uint8_t NvDerivedChannels1:1;
		  uint8_t NvDerivedChannels2:1;
		  uint64_t A_accelCalibration0:21;
		  uint64_t MPU9150GyroCalibration:21;
		  uint64_t LSM303DLHCMagCalibration:21;
		  uint64_t LSM303DLHCAccelCalibration:21;


		  //cfg in SDlog line InfoMem 118-122
		  uint8_t NvDerivedChannels3:1;
		  uint8_t NvDerivedChannels4:1;
		  uint8_t NvDerivedChannels5MSB:1;
		  uint8_t NvDerivedChannels6:1;
		  uint8_t NvDerivedChannels7:1;

		  //cfg for sd

		  uint8_t nVSensors3:1;
		  uint8_t nVSensors4:1;
		  uint8_t nVConfigSetupByte4:1;
		  uint8_t nVConfigSetupByte5:1;
		  uint8_t nVConfigSetupByte6:1;
		  uint64_t nVMPLAccelCalibration:21;
		  uint64_t nVMPLMagCalibration:21;
		  uint16_t nVMPLGyroCalibration:12;
		  uint16_t nVSDShimmerName:12;
		  uint16_t nVSDExPIDNAME:12;
		  uint8_t nVSDConfigTimeMSB:4;
		  uint8_t nVSDMyTrialID:1;
		  uint8_t nVSDNShimmer:1;
		  uint8_t nVSDTrialConfig0:1;
		  uint8_t nVSDTrialConfig1:1;
		  uint8_t nVSDBTInterval:1;
		  uint8_t nVESTEXPLENLSB:1;
		  uint8_t nVESTEXPLENMSB:1;
		  uint8_t nVMaxExPLenlSB:1;
		  uint8_t nVMaxExPLenMSB:1;
		  uint8_t macADDr:6;
		  uint8_t SD_CONFIG_DELAY_FLAG:1;

		  //cfg for sync

		  uint64_t SYNC_NODE_ADDR1:6;
		  uint64_t SYNC_NODE_ADDR2:6;
		  uint64_t SYNC_NODE_ADDR3:6;
		  uint64_t SYNC_NODE_ADDR4:6;
		  uint64_t SYNC_NODE_ADDR5:6;
		  uint64_t SYNC_NODE_ADDR6:6;
		  uint64_t SYNC_NODE_ADDR7:6;
		  uint64_t SYNC_NODE_ADDR8:6;
		  uint64_t SYNC_NODE_ADDR9:6;
		  uint64_t SYNC_NODE_ADDR10:6;
		  uint64_t SYNC_NODE_ADDR11:6;
		  uint64_t SYNC_NODE_ADDR12:6;
		  uint64_t SYNC_NODE_ADDR13:6;
		  uint64_t SYNC_NODE_ADDR14:6;
		  uint64_t SYNC_NODE_ADDR15:6;
		  uint64_t SYNC_NODE_ADDR16:6;
		  uint64_t SYNC_NODE_ADDR17:6;
		  uint64_t SYNC_NODE_ADDR18:6;
		  uint64_t SYNC_NODE_ADDR19:6;
		  uint64_t SYNC_NODE_ADDR20:6;
		  uint64_t SYNC_NODE_ADDR21:6;
		};



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
