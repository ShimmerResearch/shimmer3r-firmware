/*******************************************************************************
 * Project Name      : EZSerial_Host_Demo
 * File Name         : handlers.c
 * Version           : 1.1.1
 * Device Used       : CY8C4245AXI-483
 * Software Used     : PSoC Creator 4.1 build 2686
 * Compiler          : ARM GCC 5.4-2016-q2-update
 * Related Hardware  : CY8CKIT-042 PSoC 4 Pioneer Kit
 *                   : CY8CKIT-042-BLE Bluetooth Low Energy Pioneer Kit
 *                   : CYBLE-212019-00 EZ-BLE module
 *                   : CYBLE-212019-EVAL module
 * Owner             : JROW
 *
 ********************************************************************************
 * Copyright 2017, Cypress Semiconductor Corporation. All Rights Reserved.
 ********************************************************************************
 * This software is owned by Cypress Semiconductor Corporation (Cypress)
 * and is protected by and subject to worldwide patent protection (United
 * States and foreign), United States copyright laws and international treaty
 * provisions. Cypress hereby grants to licensee a personal, non-exclusive,
 * non-transferable license to copy, use, modify, create derivative works of,
 * and compile the Cypress Source Code and derivative works for the sole
 * purpose of creating custom software in support of licensee product to be
 * used only in conjunction with a Cypress integrated circuit as specified in
 * the applicable agreement. Any reproduction, modification, translation,
 * compilation, or representation of this software except as specified above
 * is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH
 * REGARD TO THIS MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * Cypress reserves the right to make changes without further notice to the
 * materials described herein. Cypress does not assume any liability arising out
 * of the application or use of any product or circuit described herein. Cypress
 * does not authorize its products for use as critical components in
 *life-support systems where a malfunction or failure may reasonably be expected
 *to result in significant injury to the user. The inclusion of Cypress' product
 *in a life- support systems application implies that the manufacturer assumes
 *all risk of such use and in doing so indemnifies Cypress against all charges.
 *
 * Use of this Software may be limited by and subject to the applicable Cypress
 * software license agreement.
 *******************************************************************************/

#include "stm32u5xx.h"

#include "../../S4_App/s4.h"
#include "../EZ-Serial/handlers.h"
#include "../shimmer_bt_comms.h"

uint8_t pending_response = 0;
//uint8_t timer_active = 0;
//volatile uint16_t timeout_ms_elapsed;

UART_HandleTypeDef *huartBtPtr;

uint8_t *inBytePtr;

uint8_t bt_txBuf[512];
uint8_t rxBuf[512];
uint16_t expectedByteCount;

/* Buffer read / write macros                                                 */
#define RINGFIFO_RESET(ringFifo)         \
  {                                      \
    ringFifo.rdIdx = ringFifo.wrIdx = 0; \
  }
#define RINGFIFO_WR(ringFifo, dataIn, mask)            \
  {                                                    \
    ringFifo.data[mask & ringFifo.wrIdx++] = (dataIn); \
  }
#define RINGFIFO_RD(ringFifo, dataOut, mask)              \
  {                                                       \
    ringFifo.rdIdx++;                                     \
    dataOut = ringFifo.data[mask & (ringFifo.rdIdx - 1)]; \
  }
#define RINGFIFO_EMPTY(ringFifo) (ringFifo.rdIdx == ringFifo.wrIdx)
#define RINGFIFO_FULL(ringFifo, mask) \
  ((mask & ringFifo.rdIdx) == (mask & (ringFifo.wrIdx + 1)))
#define RINGFIFO_COUNT(ringFifo, mask) \
  (mask & (ringFifo.wrIdx - ringFifo.rdIdx))

volatile RingFifoTx_t gBtTxFifo;

volatile uint8_t messageInProgress;

uint8_t dataRateTestState;
uint8_t dataRateTestTxPacket[] = { DATA_RATE_TEST_RESPONSE, 0, 0, 0, 0 };

