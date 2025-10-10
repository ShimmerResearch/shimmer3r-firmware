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
#ifndef APP_RX_DATA_SIZE
#define APP_RX_DATA_SIZE 128
#endif
#ifndef APP_TX_DATA_SIZE
#define APP_TX_DATA_SIZE 128
#endif
#ifndef CDC_EP_MAX_PKT
#define CDC_EP_MAX_PKT 64u
#endif
#ifndef CDC_TX_RING_SIZE
#define CDC_TX_RING_SIZE 512u
#endif
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
} cdc_tx_ctx_t;
static cdc_tx_ctx_t cdcTxCtx[NUMBER_OF_CDC];

extern USBD_HandleTypeDef hUsbDevice; /* Provided by USB device stack */

/* Ring helpers */
static inline uint16_t _ring_count(cdc_tx_ctx_t *c)
{
  return (uint16_t) ((c->head + CDC_TX_RING_SIZE - c->tail) % CDC_TX_RING_SIZE);
}

static inline uint16_t _ring_free(cdc_tx_ctx_t *c)
{
  return (uint16_t) (CDC_TX_RING_SIZE - 1 - _ring_count(c));
}

static void _ring_push(cdc_tx_ctx_t *c, const uint8_t *data, uint16_t len)
{
  for (uint16_t i = 0; i < len; i++)
  {
    c->ring[c->head] = data[i];
    c->head = (uint16_t) ((c->head + 1) % CDC_TX_RING_SIZE);
  }
}

static uint16_t _ring_pop(cdc_tx_ctx_t *c, uint8_t *dst, uint16_t maxLen)
{
  uint16_t avail = _ring_count(c);
  if (!avail)
    return 0;
  if (avail > maxLen)
    avail = maxLen;
  uint16_t first = avail;
  if (c->tail + first > CDC_TX_RING_SIZE)
    first = (uint16_t) (CDC_TX_RING_SIZE - c->tail);
  for (uint16_t i = 0; i < first; i++)
    dst[i] = c->ring[(c->tail + i) % CDC_TX_RING_SIZE];
  c->tail = (uint16_t) ((c->tail + first) % CDC_TX_RING_SIZE);
  return first;
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
  USBD_CDC_SetRxBuffer(cdc_ch, &hUsbDevice, RX_Buffer[cdc_ch]);
  return (USBD_OK);
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit(uint8_t cdc_ch)
{
  (void) cdc_ch;
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
  for (uint32_t i = 0; i < *Len; i++)
  {
    ShimDock_rxCallback(Buf[i]);
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
static int8_t CDC_TransmitCplt(uint8_t cdc_ch, uint8_t *Buf, uint32_t *Len,
    uint8_t epnum)
{
  (void) Buf;
  (void) Len;
  (void) epnum;
  if (cdc_ch >= NUMBER_OF_CDC)
    return (USBD_OK);
  cdc_tx_ctx_t *ctx = &cdcTxCtx[cdc_ch];
  ctx->in_flight_len = 0;
  if (_ring_count(ctx) == 0 && ctx->need_zlp)
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
  if (ch >= NUMBER_OF_CDC)
    return USBD_FAIL;
  if (Len == 0)
    return USBD_OK;
  cdc_tx_ctx_t *ctx = &cdcTxCtx[ch];
  if (_ring_free(ctx) < Len)
    return USBD_BUSY;

  _ring_push(ctx, Buf, Len);

  #if CDC_COALESCE_DELAY_TICKS == 0
  /* Coalescing disabled: start immediately if idle */
  if (ctx->in_flight_len == 0)
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

uint16_t CDC_TxFree(uint8_t ch)
{
  if (ch >= NUMBER_OF_CDC)
    return 0;
  return _ring_free(&cdcTxCtx[ch]);
}
uint16_t CDC_TxPending(uint8_t ch)
{
  if (ch >= NUMBER_OF_CDC)
    return 0;
  return _ring_count(&cdcTxCtx[ch]);
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
  if (ctx->in_flight_len == 0 && _ring_count(ctx) > 0)
  {
    if (ctx->coalesce_ticks > 0)
    {
      ctx->coalesce_ticks--;
      if (ctx->coalesce_ticks == 0)
        CDC_TryStartTx(ch, 1);
    }
  }
}

/* Internal: attempt to start transmission */
static void CDC_TryStartTx(uint8_t ch, uint8_t forceImmediate)
{
  if (ch >= NUMBER_OF_CDC)
    return;
  cdc_tx_ctx_t *ctx = &cdcTxCtx[ch];
  if (ctx->in_flight_len != 0)
    return;
  if (_ring_count(ctx) == 0)
    return;
  if (!forceImmediate && ctx->coalesce_ticks)
    return;
  extern USBD_CDC_ACM_HandleTypeDef CDC_ACM_Class_Data[];
  USBD_CDC_ACM_HandleTypeDef *hcdc = &CDC_ACM_Class_Data[ch];
  if (hcdc->TxState != 0)
    return;
  uint16_t sendLen = _ring_pop(ctx, ctx->pkt_buf, CDC_EP_MAX_PKT);
  if (!sendLen)
    return;
  ctx->in_flight_len = sendLen;
  ctx->need_zlp =
      (sendLen == CDC_EP_MAX_PKT && _ring_count(ctx) == 0) ? 1u : 0u;
  ctx->coalesce_ticks = 0;
  USBD_CDC_SetTxBuffer(ch, &hUsbDevice, ctx->pkt_buf, sendLen);
  USBD_CDC_TransmitPacket(ch, &hUsbDevice);
}

/* USER CODE BEGIN Additional_User_Code */
/* USER CODE END Additional_User_Code */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
