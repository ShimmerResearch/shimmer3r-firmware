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

#include "hal_CYW20820.h"

#include "stm32u5xx.h"

#include "log_and_stream_externs.h"
#include <Comms/shimmer_bt_uart.h>

#define CONSOLE_PRINT_NON_EZ_SERIAL_BYTES 0

uint8_t pending_response = 0;
//uint8_t timer_active = 0;
//volatile uint16_t timeout_ms_elapsed;

UART_HandleTypeDef *huartBtPtr;

uint8_t *inBytePtr;

uint8_t bt_txBuf[512];
uint8_t rxBuf[512];
uint16_t expectedByteCount;

uint8_t waitingForBtBoot = 0;
char btBootMsg[160] = { 0 }; //Measured to be 150 chars with v1.4.12.12
uint8_t btBootMsgIndex = 0;
uint8_t btBootMsgLineCount = 0;

uint16_t btRxWaitByteCount = 0;

uint8_t skippingBytesCount = 0;

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

#if ENABLE_BT_TX_DEBUG_PRINTS
  printf("TX data=");
  for (uint16_t i = 0; i < length; i++)
  {
    printf("%c",
        ((data[i] >> 4) & 0xF) < 10 ? ('0' + ((data[i] >> 4) & 0xF)) :
                                      ('A' - 10 + ((data[i] >> 4) & 0xF)));
    printf("%c",
        (data[i] & 0xF) < 10 ? ('0' + (data[i] & 0xF)) : ('A' - 10 + (data[i] & 0xF)));
    printf(" ");
  }
  printf("\r\n");
#endif

  //ret_val = HAL_UART_Transmit_DMA(huartBtPtr, (uint8_t *)data, length);
  //ret_val = HAL_UART_Transmit(huart, (uint8_t *)data, length, 1500*HAL_GetTickFreq());
  ret_val = HAL_UART_Transmit_IT(huartBtPtr, (uint8_t *) data, length);

  if (ret_val != HAL_OK)
  {
    SHIMMER_PRINTF("UART transmit problem in appOutput\r\n");
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
  uint16_t count = 1;

  uint8_t i = 0;
  while (i < expectedByteCount)
  {
    if (waitingForBtBoot)
    {
      btBootMsg[btBootMsgIndex++] = rxBuf[i];
      //SHIMMER_PRINTF("S0=0x%x '%c'\n", rxBuf[i], rxBuf[i]);
      if (btBootMsgIndex > 0 && btBootMsg[btBootMsgIndex - 2] == 0x0D
          && btBootMsg[btBootMsgIndex - 1] == 0x0A)
      {
        btBootMsgLineCount++;
        if (btBootMsgLineCount == 2)
        {
          setWaitingForBtBoot(0);
          //TODO fix architecture and function calling
          progressToNextBtInCmd();
        }
      }
      i += 1;
    }
    else if (skippingBytesCount > 0)
    {
#if (CONSOLE_PRINT_NON_EZ_SERIAL_BYTES)
      SHIMMER_PRINTF("S1=0x%x '%c'\n", rxBuf[i], rxBuf[i]);
#endif
      skippingBytesCount--;
      i += 1;
    }
#if TRANSPARANT_MODE
    ///* If were waiting for the rest of a Shimmer packet or the the EZ Serial
    //* parse is ideal and the header byte is a Shimmer packet header byte,
    //* parse as Shimmer packet */
    //else if (ShimBt_isWaitingForArgs()
    //   || (getEzsPacketLength() == 0 && rxBuf[i] != EZS_BINARY_TYPE_CMDRSP
    //       && rxBuf[i] != (EZS_BINARY_TYPE_CMDRSP | EZS_COMMAND_SCOPE_FLASH)
    //       && rxBuf[i] != EZS_BINARY_TYPE_EVENT))
    //{
    else if (shimmerStatus.btFirstConnectionEstablished)
    {
      //Parse as Shimmer packet
#if (CONSOLE_PRINT_NON_EZ_SERIAL_BYTES)
      SHIMMER_PRINTF("S2=0x%x '%c'\n", rxBuf[i], rxBuf[i]);
#endif
      count = btRxWaitByteCount;
      ShimBt_dmaConversionDone(&rxBuf[i]);
      i += count;
      count = btRxWaitByteCount;
    }
#endif
    else
    {
      ezs_packet_t *result = ezs_parseSingleByte(rxBuf[i]);
      if (result != 0)
      {
        //TODO fix architecture and function calling
        /* If complete EZ Serial packet parsed, send to handler */
        ezsHandlerShimmer(result);
      }
      else
      {
        ezs_input_result_t result = getLastEzsByteParseResult();

        if (result == EZS_INPUT_RESULT_IN_PROGRESS)
        {
          count = getEzsRemainingByteCount();
        }

        //TODO get working if needed (doesn't seem necessary currently)
        else if (result == EZS_INPUT_RESULT_BUFFER_OVERFLOW || result == EZS_INPUT_RESULT_UNHANDLED_PACKET
            || result == EZS_INPUT_RESULT_INVALID_CHECKSUM)
        {
          /* If packet incomplete but byte wasn't recognised as part of an EZ
           * Serial packet, send to Shimmer parser */
          if (getEzsPacketLength() == 0)
          {
#if (CONSOLE_PRINT_NON_EZ_SERIAL_BYTES)
            SHIMMER_PRINTF("S3=0x%x '%c'\n", rxBuf[i], rxBuf[i]);
#endif
          }
        }
      }
      i += 1;
    }
  }

  //Power on check in case SD Sync has turned BT off as part of the sync process
  if (shimmerStatus.btPowerOn)
  {
    if (count == 0)
    {
      count = 1;
    }
    HAL_StatusTypeDef status = setBtRxDmaWaitingForResponse(count);
  }
}

