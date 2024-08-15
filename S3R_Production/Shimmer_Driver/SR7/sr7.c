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

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "cc2420.h"
#include "sr7.h"
#include "string.h"
/* Includes ------------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
volatile BASIC_RF_SETTINGS rfSettings;
SPI_HandleTypeDef *hspi_sr7;
uint8_t (*SR7_dataAvailableFuncPtr)(BASIC_RF_RX_INFO *pRRI) = 0;
uint8_t SR7_sendingInProgress;

uint8_t txBuf, rxBuf, txAddr = CC2420_TXFIFO, rxAddr = CC2420_RXFIFO;
uint8_t txWordBuf[2], rxWordBuf[2];

//static volatile uint8_t fifop_is_1, fifo_is_1, SFD_is_1;
/* Private variables ---------------------------------------------------------*/


/* Definitions ---------------------------------------------------------------*/

#define BASIC_RF_ACK_PACKET_SIZE             5
#define BASIC_RF_LENGTH_MASK                 0x7F
#define BASIC_RF_CRC_OK_BM                   0x80

//#define FIFOP_IS_1                           (((uint8_t)HAL_GPIO_ReadPin(GPIOI, RADIO_FIFOP_Pin)) & 0x01)
//#define FIFO_IS_1                            (((uint8_t)HAL_GPIO_ReadPin(GPIOI, RADIO_FIFO_Pin)) & 0x20)
//#define SFD_IS_1                             (((uint8_t)HAL_GPIO_ReadPin(RADIO_SFD_GPIO_Port, RADIO_SFD_Pin)) & 0x04)


#define SPI_CSN(x)  HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, x?GPIO_PIN_SET:GPIO_PIN_RESET)

#define FASTSPI_TX(buf, size)                HAL_SPI_Transmit(hspi_sr7, buf, size, 5);
#define FASTSPI_RX(buf, size)                HAL_SPI_Receive(hspi_sr7, buf, size, 5);
#define FASTSPI_TX_RX(tx,rx,size)            HAL_SPI_TransmitReceive(hspi_sr7, tx, rx, size, 5);

#define FASTSPI_RX_WORD(x)                   FASTSPI_RX(x, 2);
#define FASTSPI_TX_ADDR(x)                   FASTSPI_TX(x, 1);

/***********************************************************
   FAST SPI: Radio register access
***********************************************************/
//      s = command strobe
//      a = register address
//      v = register value


#define FASTSPI_STROBE(s)                    \
   do{                                       \
      SPI_CSN(0);                            \
      FASTSPI_TX(s, 1);                      \
      SPI_CSN(1);                            \
   }while(0)

#define FASTSPI_RX_ADDR(a)                   \
   do{                                       \
      txBuf = (a | 0x40);                    \
      FASTSPI_TX(&txBuf, 1);                 \
   }while(0)

#define FASTSPI_RX_GARBAGE()                 \
   do {                                      \
      uint8_t *pGarbageBuf;                  \
      FASTSPI_RX(pGarbageBuf,1);             \
      /* do nothing with it */               \
   } while(0)

#define FASTSPI_SETREG(a,v)                  \
   do {                                      \
      SPI_CSN(0);                            \
      FASTSPI_TX_ADDR(a);                    \
      txWordBuf[0] = (uint8_t)(v >> 8);      \
      txWordBuf[1] = (uint8_t)(v);           \
      FASTSPI_TX(txWordBuf, 2);              \
      SPI_CSN(1);                            \
   } while (0)


#define FASTSPI_GETREG(a,v)                  \
   do {                                      \
      SPI_CSN(0);                            \
      FASTSPI_RX_ADDR(a);                    \
      /**v = *a;     */                          \
      FASTSPI_RX_WORD(v);                    \
      SPI_CSN(1);                            \
   } while (0)


// Updates the SPI status
#define FASTSPI_UPD_STATUS(s)                \
   do {                                      \
      SPI_CSN(0);                            \
      txBuf = CC2420_SNOP;                   \
      FASTSPI_TX_RX(&txBuf, &rxBuf, 1);      \
      s = rxBuf;                             \
      SPI_CSN(1);                            \
   } while (0)


#define FASTSPI_WRITE_FIFO(p,c)              \
   do {                                      \
      SPI_CSN(0);                            \
      FASTSPI_TX_ADDR(&txAddr);              \
      FASTSPI_TX(p, c);                      \
      SPI_CSN(1);                            \
   } while (0)


