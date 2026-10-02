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

/* Largest data payload one EZ-Serial SPP_SEND command carries: exactly the
 * module's own limit, measured on IF820 FW v1.4.18.18 (release image; a
 * vendor test image agrees) on 2026-10-01/02 with the cap swept from 255 to
 * 400 bytes, byte by byte from 300 to 305:
 *
 *   data bytes   on the wire   result
 *   255, 300     263, 308      every frame accepted, stream intact
 *   301 and up   309 and up    every frame rejected with EVT_SYSTEM_ERROR
 *                              0x0209 (invalid checksum), nothing sent
 *
 * The module does honour the 11-bit length field (Fix 10): the 258- and
 * 303-byte payloads above need the type-byte MSBs. A 1020-byte SPP_SEND that
 * failed the same way on 2026-08-25 was misread as the MSBs being ignored,
 * which is why this was once 252. Over-length frames are reported as a bad
 * checksum, not 0x020A (invalid command length), so a cap set even slightly
 * too high silently stops all data.
 *
 * Verified on v1.4.18.18 only. BT_selectDataPath() also sends v1.4.17
 * modules down this path, and their limit has not been measured. Kept in sync
 * with BT_TX_MAX_DMA_CHUNK in shimmer_bt_uart.h via a static assert in
 * hal_CYW20820.c. */
#define EZS_SPP_SEND_MAX_DATA_BYTES 300U

/* Bench diagnostic: print min/avg/max EZ-Serial command->response round-trip
 * times, one line per 256 completed commands. During a data-rate test this is
 * effectively the SPP_SEND RTT - the quantity that caps SPP_SEND-framing
 * throughput at chunk_size / RTT. Measured on v1.4.18.18: 3.1 ms avg at 255 B
 * and 3.5 ms at 300 B - roughly 1.1 ms fixed plus 8 us per byte - giving
 * ~58 KB/s and ~64 KB/s with the module's busy retries. Off by default; flip
 * to 1 when investigating throughput. */
#define ENABLE_BT_CMD_RTT_STATS     0

HAL_StatusTypeDefShimmer BtTransmit(const uint8_t *buf, uint16_t len);
uint8_t BtTransmitRetryLast(void);
void BtTransmitAckLast(void);
uint8_t isBtUartTxBusy(void);
void resetEzsPendingResponse(void);
uint8_t isPendingResponseFromBtModule(void);
void resetBtRxBuff(void);

void setWaitingForBtBoot(uint8_t state);
void setSkippingBytesCount(uint8_t count);
char *getBtBootMsgPtr(void);

void setDmaWaitingForResponse(uint16_t count);
uint16_t getDmaWaitingForResponse(void);

#endif /* HANDLERS_H */

/* [] END OF FILE */