/*******************************************************************************
 * Interrupt Handler Name: TimerInterruptHandler
 ****************************************************************************//**
* Manages the 1 kHz timer that runs while waiting for input data to be received.
*******************************************************************************/
//CY_ISR(TimerInterruptHandler)
//{
//    /* increment protocol timeout 1kHz counter */
//    timeout_ms_elapsed++;
//}

void appHandler(ezs_packet_t *packet)
{
  if (packet->packet_type == EZS_PACKET_TYPE_RESPONSE)
  {
    /* clear pending response flag */
    if (pending_response != 0)
    {
      pending_response--;
    }
  }

  /* send packet to app-level callback, if defined */
  if (ezsHandler)
  {
    /* NOTE: packet-specific application handler code could be placed right
     * inside this function (appHandler), but this method allows cleaner
     * separation between each block of functionality:
     *  1. EZ-Serial API protocol (ezsapi.c/.h)
     *  2. Platform-specific I/O handlers (handlers.c/.h)
     *  3. Application logic (main.c)
     */
    ezsHandler(packet);
  }
}

ezs_output_result_t appOutput(uint16_t length, const uint8_t *data)
{
  /* make sure we aren't already waiting for a response */
  if (pending_response != 0)
  {
    /* only one pending response at a time is allowed */
    return EZS_OUTPUT_RESULT_RESPONSE_PENDING;
  }

  /* increment pending response counter */
  pending_response++;

  /* send data out through UART */
  //UART_SpiUartPutArray((uint8_t *)data, length);
  HAL_StatusTypeDef ret_val;

  //ret_val = HAL_UART_Transmit_DMA(huart, (uint8_t *)data, length);
  //ret_val = HAL_UART_Transmit(huart, (uint8_t *)data, length, 1500*HAL_GetTickFreq());
  ret_val = HAL_UART_Transmit_IT(huartBtPtr, (uint8_t *) data, length);

  if (ret_val != HAL_OK)
  {
    SHIMMER_PRINTF("DMA problem in appOutput\r\n");
  }

  return EZS_OUTPUT_RESULT_DATA_WRITTEN;
}

//ezs_input_result_t appInput(uint8_t *inByte, uint16_t timeout) {
//    /* initialize timestamp for timeout detection if necessary */
//    if (timeout != 0 && timeout != 0xFFFF && timer_active == 0)
//    {
//        timeout_ms_elapsed = 0;
//        timer_active = 1;
//        TIMER_Start();
//    }
//
//    /* attempt to read a byte from UART */
//    if (UART_SpiUartGetRxBufferSize() > 0)
//    {
//        /* data available */
//        *inByte = UART_SpiUartReadRxData();
//        return EZS_INPUT_RESULT_BYTE_READ;
//    }
//    else if (pending_response != 0 && timeout != 0 && timeout_ms_elapsed > timeout)
//    {
//        /* no data available, timeout condition */
//        TIMER_Stop();
//        timeout_ms_elapsed = 0;
//        timer_active = 0;
//
//        /* clear pending response since one is no longer expected */
//        pending_response = 0;
//
//        return EZS_INPUT_RESULT_TIMEOUT;
//    }
//
//    return EZS_INPUT_RESULT_NO_DATA;
//}

ezs_input_result_t appInput(uint8_t *inByte, uint16_t timeout)
{

  inBytePtr = inByte;

  /* attempt to read a byte from UART */
  //HAL_StatusTypeDef status = HAL_UART_Receive(huart, inByte, 1, timeout);

  //setDmaRx(1);

  //return EZS_INPUT_RESULT_NO_DATA;
  return EZS_INPUT_RESULT_BYTE_READ;
}

HAL_StatusTypeDef setBtRxDmaWaitingForResponse(uint16_t length)
{
  expectedByteCount = length;
  //HAL_StatusTypeDef status = HAL_UART_AbortReceive(huart);

  HAL_StatusTypeDef status = HAL_UART_Receive_DMA(huartBtPtr, &rxBuf[0], expectedByteCount);

  //SHIMMER_PRINTF("%d\r\n", length);

  if (status != HAL_OK)
  {
    SHIMMER_PRINTF("setDmaWaitingForResponse fault\r\n");
  }

  return status;
}