#define FASTSPI_READ_FIFO_UINT8(b)           \
   do {                                      \
      SPI_CSN(0);                            \
      FASTSPI_RX_ADDR(rxAddr);               \
      FASTSPI_RX(b, 1);                      \
      SPI_CSN(1);                            \
   } while (0)


#define FASTSPI_READ_FIFO_NO_WAIT(p,c)       \
   do {                                      \
      SPI_CSN(0);                            \
      FASTSPI_RX_ADDR(rxAddr);               \
      FASTSPI_RX(p, c);                      \
      SPI_CSN(1);                            \
   } while (0)


#define FASTSPI_READ_FIFO_GARBAGE(c)         \
   do {                                      \
      SPI_CSN(0);                            \
      FASTSPI_RX_ADDR(rxAddr);               \
      for (int i = 0; i < c; i++) {          \
         FASTSPI_RX_GARBAGE();               \
      }                                      \
      SPI_CSN(1);                            \
   } while (0)


#define FASTSPI_WRITE_RAM_LE(p,a,c)          \
   do {                                      \
      SPI_CSN(0);                            \
      txBuf = (0x80 | (a & 0x7F));           \
      FASTSPI_TX(&txBuf, 1);                 \
      txBuf = (a >> 1) & 0xC0;               \
      FASTSPI_TX(&txBuf, 1);                 \
      txWordBuf[0] = (uint8_t)(p);           \
      txWordBuf[1] = (uint8_t)(p >> 8);      \
      FASTSPI_TX(txWordBuf, 2);              \
      HAL_Delay(1);                          \
      SPI_CSN(1);                            \
   } while (0)

/* Definitions ---------------------------------------------------------------*/


/* Functions -----------------------------------------------------------------*/

void SR7_setChannel(uint8_t channel) {
   uint16_t f;

   // Derive frequency programming from the given channel number
   f = (uint16_t) (channel - 11);   // Subtract the base channel
   f = f + (f << 2);                // Multiply with 5, which is the channel spacing
   f = f + 357 + 0x4000;            // 357 is 2405-2048, 0x4000 is LOCK_THR = 1

   // Write it to the CC2420
   txBuf = CC2420_FSCTRL;
//   FASTSPI_SETREG(&txBuf, ((uint8_t*)f));

   FASTSPI_TX_ADDR(&txBuf);
   FASTSPI_TX((uint8_t*)(&f), 2);
}

void SR7_wake(uint8_t onOff) {
   //Power on/off SR7

   //P8OUT &= ~BIT3; - turns on/off VDD for the radio
   HAL_GPIO_WritePin(GPIOG, SW_RADIO_Pin, (onOff) ? GPIO_PIN_SET : GPIO_PIN_RESET);
   HAL_Delay(100);


   //P4OUT &= ~BIT5; - Radio VREG EN
   HAL_GPIO_WritePin(RADIO_VREG_EN_GPIO_Port, RADIO_VREG_EN_Pin, (onOff) ? GPIO_PIN_SET : GPIO_PIN_RESET);
   HAL_Delay(100);

   //P8OUT &= ~BIT2; - Radio RESET N
   HAL_GPIO_WritePin(GPIOI, RADIO_RESET_N_Pin, (onOff) ? GPIO_PIN_SET : GPIO_PIN_RESET);
   HAL_Delay(1);

}

