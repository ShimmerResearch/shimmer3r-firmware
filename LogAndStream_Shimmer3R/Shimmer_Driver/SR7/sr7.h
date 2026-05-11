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
 * @author Sam O'Mahony
 * @date August, 2016
 */
 
#ifndef SR7_H
#define SR7_H

#include <stdint.h>

// Packet overhead ((frame control field, sequence number, PAN ID, destination and source) + (footer))
// Note that the length uint8_t itself is not included included in the packet length
#define BASIC_RF_PACKET_OVERHEAD_SIZE   ((2 + 1 + 2 + 2 + 2) + (2))
#define BASIC_RF_MAX_PAYLOAD_SIZE      (127 - BASIC_RF_PACKET_OVERHEAD_SIZE)

// Frame control field
#define BASIC_RF_FCF_NOACK              0x8841
#define BASIC_RF_FCF_ACK                0x8861
#define BASIC_RF_FCF_ACK_BM             0x0020
#define BASIC_RF_FCF_BM                 (~BASIC_RF_FCF_ACK_BM)
#define BASIC_RF_ACK_FCF                0x0002

//-----------------------------------------------------------------------------------
// The data structure which is used to transmit packets
typedef struct {
   uint16_t destPanId;
   uint16_t destAddr;
   uint8_t length;
   uint8_t *pPayload;
   uint8_t ackRequest;
} BASIC_RF_TX_INFO;

// The receive struct:
typedef struct {
   uint8_t seqNumber;
   uint16_t srcAddr;
   uint16_t srcPanId;
   uint16_t destAddr;
   uint16_t destPanId;
   uint8_t length;
   uint8_t *pPayload;
   uint8_t ackRequest;
   int8_t rssi;
} BASIC_RF_RX_INFO;

// The RF settings structure:
typedef struct {
    BASIC_RF_RX_INFO *pRxInfo;
    uint8_t txSeqNumber;
    volatile uint8_t ackReceived;
    uint16_t panId;
    uint16_t myAddr;
    uint8_t receiveOn;
} BASIC_RF_SETTINGS;


void SR7_setChannel(uint8_t channel);

//onOff: 0 - de-activate SR7
//onOff: 1 - activate SR7
extern void SR7_wake(uint8_t onOff);

extern void SR7_init(SPI_HandleTypeDef *hspi, BASIC_RF_RX_INFO *pRRI, uint8_t channel, uint16_t panId, uint16_t myAddr);

extern void SR7_config(uint8_t channel, uint16_t panId, uint16_t myAddr);

extern uint8_t SR7_sendPacket(BASIC_RF_TX_INFO *pRTI);

extern uint8_t SR7_sendPacketNoAck(BASIC_RF_TX_INFO *pRTI);
extern void SR7_receiveOn(void);

//pass in a pointer to the function that will get called when
//data arrives
//the passed in function returns a 1 if program execution should resume
//(i.e. clear LPM3 bits)
//otherwise return 0
extern void SR7_receiveFunction(uint8_t (*receiveFuncPtr)(BASIC_RF_RX_INFO *pRRI));

//this function needs to be called from within the FIFOP ISR
//if 1 is returned expects program execution to resume (i.e. clear LMP3 bits)
extern uint8_t SR7_fifopInterrupt(void);

// check the states of these variables
extern uint8_t SR7_checkFifop(void);
   
extern uint8_t SR7_checkFifo(void);
   
extern uint8_t SR7_checkSFD(void);

#endif /* SR7_H */
