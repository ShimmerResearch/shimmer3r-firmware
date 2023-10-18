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

#include "handlers.h"
#include "stm32u5xx.h"

uint8_t pending_response = 0;
//uint8_t timer_active = 0;
//volatile uint16_t timeout_ms_elapsed;

UART_HandleTypeDef *huart;

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
//    HAL_UART_Transmit_DMA(huart, (uint8_t *)data, length);
    HAL_UART_Transmit(huart, (uint8_t *)data, length, 1500*HAL_GetTickFreq());
    
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

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//
//	HAL_UART_Receive_DMA(huart, inByte, 1);
//}

ezs_input_result_t appInput(uint8_t *inByte, uint16_t timeout) {
    
    /* attempt to read a byte from UART */
	HAL_StatusTypeDef status = HAL_UART_Receive(huart, inByte, 1, timeout);
	if (status == HAL_OK) {
        return EZS_INPUT_RESULT_BYTE_READ;
	} else {
        return EZS_INPUT_RESULT_TIMEOUT;
	}
//    return EZS_INPUT_RESULT_NO_DATA;
}

void setBtUartInstance(UART_HandleTypeDef *huartToUse) {
	huart = huartToUse;
}
