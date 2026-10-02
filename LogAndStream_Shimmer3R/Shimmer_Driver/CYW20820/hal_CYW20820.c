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
#include <CYW20820/CYW20820.h>
#include <Comms/shimmer_bt_uart.h>

#define CONSOLE_PRINT_NON_EZ_SERIAL_BYTES 0

volatile uint8_t pending_response = 0;

/* Last SPP_SEND payload, kept for module-rejection retries (see
 * BtTransmitRetryLast). Static rather than stack: BtTransmit() runs in ISR
 * context and a longuint8a_t is 514 bytes; serialized by the
 * pending_response / UART-TX-busy guards in BtTransmit(). */
static longuint8a_t sppSendData;
static volatile uint16_t sppSendRetryCount = 0;
/* An EZ-Serial command frame is clocking out of the UART: set just before
 * appOutput() starts it, cleared on TX completion. Anything else on this UART
 * is a raw bridged transfer from BtTransmit(), so this one flag also tells the
 * TX-complete callback what just finished - a command (the module's response
 * drives the chain) or raw bytes (the callback drives it). BtTransmitAbort()
 * must never cut a command short. volatile: set in main or interrupt
 * context, read in the TX-complete callback. */
static volatile uint8_t btCmdTxInFlight = 0;
/* BtTransmitAbort() owns the UART TX: it found no command in flight and is
 * aborting. appOutput() starts no command until it is clear, so one cannot
 * begin between that check and the abort and be cut short. */
static volatile uint8_t btTxAbortInProgress = 0;
/* Retry budget for a rejected SPP_SEND. Deliberately SHORT: a retrying payload
 * serialises the whole TX chain behind it, so the budget must stay well under
 * the host's per-command timeout (~2 s). At ~1.4 ms per 0x0502
 * CONNECTION_REQUIRED attempt, 200 is ~0.3 s. Raising it to 1500 (~2.1 s,
 * bench 2026-09-07) turned a one-response stutter at BLE connect into every
 * command timing out for the whole session. The remaining cost of the short
 * budget is that a response sent in the ~0.5 s between a GATT connection and
 * the CYSPP data channel engaging can be dropped; the host re-issues. */
#define BT_SPP_SEND_RETRY_LIMIT 200U
//uint8_t timer_active = 0;
//volatile uint16_t timeout_ms_elapsed;

UART_HandleTypeDef *huartBtPtr;

uint8_t *inBytePtr;

uint8_t bt_txBuf[512];
uint8_t rxBuf[512];
volatile uint16_t expectedByteCount;

volatile uint8_t waitingForBtBoot = 0;
char btBootMsg[160] = { 0 }; //Measured to be 150 chars with v1.4.12.12
volatile uint8_t btBootMsgIndex = 0;
volatile uint8_t btBootMsgLineCount = 0;

volatile uint16_t btRxWaitByteCount = 0;

/* volatile: written by setSkippingBytesCount() from the boot sequence and
 * decremented inside the UART RX-complete callback. */
volatile uint8_t skippingBytesCount = 0;

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

_Static_assert(EZS_SPP_SEND_MAX_DATA_BYTES <= EZS_LONGUINT8A_ACTUAL_MAX,
    "SPP_SEND payload cap must fit longuint8a_t");
_Static_assert(BT_TX_MAX_DMA_CHUNK <= EZS_SPP_SEND_MAX_DATA_BYTES,
    "TX ring chunks must fit in one SPP_SEND command");

#if ENABLE_BT_CMD_RTT_STATS
static uint32_t rttStartCyc, rttMinCyc = UINT32_MAX, rttMaxCyc, rttCount;
static uint64_t rttSumCyc;

static void btCmdRttStart(void)
{
  /* Lazily enable the DWT cycle counter; wrap-safe unsigned deltas give a
   * ~26 s measurement range at 160 MHz, far beyond any command RTT. */
  if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
  {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  }
  rttStartCyc = DWT->CYCCNT;
}