void btUartTxCpltCallback(UART_HandleTypeDef *huart)
{
  ShimBt_TxCpltCallback();
}

HAL_StatusTypeDefShimmer BtTransmit(uint8_t *buf, uint8_t len)
{
#if TRANSPARANT_MODE
  HAL_StatusTypeDef ret_val = HAL_UART_Transmit_DMA(huartBtPtr, buf, len);
#else
  HAL_StatusTypeDef ret_val = HAL_OK;
  ezs_output_result_t ezs_ret;

  ezs_cmd_spp_send_command_t spp_send_command;
  spp_send_command.conn_handle = BT_getConnectionHandle();
  spp_send_command.data.length = len;
  memcpy(spp_send_command.data.data, buf, len);
  ezs_ret = ezs_cmd_spp_send_command(
      spp_send_command.conn_handle, &spp_send_command.data);

  if (ezs_ret != EZS_OUTPUT_RESULT_DATA_WRITTEN)
  {
    SHIMMER_PRINTF("BtTransmit EZS fault=%d\r\n", ezs_ret);
    ret_val = HAL_ERROR;
  }
#endif
  return (HAL_StatusTypeDefShimmer) ret_val;
}

void resetEzsPendingResponse(void)
{
  pending_response = 0;
}

uint8_t isPendingResponseFromBtModule(void)
{
  return pending_response;
}

void resetBtRxBuff(void)
{
  memset(rxBuf, 0, sizeof(rxBuf));
}

void setWaitingForBtBoot(uint8_t state)
{
  waitingForBtBoot = state;
  if (state)
  {
    memset(&btBootMsg[0], 0, sizeof(btBootMsg));
    btBootMsgIndex = 0;
    btBootMsgLineCount = 0;
  }
}

void setSkippingBytesCount(uint8_t count)
{
  skippingBytesCount = count;
}

char *getBtBootMsgPtr(void)
{
  return &btBootMsg[0];
}

void setDmaWaitingForResponse(uint16_t count)
{
  btRxWaitByteCount = count;
}

uint16_t getDmaWaitingForResponse(void)
{
  return btRxWaitByteCount;
}
