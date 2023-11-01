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
* does not authorize its products for use as critical components in life-support
* systems where a malfunction or failure may reasonably be expected to result in
* significant injury to the user. The inclusion of Cypress' product in a life-
* support systems application implies that the manufacturer assumes all risk of
* such use and in doing so indemnifies Cypress against all charges.
*
* Use of this Software may be limited by and subject to the applicable Cypress
* software license agreement.
*******************************************************************************/

#include "stm32u5xx.h"

#include "../EZ-Serial/handlers.h"
#include "../shimmer_bt_comms.h"

uint8_t pending_response = 0;
//uint8_t timer_active = 0;
//volatile uint16_t timeout_ms_elapsed;

UART_HandleTypeDef *huart;

uint8_t *inBytePtr;

uint8_t rxBuf[512];
uint16_t expectedByteCount;

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
        /* NOTE: packet-specific application handler code could be placed right inside
         * this function (appHandler), but this method allows cleaner separation between
         * each block of functionality:
         *  1. EZ-Serial API protocol (ezsapi.c/.h)
         *  2. Platform-specific I/O handlers (handlers.c/.h)
         *  3. Application logic (main.c)
         */
        ezsHandler(packet);
    }
}

ezs_output_result_t appOutput(uint16_t length, const uint8_t *data) {
    /* make sure we aren't already waiting for a response */
    if (pending_response != 0)
    {
        /* only one pending response at a time is allowed */
        return EZS_OUTPUT_RESULT_RESPONSE_PENDING;
    }

    /* increment pending response counter */
    pending_response++;

    /* send data out through UART */
//    UART_SpiUartPutArray((uint8_t *)data, length);
    HAL_StatusTypeDef ret_val;

    ret_val = HAL_UART_Transmit_DMA(huart, (uint8_t *)data, length);
//    ret_val = HAL_UART_Transmit(huart, (uint8_t *)data, length, 1500*HAL_GetTickFreq());
//    ret_val = HAL_UART_Transmit_IT(huart, (uint8_t *)data, length);

    if(ret_val != HAL_OK){
      printf("DMA problem in appOutput\r\n");
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

ezs_input_result_t appInput(uint8_t *inByte, uint16_t timeout) {

  inBytePtr = inByte;

    /* attempt to read a byte from UART */
//  HAL_StatusTypeDef status = HAL_UART_Receive(huart, inByte, 1, timeout);

//  setDmaRx(1);

//    return EZS_INPUT_RESULT_NO_DATA;
}

HAL_StatusTypeDef setDmaRx(uint16_t length) {
  expectedByteCount = length;
  HAL_StatusTypeDef status = HAL_UART_Receive_DMA(huart, &rxBuf[0], expectedByteCount);
  return status;
}

void setBtUartInstance(UART_HandleTypeDef *huartToUse) {
  huart = huartToUse;

  HAL_UART_RegisterCallback(huart, HAL_UART_RX_COMPLETE_CB_ID, btUartDmaRxCpltCallback);
//  HAL_DMA_RegisterCallback(huart->, HAL_DMA_XFER_CPLT_CB_ID, btUartDmaRxCpltCallback);


}
void btUartDmaRxCpltCallback(UART_HandleTypeDef *huart)
{
//  printf("byte received\r\n");
//  printf("%c", rxBuf[0]);

  // if start byte is CYW header byte or if in middle of waiting for full CYW response
//  if(!waitingForArgs
//      && (ezs_rx_packet_length != 0 || (b & EZS_BINARY_SOF_MASK) != 0)) {
//
//    ezs_input_result_t result = EZSerial_Parse(b);
//    if(result == EZS_INPUT_RESULT_IN_PROGRESS
//        || result == EZS_INPUT_RESULT_PACKET_COMPLETE) {
//
//    }
//  } else {
//    //Assume Shimmer command
//  }

//  ezs_packet_t *result = ezs_parseSingleByte(rxBuf[0]);
//  if(result!=0){
//    ezsHandlerShimmer(result);
//  }
//  HAL_StatusTypeDef status = setDmaRx(1);


//  for (uint8_t i = 0; i < expectedByteCount; i++)
//  {
//    ezs_packet_t *result = ezs_parseSingleByte(rxBuf[i]);
//    if (result != 0)
//    {
//      ezsHandlerShimmer(result);
//    }
//  }
//
//  uint16_t count = getEzsRemainingByteCount();
//  if (count == 0)
//  {
//    count = 1;
//  }
//  HAL_StatusTypeDef status = setDmaRx(count);


  uint16_t count = 1;

  for (uint8_t i = 0; i < expectedByteCount; i++)
  {
    /* If were waiting for the rest of a Shimmer packet or the the EZ Serial
     * parse is ideal and the header byte is a Shimmer packet header byte,
     * parse as Shimmer packet */
    if (isWaitingForArgs()
        || (getEzsPacketLength() == 0
            && rxBuf[i] != EZS_BINARY_TYPE_CMDRSP
            && rxBuf[i] != (EZS_BINARY_TYPE_CMDRSP | EZS_COMMAND_SCOPE_FLASH)
            && rxBuf[i] != EZS_BINARY_TYPE_EVENT))
    {
      // Parse as Shimmer packet
      printf("S1=%c(0x%x)\n", rxBuf[i], rxBuf[i]);
//      Dma2ConversionDone();
    }
    else
    {
      ezs_packet_t *result = ezs_parseSingleByte(rxBuf[i]);
      /* If complete EZ Serial packet parsed, send to handler */
      if (result != 0)
      {
        ezsHandlerShimmer(result);
        count = getEzsRemainingByteCount();
      }
      else
      {
        /* If packet incomplete but byte wasn't recognised as part of an EZ
         * Serial packet, send to Shimmer parser */
        if (getEzsPacketLength() == 0)
        {
          printf("S2=%c(0x%x)\n", rxBuf[i], rxBuf[i]);
        }
      }
    }
  }

  if (count == 0)
  {
    count = 1;
  }
  HAL_StatusTypeDef status = setDmaRx(count);

}

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
////  HAL_UART_Transmit_DMA(&huart1,(uint8_t *) "Message Received!\r\n", sizeof("Message Received!\r\n"));
////  HAL_UART_Receive_DMA(&huart1, pRxBuff, 10);
//}