void setBtUartInstance(UART_HandleTypeDef *huartToUse)
{
  huartBtPtr = huartToUse;

  HAL_UART_RegisterCallback(huartBtPtr, HAL_UART_RX_COMPLETE_CB_ID, btUartDmaRxCpltCallback);
  HAL_UART_RegisterCallback(huartBtPtr, HAL_UART_TX_COMPLETE_CB_ID, btUartTxCpltCallback);
  //HAL_DMA_RegisterCallback(huart->, HAL_DMA_XFER_CPLT_CB_ID, btUartDmaRxCpltCallback);
}

void btUartDmaRxCpltCallback(UART_HandleTypeDef *huart)
{
  //SHIMMER_PRINTF("byte received\r\n");
  //SHIMMER_PRINTF("%c", rxBuf[0]);

  //if start byte is CYW header byte or if in middle of waiting for full CYW response
  //if(!waitingForArgs
  //    && (ezs_rx_packet_length != 0 || (b & EZS_BINARY_SOF_MASK) != 0)) {
  //
  //  ezs_input_result_t result = EZSerial_Parse(b);
  //  if(result == EZS_INPUT_RESULT_IN_PROGRESS
  //      || result == EZS_INPUT_RESULT_PACKET_COMPLETE) {
  //
  //  }
  //} else {
  //  //Assume Shimmer command
  //}

  //ezs_packet_t *result = ezs_parseSingleByte(rxBuf[0]);
  //if(result!=0){
  //  ezsHandlerShimmer(result);
  //}
  //HAL_StatusTypeDef status = setDmaRx(1);


  //for (uint8_t i = 0; i < expectedByteCount; i++)
  //{
  //  ezs_packet_t *result = ezs_parseSingleByte(rxBuf[i]);
  //  if (result != 0)
  //  {
  //    ezsHandlerShimmer(result);
  //  }
  //}
  //
  //uint16_t count = getEzsRemainingByteCount();
  //if (count == 0)
  //{
  //  count = 1;
  //}
  //HAL_StatusTypeDef status = setDmaRx(count);


  uint16_t count = 1;

  uint8_t i = 0;
  while (i < expectedByteCount)
  {
    /* If were waiting for the rest of a Shimmer packet or the the EZ Serial
     * parse is ideal and the header byte is a Shimmer packet header byte,
     * parse as Shimmer packet */
    if (isWaitingForArgs()
        || (getEzsPacketLength() == 0 && rxBuf[i] != EZS_BINARY_TYPE_CMDRSP
            && rxBuf[i] != (EZS_BINARY_TYPE_CMDRSP | EZS_COMMAND_SCOPE_FLASH)
            && rxBuf[i] != EZS_BINARY_TYPE_EVENT))
    {
      //Parse as Shimmer packet
      SHIMMER_PRINTF("S1=0x%x\n", rxBuf[i]);
      count = getBtRxShimmerCommsWaitByteCount();
      Dma2ConversionDone(&rxBuf[i]);
      i += count;
      count = getBtRxShimmerCommsWaitByteCount();
    }
    else
    {
      ezs_packet_t *result = ezs_parseSingleByte(rxBuf[i]);
      if (result != 0)
      {
        /* If complete EZ Serial packet parsed, send to handler */
        ezsHandlerShimmer(result);
      }
      else
      {
        ezs_input_result_t result = getLastEzsByteParseResult();

        if (result == EZS_INPUT_RESULT_IN_PROGRESS)
        {
          //if (getEzsPacketLength() != 0)
          //{
          //  count = getEzsRemainingByteCount();
          //}
          count = getEzsRemainingByteCount();
        }

        //TODO get working
        else if (result == EZS_INPUT_RESULT_BUFFER_OVERFLOW || result == EZS_INPUT_RESULT_UNHANDLED_PACKET
            || result == EZS_INPUT_RESULT_INVALID_CHECKSUM)
        {
          /* If packet incomplete but byte wasn't recognised as part of an EZ
           * Serial packet, send to Shimmer parser */
          if (getEzsPacketLength() == 0)
          {
            SHIMMER_PRINTF("S2=0x%x\n", rxBuf[i]);
          }
        }
      }
      i += 1;
    }
  }

  if (count == 0)
  {
    count = 1;
  }
  HAL_StatusTypeDef status = setBtRxDmaWaitingForResponse(count);
}

