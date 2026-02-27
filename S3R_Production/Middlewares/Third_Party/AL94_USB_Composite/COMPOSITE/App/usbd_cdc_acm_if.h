/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_acm_if.h
  * @version        : v2.0_Cube
  * @brief          : Header for usbd_cdc_if.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CDC_ACM_IF_H__
#define __USBD_CDC_ACM_IF_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_acm.h"
#ifndef APP_RX_DATA_SIZE
#define APP_RX_DATA_SIZE 256
#endif
#ifndef APP_TX_DATA_SIZE
#define APP_TX_DATA_SIZE 256
#endif
#ifndef CDC_EP_MAX_PKT
#define CDC_EP_MAX_PKT 64u
#endif
#ifndef CDC_TX_RING_SIZE
#define CDC_TX_RING_SIZE 2048u
#endif
#ifndef USB_RX_RING_SIZE
#define USB_RX_RING_SIZE 2048u
#endif
/* USER CODE BEGIN INCLUDE */

 typedef struct
 {
   uint16_t usbRingHead;
   uint16_t usbRingTail;
   uint64_t usb_otg_hs_interrupt_received_count;
   uint64_t usb_otg_pcd_irq_handler_xr_received_count;
   uint64_t usb_otg_pcd_datout_recevied_count;
   uint64_t usb_cdc_data_out_received_count;
   uint64_t usb_interrupt_cdc_receive_entry_count;
   uint64_t usb_cdc_receive_rx_ring_full_count;
   uint64_t usb_pr_usb_process_cmd_task_set_count;
   uint64_t usb_pr_cmd_tasklist_entry_count;
   uint64_t usb_pr_cmd_fn_entry_count_Start1;
   uint64_t usb_pr_cmd_fn_entry_count_Start2;
   uint64_t usb_pr_cmd_clbck_count_end;
   uint64_t usb_pr_rx_clbck_entry_count_start;
   uint64_t usb_pr_shim_dockrx_clbck_dck_cmd_rsp_task_set_count;
   uint64_t usb_pr_shim_dockrx_clbck_dck_process_cmd_task_set_count;
   uint64_t usb_pr_shim_doc_rsp_tasklist_entry_count;
   uint64_t usb_pr_shim_doc_process_cmd_tasklist_entry_count;
   uint64_t usb_pr_shimdock_send_rsp_fn_entry_count;
   uint64_t usb_pr_Shimdock_processCmd_fn_entry_count;
   uint64_t usb_pr_shimdockPrcmd_set_shimdock_resp_count;
   uint64_t usb_pr_shimdock_rsp_cmd_fn_etnry_count;
   uint64_t usb_pr_shimdock_call_cdc_tx_fn_count;
   uint64_t usb_pr_cdc_transmit_fn_entry_count1;
   uint64_t usb_pr_cdc_transmit_fn_entry_count2;
   uint64_t usb_otg_pcd_irq_handler_datain_received_count;
   uint64_t usb_otg_pcd_datin_recevied_count;
   uint64_t usb_otg_ll_datain_stage_count;
   uint64_t usb_cdc_data_in_received_count;
   uint64_t usb_cdc_data_in_zlp_send_count;
   uint64_t usb_pr_cdc_transmit_cplt_fn_entry_count;
   uint64_t usb_pr_cdc_transmit_cplt_fn_error_count;
   uint8_t usbRingBuf[USB_RX_RING_SIZE];
 } UsbRxRingFifo_t;
 extern UsbRxRingFifo_t usbCmdRx;
/* USER CODE END INCLUDE */
 typedef enum
 {
   CDC_CH_DOCK_COMMS = 0,
   CDC_CH_BT_COMMS = 1
 } usb_cdc_comm_t;

 typedef enum {
   RING_TYPE_CDC_TX = 0,
   RING_TYPE_CDC_RX = 1
 } ring_type_t;

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief For Usb device.
  * @{
  */

/** @defgroup USBD_CDC_IF USBD_CDC_IF
  * @brief Usb VCP device module
  * @{
  */

/** @defgroup USBD_CDC_IF_Exported_Defines USBD_CDC_IF_Exported_Defines
  * @brief Defines.
  * @{
  */
/* USER CODE BEGIN EXPORTED_DEFINES */

/* USER CODE END EXPORTED_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Types USBD_CDC_IF_Exported_Types
  * @brief Types.
  * @{
  */

/* USER CODE BEGIN EXPORTED_TYPES */

/* USER CODE END EXPORTED_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Macros USBD_CDC_IF_Exported_Macros
  * @brief Aliases.
  * @{
  */

/* USER CODE BEGIN EXPORTED_MACRO */

/* USER CODE END EXPORTED_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Variables USBD_CDC_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

/** CDC Interface callback. */
extern USBD_CDC_ACM_ItfTypeDef  USBD_CDC_ACM_fops;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_FunctionsPrototype USBD_CDC_IF_Exported_FunctionsPrototype
  * @brief Public functions declaration.
  * @{
  */

uint8_t CDC_Transmit(uint8_t ch, uint8_t* Buf, uint16_t Len);
/* Added non-blocking transmit support helpers */
uint16_t CDC_TxFree(uint8_t ch,ring_type_t type);        /* free space remaining in TX ring */
uint16_t CDC_TxPending(uint8_t ch,ring_type_t type);     /* bytes queued but not yet sent */
void     CDC_Flush(uint8_t ch);         /* force immediate send of queued data */
void     CDC_FlushAll(void);            /* flush all CDC channels */
void     CDC_FlushTimerTick(uint8_t ch);/* call periodically (e.g. 1ms) to drive coalescing timer */
uint8_t ShimUsbProcessCmd(void);
void CDC_HardResetAll(void);
void cdc_setEnumerate(void);
/* USER CODE BEGIN EXPORTED_FUNCTIONS */

/* USER CODE END EXPORTED_FUNCTIONS */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_ACM_IF_TEMPLATE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
