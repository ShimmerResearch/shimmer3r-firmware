/*******************************************************************************
 * Project Name      : EZSerial_Host_Demo
 * File Name         : handlers.h
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

#ifndef HANDLERS_H
#define HANDLERS_H

//#include <project.h>
#include "stm32u5xx.h"
#include <CYW20820/EZ-Serial/ezsapi.h>
#include <log_and_stream_definitions.h>

//CY_ISR_PROTO(TimerInterruptHandler);

void appHandler(ezs_packet_t *packet);
ezs_output_result_t appOutput(uint16_t length, const uint8_t *data);
ezs_input_result_t appInput(uint8_t *inByte, uint16_t timeout);

HAL_StatusTypeDef setBtRxDmaWaitingForResponse(uint16_t length);
void setBtUartInstance(UART_HandleTypeDef *huartToUse);

void btUartDmaRxCpltCallback(UART_HandleTypeDef *huart);
void btUartTxCpltCallback(UART_HandleTypeDef *huart);

uint8_t isEzsBaudRateDelayPending(void);
uint8_t isEzsFactoryRebootDelayPending(void);
void setBtInitCmdsSteps(uint8_t *steps);
void incrementBtInitCmdsStep(void);

void setBtBootModeFactoryReset(void);
void setBtBootModeFirstBoot(void);
void setBtBootModeSubsequentBoot(void);

extern void ezsHandler(ezs_packet_t *packet) __attribute__((weak));
extern void ezsHandlerShimmer(ezs_packet_t *packet) __attribute__((weak));

HAL_StatusTypeDefShimmer BtTransmit(uint8_t *buf, uint8_t len);
void resetEzsPendingResponse(void);
uint8_t isPendingResponse(void);
void resetBtRxBuff(void);

void setWaitingForBtBoot(uint8_t state);
void setSkippingBytesCount(uint8_t count);
char *getBtBootMsgPtr(void);

void setDmaWaitingForResponse(uint16_t count);
uint16_t getDmaWaitingForResponse(void);

#endif /* HANDLERS_H */

/* [] END OF FILE */