void SR7_init(SPI_HandleTypeDef *hspi, BASIC_RF_RX_INFO *pRRI, uint8_t channel, uint16_t panId, uint16_t myAddr) {

   hspi_sr7 = hspi;

   //Power off - delay - power on
   SR7_wake(1);


   // --------------- Mark testing start -------------
   //Initialise the crystal oscillator
   txBuf = CC2420_SXOSCON; // 0x01
   FASTSPI_STROBE(&txBuf);

   //wait for the crystal oscillator to become stable
   uint8_t spiStatusByte = 0;
   //Poll the SPI status byte until the crystal oscillator is stable
   do {
      FASTSPI_UPD_STATUS(spiStatusByte);
   } while (!(spiStatusByte & 0x40));


   //SR7 register modifications - AAAAAAAAAAAAA
   txBuf = CC2420_IOCFG1; // 0x1d
   FASTSPI_SETREG(&txBuf, (CC2420_SFDMUX_XOSC16M_STABLE << CC2420_IOCFG1_CCAMUX)); // Turn on automatic packet acknowledgment

   //Set channel - BBBBBBBBBBBBBBB
   txBuf = CC2420_FSCTRL; // 0x18 = 24
   FASTSPI_SETREG(&txBuf, (( 1 << CC2420_FSCTRL_LOCK_THR ) |
                           ( ((uint16_t)( (channel - 11) * 5 + 357 )) << CC2420_FSCTRL_FREQ )));


   //writeMdmctrl10 - CCCCCCCCCCCCCCCCC
   txBuf = CC2420_MDMCTRL0; // 0x11
   FASTSPI_SETREG(&txBuf, (
                     ( 1 << CC2420_MDMCTRL0_RESERVED_FRAME_MODE ) |
                     ( 1 << CC2420_MDMCTRL0_ADR_DECODE ) |
                     ( 2 << CC2420_MDMCTRL0_CCA_HYST ) |
                     ( 3 << CC2420_MDMCTRL0_CCA_MOD ) |
                     ( 1 << CC2420_MDMCTRL0_AUTOCRC ) |
//       ( (autoAckEnabled && hwAutoAckDefault) << CC2420_MDMCTRL0_AUTOACK ) |
                     ( 0 << CC2420_MDMCTRL0_AUTOACK ) |
                     ( 2 << CC2420_MDMCTRL0_PREAMBLE_LENGTH ) ) );



   // DDDDDDDDDDDDDDDDDDDD
   txBuf = CC2420_RXCTRL1; // 0x17
   FASTSPI_SETREG(&txBuf, (
                     ( 1 << CC2420_RXCTRL1_RXBPF_LOCUR ) |
                     ( 1 << CC2420_RXCTRL1_LOW_LOWGAIN ) |
                     ( 1 << CC2420_RXCTRL1_HIGH_HGM ) |
                     ( 1 << CC2420_RXCTRL1_LNA_CAP_ARRAY ) |
                     ( 1 << CC2420_RXCTRL1_RXMIX_TAIL ) |
                     ( 1 << CC2420_RXCTRL1_RXMIX_VCM ) |
                     ( 2 << CC2420_RXCTRL1_RXMIX_CURRENT ) ) );
                     

   // EEEEEEEEEEEEEEEEEEEEE
   txBuf = CC2420_TXCTRL; // 0x15
   FASTSPI_SETREG(&txBuf, (
                     ( 3 << CC2420_TXCTRL_TXMIXBUF_CUR ) |
                     ( 3 << CC2420_TXCTRL_TXMIX_CURRENT ) |
                     ( 7 << CC2420_TXCTRL_PA_CURRENT ) |
                     ( 1 << CC2420_TXCTRL_RESERVED ) |
                     ( (CC2420_DEF_RFPOWER & 0x1F) << CC2420_TXCTRL_PA_LEVEL ) ) );
                     

   // --------------- Mark testing end -------------


   // FFFFFFFFFFFFFFFF
   txBuf = CC2420_MDMCTRL1; // 0x12
   FASTSPI_SETREG(&txBuf, 0x0500);     // Set the correlation threshold = 20
   txBuf = CC2420_IOCFG0; // 0x1c
   FASTSPI_SETREG(&txBuf, 0x007F);     // Set the FIFOP threshold to maximum
   txBuf = CC2420_SECCTRL0; // 0x19
   FASTSPI_SETREG(&txBuf, 0x01C4);     // Turn off "Security enable"
   

   // Set the protocol configuration
   rfSettings.pRxInfo = pRRI;
   rfSettings.panId = panId;
   rfSettings.myAddr = myAddr;
   rfSettings.txSeqNumber = 0;
   rfSettings.receiveOn = 0;


   // GGGGGGGGGGGGGG
   // Write the short address and the PAN ID to the CC2420 RAM (requires that the XOSC is on and stable)
   FASTSPI_WRITE_RAM_LE(myAddr, CC2420_RAM_SHORTADR, 2);
   FASTSPI_WRITE_RAM_LE(panId, CC2420_RAM_PANID, 2);

   SR7_dataAvailableFuncPtr = 0;
   SR7_sendingInProgress = 0;

}

