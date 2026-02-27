/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @version        : v2.0_Cube
  * @brief          : Usb device for Virtual Com Port.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_acm_if.h"

/* USER CODE BEGIN INCLUDE */
#include <shimmer_include.h>
/* USER CODE END INCLUDE */

/* USER CODE BEGIN DEFINES */

/* Coalescing: number of timer ticks to wait before sending a short packet (tick = call to CDC_FlushTimerTick). */
#ifndef CDC_COALESCE_DELAY_TICKS
//#define CDC_COALESCE_DELAY_TICKS 2u
#define CDC_COALESCE_DELAY_TICKS 0
#endif
/* USER CODE END DEFINES */

/* RX buffer (one per CDC channel) */
static uint8_t RX_Buffer[NUMBER_OF_CDC][APP_RX_DATA_SIZE];

/* Line coding state */
USBD_CDC_ACM_LineCodingTypeDef Line_Coding[NUMBER_OF_CDC];

/* Optional legacy placeholders */
uint8_t TX_Buffer[NUMBER_OF_CDC][APP_TX_DATA_SIZE];
uint32_t Write_Index[NUMBER_OF_CDC];
uint32_t Read_Index[NUMBER_OF_CDC];

/* Transmit ring context */
typedef struct
{
  volatile uint16_t head; /* ring write */
  volatile uint16_t tail; /* ring read */
  volatile uint16_t in_flight_len; /* bytes currently owned by USB */
  uint8_t need_zlp; /* send ZLP after draining */
  uint8_t ring[CDC_TX_RING_SIZE]; /* ring storage */
  uint8_t pkt_buf[CDC_EP_MAX_PKT];/* staging buffer per packet */
  uint8_t coalesce_ticks; /* remaining ticks before forcing send */
  uint8_t enumerated;
  uint64_t tx_task_entry_count;
  uint64_t tx_interrupt_sent_count;
} cdc_tx_ctx_t;
static cdc_tx_ctx_t cdcTxCtx[NUMBER_OF_CDC];


 UsbRxRingFifo_t usbCmdRx={0};
extern USBD_HandleTypeDef hUsbDevice; /* Provided by USB device stack */

/* Ring helpers */
static inline uint16_t _ring_count(void* c,ring_type_t type)
{
  switch(type)
  {
    case RING_TYPE_CDC_TX:
      cdc_tx_ctx_t* ctx = (cdc_tx_ctx_t*)c;
      return (uint16_t) ((ctx->head + CDC_TX_RING_SIZE - ctx->tail) % CDC_TX_RING_SIZE);
      break;
    case RING_TYPE_CDC_RX:
      UsbRxRingFifo_t* crx = (UsbRxRingFifo_t*)c;
      return (uint16_t) ((crx->usbRingHead + USB_RX_RING_SIZE - crx->usbRingTail) % USB_RX_RING_SIZE);
      break;
    default:
      return 0;
  }
}

static inline uint16_t _ring_free(void *c, ring_type_t type)
{
  switch(type)
  {
    case RING_TYPE_CDC_TX:
      cdc_tx_ctx_t* ctx = (cdc_tx_ctx_t*)c;
      return (uint16_t) (CDC_TX_RING_SIZE - 1 - _ring_count(ctx,type));
      break;
    case RING_TYPE_CDC_RX:
      UsbRxRingFifo_t* crx = (UsbRxRingFifo_t*)c;
      return (uint16_t) (USB_RX_RING_SIZE - 1 - _ring_count(crx,type));
      break;
    default:
      return 0;
  }
}

static void _ring_push(void *c, const uint8_t *data, uint16_t len, ring_type_t type)
{
  switch(type)
  {
    case RING_TYPE_CDC_TX:
      cdc_tx_ctx_t* ctx = (cdc_tx_ctx_t*)c;
      for (uint16_t i = 0; i < len; i++)
      {
        ctx->ring[ctx->head] = data[i];
        ctx->head = (uint16_t) ((ctx->head + 1) % CDC_TX_RING_SIZE);
      }
      break;
    case RING_TYPE_CDC_RX:
      UsbRxRingFifo_t* crx = (UsbRxRingFifo_t*)c;
      for (uint16_t i = 0; i < len; i++)
      {
        crx->usbRingBuf[crx->usbRingHead] = data[i];
        crx->usbRingHead = (uint16_t) ((crx->usbRingHead + 1) % USB_RX_RING_SIZE);
      }
      break;
    default:
      return;
  }
}