void btUartTxCpltCallback(UART_HandleTypeDef *huart)
{
  if (isBtConnected())
  {
    if (dataRateTestState)
    {
      loadBtTxBufForDataRateTest();
    }
    else
    {
      sendNextChar();
    }
  }
  else
  {
    clearBtTxBuf(0);
  }
}

void sendNextCharIfNotInProgress(void)
{
  if (!messageInProgress)
  {
    sendNextChar();
  }
}

void sendNextChar(void)
{
  if (!RINGFIFO_EMPTY(gBtTxFifo)
      //#if BT_FLUSH_TX_BUF_IF_RN4678_RTS_LOCK_DETECTED
      //            && (rn4678RtsLockDetected || !isBtModuleOverflowPinHigh())
      //#else
      //            && !isBtModuleOverflowPinHigh())
      //#endif
  )
  {
    messageInProgress = 1;
    /* commenting out while loop as individual bytes are sent based on
     * interrupt firing so no need to wait here. */
    //ensure no tx interrupt is pending
    //while (UCA1IFG & UCTXIFG);
    //uint8_t bt_txBuf;

    //RINGFIFO_RD(gBtTxFifo, bt_txBuf[0], BT_TX_BUF_MASK);
    //HAL_StatusTypeDef ret_val = HAL_UART_Transmit_IT(huart, &bt_txBuf[0], 1);

    HAL_StatusTypeDef ret_val;
    uint8_t numBytes;

    uint8_t rdIdx = (gBtTxFifo.rdIdx & BT_TX_BUF_MASK);
    uint8_t wrIdx = (gBtTxFifo.wrIdx & BT_TX_BUF_MASK);

    if (rdIdx < wrIdx)
    {
      numBytes = wrIdx - rdIdx;
    }
    else
    {
      numBytes = BT_TX_BUF_SIZE - rdIdx;
    }
    gBtTxFifo.rdIdx += numBytes;
    ret_val = HAL_UART_Transmit_DMA(huartBtPtr, &gBtTxFifo.data[rdIdx], numBytes);
  }
  else
  {
    messageInProgress = 0; //false
  }
}

void clearBtTxBuf(uint8_t isCalledFromMain)
{
  //uint16_t i;
  /* We don't want to be clearing the TX buffer if main is in the middle to
   * streaming bytes to it */
  //if (isCalledFromMain)
  //{
  RINGFIFO_RESET(gBtTxFifo);

  //Reset all bytes in the buffer -> only used during debugging
  //for(i=BT_TX_BUF_SIZE-1;i<BT_TX_BUF_SIZE;i--)
  //{
  //    *(&gBtTxFifo.data[0]+i) = 0xFF;
  //}
  //}
  //else
  //{
  //setBtClearTxBufFlag(1);
  //}
}

void pushByteToBtTxBuf(uint8_t c)
{
  if (!RINGFIFO_FULL(gBtTxFifo, BT_TX_BUF_MASK))
  {
    RINGFIFO_WR(gBtTxFifo, c, BT_TX_BUF_MASK);
  }
}

void pushBytesToBtTxBuf(uint8_t *buf, uint8_t len)
{
  //uint8_t i;
  //for (i = 0; i < len; i++)
  //{
  //    pushByteToBtTxBuf(*(buf + i));
  //}

  /* if enough space at after head, copy it in */
  uint16_t spaceAfterHead = BT_TX_BUF_SIZE - (gBtTxFifo.wrIdx & BT_TX_BUF_MASK);
  if (spaceAfterHead > len)
  {
    memcpy_vout(&gBtTxFifo.data[(gBtTxFifo.wrIdx & BT_TX_BUF_MASK)], buf, len);
    gBtTxFifo.wrIdx += len;
  }
  else
  {
    /* Fill from head to end of buf */
    memcpy_vout(&gBtTxFifo.data[(gBtTxFifo.wrIdx & BT_TX_BUF_MASK)], buf, spaceAfterHead);
    gBtTxFifo.wrIdx += spaceAfterHead;

    /* Fill from start of buf. We already checked above whether there is
     * enough space in the buf (getSpaceInBtTxBuf()) so we don't need to
     * worry about the tail position. */
    uint16_t remaining = len - spaceAfterHead;
    memcpy_vout(&gBtTxFifo.data[(gBtTxFifo.wrIdx & BT_TX_BUF_MASK)],
        buf + spaceAfterHead, remaining);
    gBtTxFifo.wrIdx += remaining;
  }
}