void SR7_config(uint8_t channel, uint16_t panId, uint16_t myAddr) {
   rfSettings.panId = panId;
   rfSettings.myAddr = myAddr;

//   uint16_t gie = __get_SR_register() & GIE; //Store current GIE state
//   __disable_interrupt();                    //Make this operation automatic

   SR7_setChannel(channel);

   // Write the short address and the PAN ID to the CC2420 RAM (requires that the XOSC is on and stable)
   FASTSPI_WRITE_RAM_LE(myAddr, CC2420_RAM_SHORTADR, 2);
   FASTSPI_WRITE_RAM_LE(panId, CC2420_RAM_PANID, 2);

//   __bis_SR_register(gie);                   //Restore original GIE state

}

//automatic send operation takes 0.43ms
uint8_t SR7_sendPacket(BASIC_RF_TX_INFO *pRTI) {
   uint16_t frameControlField;
   uint8_t packetLength;
   uint8_t spiStatus;
   uint8_t success;

   if (!SR7_sendingInProgress) {
      SR7_sendingInProgress = 1;
      // Wait until the transceiver is idle
      //while (FIFOP_IS_1 || SFD_IS_1);
      while (SR7_checkFifop() || SR7_checkSFD()) {
         if ((SR7_checkFifop()) && (!(SR7_checkFifo()))) {
            txBuf = CC2420_SFLUSHRX;
            FASTSPI_STROBE(&txBuf);
            //FASTSPI_STROBE(CC2420_SFLUSHRX);
            SR7_sendingInProgress = 0;
            return 0;
         }
      }

//      uint16_t gie = __get_SR_register() & GIE; //Store current GIE state
//      __disable_interrupt();                    //Make this operation atomic

      // Flush the TX FIFO just in case...
      txBuf = CC2420_SFLUSHTX;
      FASTSPI_STROBE(&txBuf);

      // Turn on RX if necessary
      //TODO: this doesn't seem to work
      //i.e. if SR7_receiveOn() is not already called then transmission doesn't work????
      if (!rfSettings.receiveOn) {
         txBuf = CC2420_SRXON;
         FASTSPI_STROBE(&txBuf);
      }

      // Wait for the RSSI value to become valid
      do {
         FASTSPI_UPD_STATUS(spiStatus);
      } while (!(spiStatus & 0x02));

      packetLength = (uint8_t) (pRTI->length + BASIC_RF_PACKET_OVERHEAD_SIZE);
      frameControlField = pRTI->ackRequest ? BASIC_RF_FCF_ACK : BASIC_RF_FCF_NOACK;

      // FFFFFFFFFFFFFFFFF
      FASTSPI_WRITE_FIFO(&packetLength, 1);                         // Packet length


      // GGGGGGGGGGGGGGGGGGGGG
      FASTSPI_WRITE_FIFO((uint8_t*) &frameControlField, 2);         // Frame control field

      FASTSPI_WRITE_FIFO((uint8_t*) &rfSettings.txSeqNumber, 1);    // Sequence number
      FASTSPI_WRITE_FIFO((uint8_t*) &rfSettings.panId, 2);          // Dest. PAN ID
      FASTSPI_WRITE_FIFO((uint8_t*) &pRTI->destAddr, 2);            // Dest. address
      FASTSPI_WRITE_FIFO((uint8_t*) &rfSettings.myAddr, 2);         // Source address
      FASTSPI_WRITE_FIFO(pRTI->pPayload, pRTI->length);             // Payload

      // Wait for the transmission to begin before exiting (makes sure that this
      // function cannot be called a second time, and thereby cancelling the first
      // transmission (observe the FIFOP + SFD test above).


      //FASTSPI_STROBE(CC2420_STXONCCA);
      //TODO: need to put a timeout on this, and/or multiple tries, or use CC2420_STXON strobe
      //if channel is busy SFD pin never goes high

      txBuf = CC2420_STXON;
      FASTSPI_STROBE(&txBuf);

      while (!(SR7_checkSFD())); // this statement takes 12/32768s ~ 0.4ms

//      __bis_SR_register(gie);                   //Restore original GIE state

      // Wait for the acknowledge to be received, if any
      if (pRTI->ackRequest) {
         //TODO
         success = 0;
      } else {
         success = 1;
      }

      // Turn off the receiver if it should not continue to be enabled
      if (!rfSettings.receiveOn) {
//         gie = __get_SR_register() & GIE;    //Store current GIE state
//         __disable_interrupt();              //Make this operation atomic

         txBuf = CC2420_SRFOFF;
         FASTSPI_STROBE(&txBuf);

//         __bis_SR_register(gie);             //Restore original GIE state
      }
      // Increment the sequence number, and return the result
      rfSettings.txSeqNumber++;

      SR7_sendingInProgress = 0;
      return success;
   }
   else {
      return 0;
   }
}