/**
 * @brief  Copies data from the ring buffer without removing it.
 * @param  ctx: Pointer to the CDC TX context
 * @param  pData: Destination buffer (pkt_buf)
 * @param  len: Number of bytes to peek
 * @param  type: Ring type identifier
 * @return Number of bytes successfully copied
 */
static uint16_t _ring_peek(cdc_tx_ctx_t *ctx, uint8_t *pData, uint16_t len, uint8_t type)
{
  uint16_t i;
  uint16_t count = _ring_count(ctx, type);

  if (len > count)
  {
    len = count;
  }

  for (i = 0; i < len; i++)
  {
    /* Use the existing tail as a starting point without modifying it */
    pData[i] = ctx->ring[(ctx->tail + i) % CDC_TX_RING_SIZE];
  }

  return len;
}

static uint16_t _ring_pop(void *c, uint8_t *dst, uint16_t maxLen, ring_type_t type)
{
  switch(type)
  {
  case RING_TYPE_CDC_TX:
  {
    cdc_tx_ctx_t *ctx = (cdc_tx_ctx_t*)c;
    uint16_t avail = _ring_count(c, type);
    if (!avail)
     return 0;
    if (avail > maxLen)
     avail = maxLen;
    uint16_t first = avail;
    if (ctx->tail + first > CDC_TX_RING_SIZE)
      first = (uint16_t)(CDC_TX_RING_SIZE - ctx->tail);
    if (dst != NULL) {
        for (uint16_t i = 0; i < first; i++)
            dst[i] = ctx->ring[(ctx->tail + i) % CDC_TX_RING_SIZE];
    }
   ctx->tail = (uint16_t)((ctx->tail + first) % CDC_TX_RING_SIZE);
    return first;
  }
  case RING_TYPE_CDC_RX:
  {
    UsbRxRingFifo_t *crx = (UsbRxRingFifo_t*)c;
    uint16_t avail = _ring_count(crx, type);
    if (!avail)
      return 0;
    uint16_t first = avail;
    if (crx->usbRingTail + avail > USB_RX_RING_SIZE)
      first = (uint16_t)(USB_RX_RING_SIZE - crx->usbRingTail);
    for (uint16_t i = 0; i < first; i++)
      dst[i] = crx->usbRingBuf[(crx->usbRingTail + i) % USB_RX_RING_SIZE];
    crx->usbRingTail = (uint16_t)((crx->usbRingTail + first) % USB_RX_RING_SIZE);
    return first;
  }
  default:
    return 0;
  }
}

/* Forward */
static void CDC_TryStartTx(uint8_t ch, uint8_t forceImmediate);