//https://stackoverflow.com/questions/54964154/is-memcpyvoid-dest-src-n-with-a-volatile-array-safe
volatile void *memcpy_vout(volatile void *dest, const void *src, size_t n)
{
  const uint8_t *src_c = (const uint8_t *) src;
  volatile uint8_t *dest_c = (volatile uint8_t *) dest;

  for (size_t i = 0; i < n; i++)
    dest_c[i] = src_c[i];

  return dest;
}

uint16_t getUsedSpaceInBtTxBuf(void)
{
  return RINGFIFO_COUNT(gBtTxFifo, BT_TX_BUF_MASK);
}

uint16_t getSpaceInBtTxBuf(void)
{
  //Minus 1 as we always need to leave 1 empty byte in the rolling buffer
  return BT_TX_BUF_SIZE - 1 - getUsedSpaceInBtTxBuf();
}

void setBtDataRateTestState(uint8_t state)
{
  dataRateTestState = state;
  *((uint32_t *) &dataRateTestTxPacket[1]) = 0;
}

uint8_t getBtDataRateTestState(void)
{
  return dataRateTestState;
}

void loadBtTxBufForDataRateTest(void)
{
  //uint16_t spaceInTxBuf = getSpaceInBtTxBuf();
  //uint16_t i;
  //if (spaceInTxBuf > sizeof(btDataRateTestCounter) + 1U)
  //{
  //  for (i = 0; i < (spaceInTxBuf / (sizeof(btDataRateTestCounter) + 1U)); i++)
  //  {
  //    pushByteToBtTxBuf(DATA_RATE_TEST_RESPONSE);
  //    pushBytesToBtTxBuf((uint8_t *) &btDataRateTestCounter, sizeof(btDataRateTestCounter));
  //    dataRateTestCounter++;
  //  }
  //}

  //if (getSpaceInBtTxBuf() > (sizeof(dataRateTestCounter) + 1U))
  //{
  //  pushByteToBtTxBuf(DATA_RATE_TEST_RESPONSE);
  //  pushBytesToBtTxBuf((uint8_t*) &dataRateTestCounter,
  //  sizeof(dataRateTestCounter)); dataRateTestCounter++;
  //}

  HAL_StatusTypeDef ret_val = HAL_UART_Transmit_DMA(
      huartBtPtr, &dataRateTestTxPacket[0], sizeof(dataRateTestTxPacket));
  (*((uint32_t *) &dataRateTestTxPacket[1]))++;
}


//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
////  HAL_UART_Transmit_DMA(&huart1,(uint8_t *) "Message Received!\r\n", sizeof("Message Received!\r\n"));
////  HAL_UART_Receive_DMA(&huart1, pRxBuff, 10);
//}

HAL_StatusTypeDef BT_write(uint8_t *buf, uint8_t len)
{
  //HAL_StatusTypeDef ret_val;
  //memcpy(bt_txBuf, buf, len);
  //ret_val = HAL_UART_Transmit_DMA(huart, bt_txBuf, len);

  //SHIMMER_PRINTF("BT_write=%d\n", len);

  pushBytesToBtTxBuf(buf, len);

  sendNextCharIfNotInProgress();

  //ret_val = HAL_UART_Transmit_IT(huart, bt_txBuf, len);
  //if(ret_val == HAL_OK){
  //   PeriStat_Set(STAT_PERI_BT);
  //}
  //return ret_val;
  return HAL_OK;
}

void resetEzsPendingResponse(void)
{
  pending_response = 0;
}

void resetBtRxBuff(void)
{
  memset(rxBuf, 0, sizeof(rxBuf));
}