uint8_t SR7_sendPacketNoAck(BASIC_RF_TX_INFO *pRTI) {
   uint16_t frameControlField;
   uint8_t packetLength;
   uint8_t spiStatus;
//   uint8_t success;

   if (!SR7_sendingInProgress) {
      SR7_sendingInProgress = 1;
      // Wait until the transceiver is idle
      while (SR7_checkFifop() || SR7_checkSFD()) {
         if ((SR7_checkFifop()) && (!(SR7_checkFifo()))) {
            txBuf = CC2420_SFLUSHRX;
            FASTSPI_STROBE(&txBuf);
            FASTSPI_STROBE(&txBuf);
            SR7_sendingInProgress = 0;
            return 0;
         }
      }

//      uint16_t gie = __get_SR_register() & GIE; //Store current GIE state
//      __disable_interrupt();                    //Make this operation atomic

      // Flush the TX FIFO just in case...
      txBuf = CC2420_SFLUSHTX;
      FASTSPI_STROBE(&txBuf);

      // Turn on RX if necessary
      //TODO: this doesn't seem to work
      //i.e. if SR7_receiveOn() is not already called then transmission doesn't work????
      if (!rfSettings.receiveOn) {
         txBuf = CC2420_SRXON;
         FASTSPI_STROBE(&txBuf);
      }

      // Wait for the RSSI value to become valid
      do {
         FASTSPI_UPD_STATUS(spiStatus);
      } while (!(spiStatus & 0x02));

      packetLength = pRTI->length + BASIC_RF_PACKET_OVERHEAD_SIZE;
      frameControlField = pRTI->ackRequest ? BASIC_RF_FCF_ACK : BASIC_RF_FCF_NOACK; 
            

      FASTSPI_WRITE_FIFO(&packetLength, 1);                         // Packet length
      FASTSPI_WRITE_FIFO((uint8_t*) &frameControlField, 2);         // Frame control field
      FASTSPI_WRITE_FIFO((uint8_t*) &rfSettings.txSeqNumber, 1);    // Sequence number
      FASTSPI_WRITE_FIFO((uint8_t*) &rfSettings.panId, 2);          // Dest. PAN ID
      FASTSPI_WRITE_FIFO((uint8_t*) &pRTI->destAddr, 2);            // Dest. address
      FASTSPI_WRITE_FIFO((uint8_t*) &rfSettings.myAddr, 2);         // Source address     
      FASTSPI_WRITE_FIFO(pRTI->pPayload, pRTI->length);             // Payload

      // Wait for the transmission to begin before exiting (makes sure that this
      // function cannot be called a second time, and thereby cancelling the first
      // transmission (observe the FIFOP + SFD test above).


      //FASTSPI_STROBE(CC2420_STXONCCA);
      //TODO: need to put a timeout on this, and/or multiple tries, or use CC2420_STXON strobe
      //if channel is busy SFD pin never goes high


      txBuf = CC2420_STXON;
      FASTSPI_STROBE(&txBuf);      

      // Increment the sequence number, and return the result
      rfSettings.txSeqNumber++;

      SR7_sendingInProgress = 0;
   }
   return 0;
}

void SR7_receiveOn(void) {

   rfSettings.receiveOn = 1;
   txBuf = CC2420_SRXON; // 0x03
   FASTSPI_STROBE(&txBuf);
   txBuf = CC2420_SFLUSHRX; // 0x08
   FASTSPI_STROBE(&txBuf);

}


void SR7_receiveFunction(uint8_t (*receiveFuncPtr)(BASIC_RF_RX_INFO *pRRI)) {
   SR7_dataAvailableFuncPtr = receiveFuncPtr;
}