static void btCmdRttStop(void)
{
  uint32_t deltaCyc = DWT->CYCCNT - rttStartCyc;

  if (deltaCyc < rttMinCyc)
  {
    rttMinCyc = deltaCyc;
  }
  if (deltaCyc > rttMaxCyc)
  {
    rttMaxCyc = deltaCyc;
  }
  rttSumCyc += deltaCyc;
  rttCount++;

  if (rttCount >= 256U)
  {
    uint32_t cycPerUs = SystemCoreClock / 1000000U;
    SHIMMER_PRINTF("BT cmd RTT: n=%lu min=%luus avg=%luus max=%luus\r\n",
        rttCount, rttMinCyc / cycPerUs,
        (uint32_t) (rttSumCyc / rttCount) / cycPerUs, rttMaxCyc / cycPerUs);
    rttMinCyc = UINT32_MAX;
    rttMaxCyc = 0;
    rttSumCyc = 0;
    rttCount = 0;
  }
}
#endif /* ENABLE_BT_CMD_RTT_STATS */

void appHandler(ezs_packet_t *packet)
{
  if (packet->packet_type == EZS_PACKET_TYPE_RESPONSE)
  {
    /* clear pending response flag */
    if (pending_response != 0)
    {
      pending_response = 0;
#if ENABLE_BT_CMD_RTT_STATS
      btCmdRttStop();
#endif
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
  pending_response = 1;

#if ENABLE_BT_CMD_RTT_STATS
  btCmdRttStart();
#endif

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
  /* The checks, the flag and the start are one step: BtTransmitAbort() and
   * BtTransmit() can run from interrupts of either priority relative to this.
   * - A raw bridged transfer still running means HAL_UART_Transmit_IT() would
   *   refuse anyway. Refusing here, before the flag is set, also stops that
   *   transfer's completion - if it landed in between - being taken for this
   *   command's by the TX-complete callback, which would drop the raw chain.
   * - While BtTransmitAbort() holds the UART, a command started now would be
   *   cut short by it.
   * The flag is set before the transmit starts because a short frame can
   * complete, and its callback clear the flag, before HAL_UART_Transmit_IT()
   * returns - with interrupts masked here that callback runs after the
   * restore, so the order still holds. HAL_UART_Transmit_IT() only arms the
   * TX interrupt; it does not wait. */
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  if (btTxAbortInProgress || isBtUartTxBusy())
  {
    ret_val = HAL_BUSY;
  }
  else
  {
    btCmdTxInFlight = 1;
    ret_val = HAL_UART_Transmit_IT(huartBtPtr, (uint8_t *) data, length);
    if (ret_val != HAL_OK)
    {
      /* Still masked: once interrupts are back a raw transfer could start
       * and complete, and a flag left set would have it taken for a
       * command's completion */
      btCmdTxInFlight = 0;
    }
  }
  __set_PRIMASK(primask);

  if (ret_val != HAL_OK)
  {
    /* Nothing was sent, so no response is coming: leaving pending_response
     * set here would block every later command forever, and returning
     * DATA_WRITTEN would tell the caller the command is in flight when it is
     * not. Roll back and report the failure so callers retry. btCmdTxInFlight
     * is already clear: never set, or cleared above while still masked. */
    SHIMMER_PRINTF("UART transmit problem in appOutput\r\n");
    pending_response = 0;
    return EZS_OUTPUT_RESULT_NO_HANDLER;
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
    else if (getBtCysppState())
    {
      /* Parse as Shimmer packet, gated on the LIVE data-mode state from the
       * CYSPP pin. Bench (2026-08-25, v1.4.18.18): the module actively hops
       * between SPP data mode (pin LOW - UART bytes are bridged payload) and
       * command mode (pin HIGH - UART bytes are EZ-Serial frames, e.g. the
       * connection/pairing/disconnect events it exits data mode to deliver),
       * even while a connection is up. The pin is therefore the demux
       * signal, and neither connection state (earlier attempt: the in-band
       * connected event got eaten the moment the gate opened) nor a sticky
       * first-connection flag (eats every event after the first connection
       * of a power cycle) can stand in for it. */
#if (CONSOLE_PRINT_NON_EZ_SERIAL_BYTES)
      SHIMMER_PRINTF("S2=0x%x '%c'\n", rxBuf[i], rxBuf[i]);
#endif
      count = btRxWaitByteCount;
      ShimBt_dmaConversionDone(&rxBuf[i]);
      /* Never advance by zero. btRxWaitByteCount starts at 0 and is only set
       * once the Shimmer parser is waiting for a known number of bytes, and
       * some of the values it is set from are computed lengths - so a 0 here
       * is not provably impossible, and it would spin this loop forever
       * inside the UART RX-complete interrupt. Consuming one byte keeps the
       * loop bounded by expectedByteCount. */
      i += (count > 0U) ? count : 1U;
      count = btRxWaitByteCount;
    }
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
  /* Decide from what just completed. This used to read btLastTxWasRaw, which
   * BtTransmit() could only set after HAL_UART_Transmit_DMA() returned - and a
   * 1-byte raw transfer (the ACK that starts a data-rate test) can complete
   * before then, so the callback saw the previous transfer's value. After a
   * classic SPP_SEND session that was 0: the raw chain was never advanced and
   * BLE went silent, with btTxInProgress stuck at 1 and nothing in flight
   * (bench, SWD-confirmed). appOutput() marks a command before starting it, so
   * this cannot lag the way the old flag did. */
  uint8_t completedCmd = btCmdTxInFlight;
  btCmdTxInFlight = 0;
  ShimBt_TxCpltCallback();

  if (!completedCmd)
  {
    /* A raw bridged transfer has no SPP_SEND response to drive the transfer
     * chain, so the DMA completion is the moment to hand the UART the next
     * chunk - the pre-split mainline behaviour. Keyed on what was actually
     * sent, not on the classic-SPP policy: a BLE CYSPP data pipe is raw on
     * every module version. The common repo cannot see any of this, which is
     * why it lives here and not in ShimBt_TxCpltCallback(). */
    ShimBt_triggerNextTransfer();
  }
}

HAL_StatusTypeDefShimmer BtTransmit(const uint8_t *buf, uint16_t len)
{
  /* A raw data bridge is engaged: a BLE CYSPP data pipe (any module version -
   * SPPM bit 1 governs classic SPP only), or classic transparent SPP on a
   * legacy module. Raw DMA is the only correct transmit here; an SPP_SEND
   * command would be injected into the pipe as garbage, and on a BLE pipe the
   * module rejects it outright (0x0502 CONNECTION_REQUIRED - there is no
   * classic SPP link to send on). */
  if (getBtCysppState())
  {
    /* One step with appOutput()'s check-and-start: HAL_UART_Transmit_DMA()'s
     * own READY test and BUSY_TX claim are two steps, so an interrupt calling
     * appOutput() between them would start a command on the same UART. Held
     * off while BtTransmitAbort() owns the UART, whose abort would otherwise
     * leave gState READY over a transfer started mid-abort. The DMA start
     * only programs the channel; it does not wait. */
    HAL_StatusTypeDef rawRet = HAL_BUSY;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (!btTxAbortInProgress)
    {
      rawRet = HAL_UART_Transmit_DMA(huartBtPtr, buf, len);
    }
    __set_PRIMASK(primask);
    return (HAL_StatusTypeDefShimmer) rawRet;
  }

  if (BT_isTransparentMode() || BT_isBleSessionActive())
  {
    /* The transport is a raw pipe but it is not carrying data right now - no
     * bridge yet, or a command-mode window mid-session. Neither framing works
     * here: raw bytes would hit the module's EZ-Serial parser as garbage
     * commands, and an SPP_SEND would be rejected outright on a BLE session
     * (0x0502 CONNECTION_REQUIRED - there is no classic SPP link), which is
     * what produced the rejection bursts on the bench. Hold instead; the ring
     * keeps the data and the pipe resuming kicks the drain. */
    return HAL_SHIM_BUSY;
  }

  /* SPP_SEND framing (modules v1.4.17+, classic SPP, no bridge active) */
  HAL_StatusTypeDef ret_val = HAL_OK;
  ezs_output_result_t ezs_ret;

  /* Refuse before building the command: ezs_cmd_va() writes into the global
   * ezs_tx_packet, so constructing a command while another is outstanding
   * would corrupt a frame the UART may still be clocking out. appOutput()'s
   * own pending check happens only after that damage is done. Callers treat
   * any non-OK as retry-later. */
  if (isPendingResponseFromBtModule())
  {
    return HAL_SHIM_BUSY;
  }

  /* Same hazard from the other side: pending_response can already be cleared
   * (a module error discards the in-flight command) while the UART is still
   * clocking that frame out of ezs_tx_packet. Building a new command now
   * would corrupt the bytes still being transmitted. Callers treat any
   * non-OK as retry-later; the module's response or error event for the
   * in-flight frame restarts the chain. */
  if (isBtUartTxBusy())
  {
    return HAL_SHIM_BUSY;
  }

  /* The frame's 8-bit payload length caps one SPP_SEND at
   * EZS_SPP_SEND_MAX_DATA_BYTES of data. An oversized frame is worse than a
   * refused one: the module never answers a corrupt frame, so
   * pending_response would stay set and mute TX for the rest of the power
   * cycle. */
  if (len > EZS_SPP_SEND_MAX_DATA_BYTES)
  {
    SHIMMER_PRINTF("BtTransmit: %u > SPP_SEND max %u\r\n", len,
        (uint16_t) EZS_SPP_SEND_MAX_DATA_BYTES);
    return HAL_SHIM_ERROR;
  }

  sppSendData.length = len;
  memcpy(sppSendData.data, buf, len);
  sppSendRetryCount = 0;

  ezs_ret = ezs_cmd_spp_send_command(BT_getConnectionHandle(), &sppSendData);

  if (ezs_ret != EZS_OUTPUT_RESULT_DATA_WRITTEN)
  {
    SHIMMER_PRINTF("BtTransmit EZS fault=%d\r\n", ezs_ret);
    ret_val = HAL_ERROR;
  }
  return (HAL_StatusTypeDefShimmer) ret_val;
}

/* Re-issue the last SPP_SEND payload after the module rejected it - most
 * commonly EZS_ERR_CORE_INSUFFICIENT_RESOURCES (0x0109) when its SPP TX queue
 * toward the radio is full. The payload survives in sppSendData (the TX ring
 * released its copy when the UART transfer completed), so a rejection costs
 * nothing but the retry round trip - which is also what paces us to the rate
 * the radio actually drains. Returns 1 if the retry was issued, 0 if the
 * payload was dropped (retry budget exhausted) or nothing was pending.
 *
 * The retry cap bounds a persistent-failure loop (e.g. rejected while the
 * link is tearing down): at ~3 ms per attempt, 200 tries is ~0.6 s before the
 * chunk is abandoned and the stream moves on. */
uint8_t BtTransmitRetryLast(void)
{
  if (sppSendData.length == 0)
  {
    return 0;
  }

  if (++sppSendRetryCount > BT_SPP_SEND_RETRY_LIMIT)
  {
    SHIMMER_PRINTF("BtTransmit: dropped %u bytes after %u rejected sends\r\n",
        sppSendData.length, (uint16_t) BT_SPP_SEND_RETRY_LIMIT);
    sppSendData.length = 0;
    sppSendRetryCount = 0;
    return 0;
  }

  if (isPendingResponseFromBtModule() || isBtUartTxBusy())
  {
    return 0;
  }

  if (ezs_cmd_spp_send_command(BT_getConnectionHandle(), &sppSendData) != EZS_OUTPUT_RESULT_DATA_WRITTEN)
  {
    return 0;
  }

  return 1;
}

/* The module accepted the last SPP_SEND: invalidate the held payload so no
 * later recovery path (e.g. a system-error retry for an unrelated command)
 * can ever re-send it and duplicate data in the stream. */
void BtTransmitAckLast(void)
{
  sppSendData.length = 0;
  sppSendRetryCount = 0;
}

/* Overrides the weak no-op in shimmer_bt_uart.c, which calls it from
 * ShimBt_clearBtTxBuf() so that no stale data goes out after the clear and the
 * TX-complete callback cannot advance the read index over the reset ring.
 *
 * An EZ-Serial command frame is never cut short. The module holds a partial
 * command until its own timeout, then raises EVT_SYSTEM_ERROR 0x0207 and
 * discards it. Aborting SPP_SEND frames that way cost that timeout on every
 * data-rate test stop (bench-confirmed, one 0x0207 per stop), and the
 * system-error recovery then re-sent the held payload anyway. So the frame
 * is left to finish, and the held SPP_SEND payload is dropped instead - which
 * is what actually keeps stale data from being re-sent: a busy (0x0109) or
 * failed response to the frame in flight finds nothing to retry and moves
 * the chain on. Finishing is safe for the ring: the frame lives in
 * ezs_tx_packet, not the ring, and on this path TX completion only advances
 * rdIdx by numBytesBeingRead, which the caller zeroes before the reset.
 *
 * A raw bridged DMA transfer is still aborted: the module only forwards those
 * bytes, so cutting one just means fewer stale bytes.
 *
 * Only on an initialised UART. BtStop() runs btDeinit() - HAL_UART_Abort()
 * then HAL_UART_DeInit() - before ShimBt_stopCommon() clears the TX ring and
 * calls this, and HAL_UART_AbortTransmit() ends by setting gState to READY
 * unconditionally. On a de-initialised handle that READY is stale: the next
 * HAL_UART_Init() runs MspInit (USART3 clock, pins, DMA channels, IRQ) only
 * when gState is RESET, so it skipped it and the BT UART stayed dead for
 * every later BtStart(). The boot baud ladder always retries on a module
 * still at its factory 115200 baud, so on a never-configured module BT never
 * came up (MAC 0000, DEV-962). Skipping loses nothing: the HAL_UART_Abort()
 * in btDeinit() has already stopped any transfer in flight. */
void BtTransmitAbort(void)
{
  sppSendData.length = 0;
  sppSendRetryCount = 0;

  if (huartBtPtr == NULL || huartBtPtr->gState == HAL_UART_STATE_RESET)
  {
    /* De-initialised: btDeinit()'s HAL_UART_Abort() ended any transfer
     * without a completion callback, so nothing is in flight */
    btCmdTxInFlight = 0;
    return;
  }

  /* Check and claim in one step. This can run from the BT_CYSPP EXTI (the
   * lowest priority), and a higher-priority interrupt could otherwise start a
   * command between finding none in flight and the abort, which would then cut
   * that command short. The abort itself runs with interrupts enabled:
   * HAL_DMA_Abort() polls for the channel suspend against a HAL_GetTick()
   * timeout, which masked interrupts would freeze. */
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint8_t cmdInFlight = btCmdTxInFlight;
  if (!cmdInFlight)
  {
    btTxAbortInProgress = 1;
  }
  __set_PRIMASK(primask);

  if (cmdInFlight)
  {
    return;
  }

  HAL_UART_AbortTransmit(huartBtPtr);
  btTxAbortInProgress = 0;
}

void resetEzsPendingResponse(void)
{
  pending_response = 0;
}

uint8_t isPendingResponseFromBtModule(void)
{
  return pending_response;
}

uint8_t isBtUartTxBusy(void)
{
  /* gState carries the TX half of the HAL UART state machine (RxState the
   * other); BUSY_TX here means an interrupt-driven transmit of ezs_tx_packet
   * is still clocking out. */
  return huartBtPtr != 0 && huartBtPtr->gState != HAL_UART_STATE_READY;
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