/* CDC interface callbacks */
/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init(uint8_t cdc_ch)
{
 // CDC_HardResetAll();
  USBD_CDC_SetRxBuffer(cdc_ch, &hUsbDevice, RX_Buffer[cdc_ch]);
  cdc_tx_ctx_t *ctx = &cdcTxCtx[cdc_ch];
    if (ctx->enumerated == 0)  /* Guard to prevent multiple sets */
    {
      ctx->enumerated = 1;
    }

  return (USBD_OK);
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit(uint8_t cdc_ch)
{
  (void) cdc_ch;
  cdc_tx_ctx_t *ctx = &cdcTxCtx[cdc_ch];
        /* Guard to prevent multiple sets */

        ctx->enumerated = 0;

  return (USBD_OK);
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control(uint8_t cdc_ch, uint8_t cmd, uint8_t *pbuf,
    uint16_t length)
{
  (void) length;
  switch (cmd)
  {
  case CDC_SEND_ENCAPSULATED_COMMAND:

    break;

  case CDC_GET_ENCAPSULATED_RESPONSE:

    break;

  case CDC_SET_COMM_FEATURE:

    break;

  case CDC_GET_COMM_FEATURE:

    break;

  case CDC_CLEAR_COMM_FEATURE:

    break;

    /*******************************************************************************/
    /* Line Coding Structure                                                       */
    /*-----------------------------------------------------------------------------*/
    /* Offset | Field       | Size | Value  | Description                          */
    /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
    /* 4      | bCharFormat |   1  | Number | Stop bits                            */
    /*                                        0 - 1 Stop bit                       */
    /*                                        1 - 1.5 Stop bits                    */
    /*                                        2 - 2 Stop bits                      */
    /* 5      | bParityType |  1   | Number | Parity                               */
    /*                                        0 - None                             */
    /*                                        1 - Odd                              */
    /*                                        2 - Even                             */
    /*                                        3 - Mark                             */
    /*                                        4 - Space                            */
    /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
    /*******************************************************************************/
  case CDC_SET_LINE_CODING:
    Line_Coding[cdc_ch].bitrate = (uint32_t) (pbuf[0] | (pbuf[1] << 8)
        | (pbuf[2] << 16) | (pbuf[3] << 24));
    Line_Coding[cdc_ch].format = pbuf[4];
    Line_Coding[cdc_ch].paritytype = pbuf[5];
    Line_Coding[cdc_ch].datatype = pbuf[6];
    break;
  case CDC_GET_LINE_CODING:
    pbuf[0] = (uint8_t) Line_Coding[cdc_ch].bitrate;
    pbuf[1] = (uint8_t) (Line_Coding[cdc_ch].bitrate >> 8);
    pbuf[2] = (uint8_t) (Line_Coding[cdc_ch].bitrate >> 16);
    pbuf[3] = (uint8_t) (Line_Coding[cdc_ch].bitrate >> 24);
    pbuf[4] = Line_Coding[cdc_ch].format;
    pbuf[5] = Line_Coding[cdc_ch].paritytype;
    pbuf[6] = Line_Coding[cdc_ch].datatype;
    break;
  default:
    break;
  }
  return (USBD_OK);
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will issue a NAK packet on any OUT packet received on
  *         USB endpoint until exiting this function. If you exit this function
  *         before transfer is complete on CDC interface (ie. using DMA controller)
  *         it will result in receiving more data while previous ones are still
  *         not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive(uint8_t cdc_ch, uint8_t *Buf, uint32_t *Len)
{

  UsbRxRingFifo_t* c = &usbCmdRx;
  c->usb_interrupt_cdc_receive_entry_count++;
  if (_ring_free(c, RING_TYPE_CDC_RX) == 0)
  {
    c-> usb_cdc_receive_rx_ring_full_count++;
  }
  else
  {
    _ring_push(c, Buf, (uint16_t)*Len, RING_TYPE_CDC_RX);
    ShimTask_set(TASK_USB_PROCESS_CMD);
    c->usb_pr_usb_process_cmd_task_set_count++;
  }
  USBD_CDC_SetRxBuffer(cdc_ch, &hUsbDevice, &Buf[0]);
  USBD_CDC_ReceivePacket(cdc_ch, &hUsbDevice);
  return (USBD_OK);
}

/**
  * @brief  CDC_TransmitCplt_FS
  *         Data transmited callback
  *
  *         @note
  *         This function is IN transfer complete callback used to inform user that
  *         the submitted Data is successfully sent over USB.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
#if 0
static int8_t CDC_TransmitCplt(uint8_t cdc_ch, uint8_t *Buf, uint32_t *Len,
    uint8_t epnum)
{
  UsbRxRingFifo_t* crx = &usbCmdRx;
  crx-> usb_pr_cdc_transmit_cplt_fn_entry_count++;
  (void) Buf;
  (void) Len;
  (void) epnum;
  if (cdc_ch >= NUMBER_OF_CDC)
    return (USBD_OK);
  cdc_tx_ctx_t *ctx = &cdcTxCtx[cdc_ch];
  ctx->in_flight_len = 0;
  if (_ring_count(ctx,RING_TYPE_CDC_TX) == 0 && ctx->need_zlp)
  {
    ctx->need_zlp = 0;
    extern USBD_CDC_ACM_HandleTypeDef CDC_ACM_Class_Data[];
    USBD_CDC_ACM_HandleTypeDef *hcdc = &CDC_ACM_Class_Data[cdc_ch];
    if (hcdc->TxState == 0)
    {
      USBD_CDC_SetTxBuffer(cdc_ch, &hUsbDevice, NULL, 0);
      USBD_CDC_TransmitPacket(cdc_ch, &hUsbDevice);
      return (USBD_OK);
    }
  }
  CDC_TryStartTx(cdc_ch, 0);
  return (USBD_OK);
}
#endif
static int8_t CDC_TransmitCplt(uint8_t cdc_ch, uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
  UsbRxRingFifo_t* crx = &usbCmdRx;
  crx->usb_pr_cdc_transmit_cplt_fn_entry_count++;

  if (cdc_ch >= NUMBER_OF_CDC) return (USBD_OK);

  cdc_tx_ctx_t *ctx = &cdcTxCtx[cdc_ch];
  extern USBD_CDC_ACM_HandleTypeDef CDC_ACM_Class_Data[];
  USBD_CDC_ACM_HandleTypeDef *hcdc = &CDC_ACM_Class_Data[cdc_ch];

  /* ISSUE TYPE 1 FIX: Logic must clear in_flight_len to allow new attempts */
  ctx->in_flight_len = 0;

  /* Handle ZLP Requirement */
  if (ctx->need_zlp)
  {
    ctx->need_zlp = 0;

       USBD_CDC_SetTxBuffer(cdc_ch, &hUsbDevice, NULL, 0);
    if (USBD_CDC_TransmitPacket(cdc_ch, &hUsbDevice) == USBD_OK)
    {
       /* Wait for the ZLP's own Cplt interrupt to call CDC_TryStartTx */
       return (USBD_OK);
    }
  }

  /* Restart the pump for any remaining data in the ring buffer */
  CDC_TryStartTx(cdc_ch, 0);
  return (USBD_OK);
}

USBD_CDC_ACM_ItfTypeDef USBD_CDC_ACM_fops =
{ CDC_Init, CDC_DeInit, CDC_Control, CDC_Receive, CDC_TransmitCplt };

/* Public API */
/**
  * @brief  CDC_Transmit
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit(uint8_t ch, uint8_t *Buf, uint16_t Len)
{
  UsbRxRingFifo_t* crx = &usbCmdRx;
  crx->usb_pr_cdc_transmit_fn_entry_count1++;
  if (ch >= NUMBER_OF_CDC)
    return USBD_FAIL;
  if (Len == 0)
    return USBD_OK;
  cdc_tx_ctx_t *ctx = &cdcTxCtx[ch];
  if (_ring_free(ctx,RING_TYPE_CDC_TX) < Len)
    return USBD_BUSY;

  _ring_push(ctx, Buf, Len, RING_TYPE_CDC_TX);
  crx->usb_pr_cdc_transmit_fn_entry_count2++;
  #if CDC_COALESCE_DELAY_TICKS == 0
  /* Coalescing disabled: start immediately if idle */
  //if (ctx->in_flight_len == 0)
    CDC_TryStartTx(ch, 1);
#else
  /* Coalescing enabled */
  /* If idle and no in-flight, start coalescing timer instead of immediate send unless we already fill a packet */
  if (ctx->in_flight_len == 0)
  {
    if (_ring_count(ctx) >= CDC_EP_MAX_PKT)
    {
      CDC_TryStartTx(ch, 1);
    }
    else if (ctx->coalesce_ticks == 0)
    {
      ctx->coalesce_ticks = (uint8_t) CDC_COALESCE_DELAY_TICKS;
    }
  }
#endif

  return USBD_OK;
}

uint16_t CDC_TxFree(uint8_t ch,ring_type_t type)
{
  if (ch >= NUMBER_OF_CDC)
    return 0;
  return _ring_free(&cdcTxCtx[ch],type);
}
uint16_t CDC_TxPending(uint8_t ch,ring_type_t type)
{
  if (ch >= NUMBER_OF_CDC)
    return 0;
  return _ring_count(&cdcTxCtx[ch],type);
}

void CDC_Flush(uint8_t ch)
{
  if (ch >= NUMBER_OF_CDC)
    return;
  cdc_tx_ctx_t *ctx = &cdcTxCtx[ch];
  ctx->coalesce_ticks = 0;
  CDC_TryStartTx(ch, 1);
}

void CDC_FlushAll(void)
{
  for (uint8_t i = 0; i < NUMBER_OF_CDC; i++)
    CDC_Flush(i);
}

void CDC_FlushTimerTick(uint8_t ch)
{
  if (ch >= NUMBER_OF_CDC)
    return;
  cdc_tx_ctx_t *ctx = &cdcTxCtx[ch];
  if (ctx->in_flight_len == 0 && _ring_count(ctx,RING_TYPE_CDC_TX) > 0)
  {
    if (ctx->coalesce_ticks > 0)
    {
      ctx->coalesce_ticks--;
      if (ctx->coalesce_ticks == 0)
        CDC_TryStartTx(ch, 1);
    }
  }
}

#if 0
/* Internal: attempt to start transmission */
static void CDC_TryStartTx(uint8_t ch, uint8_t forceImmediate)
{
  UsbRxRingFifo_t* crx = &usbCmdRx;
  if (ch >= NUMBER_OF_CDC)
    return;
  cdc_tx_ctx_t *ctx = &cdcTxCtx[ch];
  if (ctx->in_flight_len != 0)
    return;
  if (_ring_count(ctx,RING_TYPE_CDC_TX) == 0)
    return;
  if (!forceImmediate && ctx->coalesce_ticks)
    return;
  extern USBD_CDC_ACM_HandleTypeDef CDC_ACM_Class_Data[];
  USBD_CDC_ACM_HandleTypeDef *hcdc = &CDC_ACM_Class_Data[ch];
  if (hcdc->TxState != 0)
    return;
  uint16_t sendLen = _ring_pop(ctx, ctx->pkt_buf, CDC_EP_MAX_PKT,RING_TYPE_CDC_TX);
  if (!sendLen)
    return;
  ctx->in_flight_len = sendLen;
  ctx->need_zlp =
      (sendLen == CDC_EP_MAX_PKT && _ring_count(ctx,RING_TYPE_CDC_TX) == 0) ? 1u : 0u;
  ctx->coalesce_ticks = 0;
  USBD_CDC_SetTxBuffer(ch, &hUsbDevice, ctx->pkt_buf, sendLen);
  if(USBD_CDC_TransmitPacket(ch, &hUsbDevice) != USBD_OK)
  {
    crx->usb_pr_cdc_transmit_cplt_fn_error_count++;
  }
}
#endif
static void CDC_TryStartTx(uint8_t ch, uint8_t forceImmediate)
{
  UsbRxRingFifo_t* crx = &usbCmdRx;
  if (ch >= NUMBER_OF_CDC) return;

  cdc_tx_ctx_t *ctx = &cdcTxCtx[ch];

  if (!ctx->enumerated)
     return;

  extern USBD_CDC_ACM_HandleTypeDef CDC_ACM_Class_Data[];
  USBD_CDC_ACM_HandleTypeDef *hcdc = &CDC_ACM_Class_Data[ch];

  /* --- NEW RECOVERY LOGIC START --- */
  /* RECOVERY 1: Fix 'Orphaned' logic If hardware says it's IDLE (0) but our logic is stuck at 13, reset it. */
  __disable_irq();
  if (hcdc->TxState == 0 && ctx->in_flight_len != 0)
  {
  ctx->in_flight_len = 0;
  }

  /* --- NEW RECOVERY LOGIC END --- */

  /* ISSUE TYPE 2 FIX: Ensure we don't start if logic thinks we are busy
     OR if the hardware library is still in a busy state */
  if (ctx->in_flight_len != 0 || hcdc->TxState != 0)
  {
    __enable_irq();
    return;
  }
  __enable_irq();
  uint16_t count = _ring_count(ctx, RING_TYPE_CDC_TX);
  if (count == 0)
    return;

  if (!forceImmediate && ctx->coalesce_ticks)
    return;

  /* Peek instead of Pop: Keep data in ring until hardware accepts it */
  uint16_t sendLen = (count > CDC_EP_MAX_PKT) ? CDC_EP_MAX_PKT : count;
  _ring_peek(ctx, ctx->pkt_buf, sendLen, RING_TYPE_CDC_TX);

  USBD_CDC_SetTxBuffer(ch, &hUsbDevice, ctx->pkt_buf, sendLen);

  /* CRITICAL CHANGE: Only update state if hardware returns USBD_OK */
  if (USBD_CDC_TransmitPacket(ch, &hUsbDevice) == USBD_OK)
  {
    ctx->in_flight_len = sendLen;
    /* Determine if we need ZLP: packet is full and no more data in ring */
    ctx->need_zlp = (sendLen == CDC_EP_MAX_PKT) ? 1u : 0u;
    ctx->coalesce_ticks = 0;
    /* Now safe to remove from ring buffer */
    _ring_pop(ctx, NULL, sendLen, RING_TYPE_CDC_TX);
  }
  else
  {
    /* Hardware rejected it: hcdc->TxState was likely 1.
       Wait for the next Cplt interrupt to trigger this again. */
    crx->usb_pr_cdc_transmit_cplt_fn_error_count++;
  }
}

uint8_t ShimUsbProcessCmd(void)
{
  UsbRxRingFifo_t *c = &usbCmdRx;
  c->usb_pr_cmd_fn_entry_count_Start1++;
  uint8_t data[150] = {0};
  uint8_t temp = 0;
 static uint8_t sendLen = 0;
 static uint8_t pending = 0;
 static uint8_t offset = 0;
 uint8_t dollar_found = 0;
static  uint64_t count_end = 0;
static  uint64_t count_start = 0;
 uint8_t count = _ring_count(c, RING_TYPE_CDC_RX);
 if (!count && !pending)
   return USBD_BUSY;

 for(offset = 0 ; offset < count;offset++)
 {
   if(c->usbRingBuf[(c->usbRingTail+offset)% USB_RX_RING_SIZE] == '$')
   {
     dollar_found = 1;
     break;
   }
 }
 if (!dollar_found)
 {
     if (!pending)
     {
         // Junk bytes, discard them all
         _ring_pop(c, data, offset, RING_TYPE_CDC_RX);
         return USBD_BUSY;
     }
     else
     {
         // Waiting for previous pending command → do nothing
         return USBD_BUSY;
     }
 }

 if (offset > 0 && !pending)
 {
   _ring_pop(c, data, offset, RING_TYPE_CDC_RX); //
 }
 else if(offset > 0 && pending)
 {
   // Bytes before '$' are part of previously pending command
   // Do nothing, wait for remaining bytes to arrive
   // pending stays 1
 }
 c->usb_pr_cmd_fn_entry_count_Start2++;
 // tail is now guaranteed to point to '$' and offset 0
 count = _ring_count(c, RING_TYPE_CDC_RX);
 if(count>=3)
 {
   sendLen = c->usbRingBuf[(c->usbRingTail+2)% USB_RX_RING_SIZE]+5;
   if(sendLen <= count)
   {
     _ring_pop(c,data,sendLen,RING_TYPE_CDC_RX);
     for(uint8_t i = 0; i<sendLen; i++)
     {

       ShimDock_rxCallback(data[i]);
     }
     pending = 0;
     sendLen = 0;
     c->usb_pr_cmd_clbck_count_end++;
     return USBD_OK;
   }
   else
   {
     pending = 1;
     return USBD_BUSY;
  }
}
return USBD_OK;
}

void CDC_HardResetAll(void)
{
  for (uint8_t i = 0; i < NUMBER_OF_CDC; i++)
  {
    cdc_tx_ctx_t *ctx = &cdcTxCtx[i];
    __disable_irq();
    ctx->head = 0;
    ctx->tail = 0;
    ctx->in_flight_len = 0;      /* CRITICAL */
    ctx->need_zlp = 0;
    ctx->coalesce_ticks = 0;
    ctx->enumerated = 0;
    ctx->enumerated = 0;  /* Reset during USB reset */
    __enable_irq();
  }
}

void cdc_setEnumerate(void)
{
  cdc_tx_ctx_t *ctx = &cdcTxCtx[0];
  ctx->enumerated = 1;
}

/* USER CODE BEGIN Additional_User_Code */
/* USER CODE END Additional_User_Code */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