uint8_t SR7_fifopInterrupt(void) {
   uint16_t frameControlField;
   uint8_t length;
   uint8_t pFooter[2];


   // Clean up and exit in case of FIFO overflow, which is indicated by FIFOP = 1 and FIFO = 0
   if ((SR7_checkFifop()) && (!(SR7_checkFifo()))) {
      txBuf = CC2420_SFLUSHRX;
      FASTSPI_STROBE(&txBuf);
      FASTSPI_STROBE(&txBuf);
      return 0;
   }

   
   // Payload length
   FASTSPI_READ_FIFO_UINT8(&rxBuf);
   length = rxBuf;

   length &= BASIC_RF_LENGTH_MASK; // Ignore MSB

   // Ignore the packet if the length is too short
   if (length < BASIC_RF_ACK_PACKET_SIZE) {
      FASTSPI_READ_FIFO_GARBAGE(length);

      // Otherwise, if the length is valid, then proceed with the rest of the packet
   } else {
      // Register the payload length
      rfSettings.pRxInfo->length = length - BASIC_RF_PACKET_OVERHEAD_SIZE;

      // Read the frame control field and the data sequence number
      FASTSPI_READ_FIFO_NO_WAIT((uint8_t*) &frameControlField, 2);
      rfSettings.pRxInfo->ackRequest = !!(frameControlField & BASIC_RF_FCF_ACK_BM);

      FASTSPI_READ_FIFO_UINT8(&rxBuf);
      rfSettings.pRxInfo->seqNumber = rxBuf;

      // Is this an acknowledgment packet?
      if ((length == BASIC_RF_ACK_PACKET_SIZE) && (frameControlField == BASIC_RF_ACK_FCF)
            && (rfSettings.pRxInfo->seqNumber == rfSettings.txSeqNumber)) {

         // Read the footer and check for CRC OK
         FASTSPI_READ_FIFO_NO_WAIT((uint8_t*) pFooter, 2);

         // Indicate the successful ack reception (this flag is polled by the transmission routine)
         if (pFooter[1] & BASIC_RF_CRC_OK_BM) rfSettings.ackReceived = 1;

         // Too small to be a valid packet?
      } else if (length < BASIC_RF_PACKET_OVERHEAD_SIZE) {
         FASTSPI_READ_FIFO_GARBAGE(length - 3);
         return 0;

         // Receive the rest of the packet
      } else {
         // Skip the destination PAN and address (that's taken care of by harware address recognition!)
         //FASTSPI_READ_FIFO_GARBAGE(4);

         // Still read the destination PAN and address
         FASTSPI_READ_FIFO_NO_WAIT((uint8_t*) &rfSettings.pRxInfo->destPanId, 2);
         FASTSPI_READ_FIFO_NO_WAIT((uint8_t*) &rfSettings.pRxInfo->destAddr, 2);

         // Read the source address
         FASTSPI_READ_FIFO_NO_WAIT((uint8_t*) &rfSettings.pRxInfo->srcAddr, 2);

         // Read the packet payload
         FASTSPI_READ_FIFO_NO_WAIT(rfSettings.pRxInfo->pPayload, rfSettings.pRxInfo->length);

         // Read the footer to get the RSSI value
         FASTSPI_READ_FIFO_NO_WAIT((uint8_t*) pFooter, 2);
         rfSettings.pRxInfo->rssi = pFooter[0];

         if (SR7_dataAvailableFuncPtr) {      // ensure this has been set
            // Notify the application about the received _data_ packet if the CRC is OK
            if (((frameControlField & (BASIC_RF_FCF_BM)) == BASIC_RF_FCF_NOACK) && (pFooter[1] & BASIC_RF_CRC_OK_BM)) {
               //rfSettings.pRxInfo = basicRfReceivePacket(rfSettings.pRxInfo);
               return (*SR7_dataAvailableFuncPtr)(rfSettings.pRxInfo);
            }
         }
      }
   }
   return 0;
}


uint8_t  SR7_checkFifop() {
   return ((uint8_t)HAL_GPIO_ReadPin(GPIOI, RADIO_FIFOP_Pin));
}
uint8_t SR7_checkFifo() {
   return ((uint8_t)HAL_GPIO_ReadPin(GPIOI, RADIO_FIFO_Pin));
}
uint8_t SR7_checkSFD() {
   return ((uint8_t)HAL_GPIO_ReadPin(RADIO_SFD_GPIO_Port, RADIO_SFD_Pin));
}

/* Functions -----------------------------------------------------------------*/
