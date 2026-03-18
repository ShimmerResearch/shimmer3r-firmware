/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    ux_device_cdc_acm.c
 * @author  MCD Application Team
 * @brief   USBX Device applicative file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ux_device_cdc_acm.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
usbx_cdc_acm_t usbx_cdc_tx_rx = { NULL, cdc_rx_buffer, 0, APP_RX_DATA_SIZE, 0, 0, UX_STATE_RESET,
  UX_STATE_RESET, usbx_success, usbx_success, 0, 1, 0, 1, 0, 0, NULL, 0 };
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
UX_SLAVE_CLASS_CDC_ACM *cdc_acm = NULL;
uint8_t cdc_tx_buffer[APP_TX_DATA_SIZE];
uint8_t cdc_rx_buffer[APP_RX_DATA_SIZE];
uint8_t cdc_command_buffer[RX_COMMAND_BUFFER_SIZE];

static volatile bool cdc_port_open = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  USBD_CDC_ACM_Activate
 *         This function is called when insertion of a CDC ACM device.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Activate */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  cdc_acm = (UX_SLAVE_CLASS_CDC_ACM *) cdc_acm_instance;
  cdc_port_open = false; /* host hasn't opened the serial port yet */
  /* USER CODE END USBD_CDC_ACM_Activate */

  return;
}

/**
 * @brief  USBD_CDC_ACM_Deactivate
 *         This function is called when extraction of a CDC ACM device.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_Deactivate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Deactivate */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  cdc_acm = UX_NULL;
  cdc_port_open = false;
  /* USER CODE END USBD_CDC_ACM_Deactivate */

  return;
}

/**
 * @brief  USBD_CDC_ACM_ParameterChange
 *         This function is invoked to manage the CDC ACM class requests.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_ParameterChange(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_ParameterChange */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);

  if (cdc_acm != UX_NULL)
  {
    /* The CDC ACM instance holds the latest control-line state.
     * The field name below is the typical USBX field; adjust if your
     * USBX version uses a different member name. DTR is bit 0. */
    uint16_t line_state = (uint16_t) cdc_acm->ux_slave_class_cdc_acm_data_dtr_state;
    cdc_port_open = (line_state & 0x01) ? true : false; /* DTR == bit0 */
  }
  /* USER CODE END USBD_CDC_ACM_ParameterChange */

  return;
}

/* USER CODE BEGIN 1 */
usbx_cdc_acm_result_t USBX_CDC_ACM_Transmit(uint8_t *buffer, uint16_t size)
{
  if (!cdc_acm || _ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED)
  {
    usbx_cdc_tx_rx.tx_result = usbx_error;
    return usbx_error;
  }

  if (buffer == NULL || size == 0)
  {
    usbx_cdc_tx_rx.tx_result = usbx_error;
    return usbx_error;
  }
  usbx_cdc_tx_rx.tx_buffer = buffer;
  usbx_cdc_tx_rx.tx_length = size;
  usbx_cdc_tx_rx.tx_count = 0;
  usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
  usbx_cdc_tx_rx.tx_scheduled = 1;
  usbx_cdc_tx_rx.tx_active = 1;
  usbx_cdc_tx_rx.tx_pending = 0;
  usbx_cdc_tx_rx.tx_result = usbx_success;
  //Immediately call the middleware engine to progress if possible
  while (usbx_cdc_tx_rx.tx_active)
  {
    cdc_acm_write_task();
  }
  return usbx_cdc_tx_rx.tx_result;
}

VOID cdc_acm_write_task(VOID)
{
  if (!usbx_cdc_tx_rx.tx_active)
  {
    return; //nothing to do
  }
  ULONG actual_length = 0;
  UINT retVal;
  switch (usbx_cdc_tx_rx.tx_engine_state)
  {
  case UX_STATE_RESET:
    if (usbx_cdc_tx_rx.tx_scheduled || usbx_cdc_tx_rx.tx_pending)
    {
      usbx_cdc_tx_rx.tx_engine_state = UX_STATE_WAIT;
    }
    else
    {
      usbx_cdc_tx_rx.tx_active = 0;
    }
    break;
  case UX_STATE_WAIT:
    if (usbx_cdc_tx_rx.tx_scheduled || usbx_cdc_tx_rx.tx_pending)
    {
      //Call USBX to transmit remaining bytes
      retVal = ux_device_class_cdc_acm_write_run(cdc_acm,
          usbx_cdc_tx_rx.tx_buffer + usbx_cdc_tx_rx.tx_count,
          usbx_cdc_tx_rx.tx_length - usbx_cdc_tx_rx.tx_count, &actual_length);
      //Handle error
      if (retVal < UX_STATE_IDLE)
      {
        //Fatal error: stop everything
        usbx_cdc_tx_rx.tx_result = usbx_error;
        usbx_cdc_tx_rx.tx_active = 0;
        usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
        usbx_cdc_tx_rx.tx_count = 0;
        usbx_cdc_tx_rx.tx_pending = 0;
        usbx_cdc_tx_rx.tx_scheduled = 0;
      }
      //Handle successful completion
      else if (retVal == UX_STATE_NEXT)
      {
        //Transmission fully completed
        usbx_cdc_tx_rx.tx_result = usbx_success;
        usbx_cdc_tx_rx.tx_active = 0;
        usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
        usbx_cdc_tx_rx.tx_count = 0;
        usbx_cdc_tx_rx.tx_pending = 0;
        usbx_cdc_tx_rx.tx_scheduled = 0;
      }
      else if (retVal == UX_STATE_WAIT)
      { //Partial transmission: update count and mark pending
        usbx_cdc_tx_rx.tx_count += actual_length;
        usbx_cdc_tx_rx.tx_pending = 1;
      }
      else
      {
        //Nothing to send, go idle
        usbx_cdc_tx_rx.tx_active = 0;
        usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
      }
    }
    break;
  default: //Unknown state; reset
    usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
    break;
  }
}

VOID cdc_acm_read_task(VOID)
{
  if (!cdc_acm || _ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED)
  {
    usbx_cdc_tx_rx.rx_result = usbx_error;
    return;
  }
  ULONG actual_length = 0;
  UINT status;
  switch (usbx_cdc_tx_rx.rx_engine_state)
  {
  case UX_STATE_RESET:
    if (usbx_cdc_tx_rx.rx_scheduled || usbx_cdc_tx_rx.rx_pending)
    {
      usbx_cdc_tx_rx.rx_engine_state = UX_STATE_WAIT;
    }
    break;
  case UX_STATE_WAIT:
    status = ux_device_class_cdc_acm_read_run(cdc_acm,
        usbx_cdc_tx_rx.rx_buffer + usbx_cdc_tx_rx.rx_count,
        usbx_cdc_tx_rx.rx_length - usbx_cdc_tx_rx.rx_count, &actual_length);
    usbx_cdc_tx_rx.rx_count += actual_length;
    if (status < UX_STATE_IDLE)
    {
      //Fatal error: stop everything
      usbx_cdc_tx_rx.rx_result = usbx_error;
      usbx_cdc_tx_rx.rx_count = 0;                     //discard current buffer
      usbx_cdc_tx_rx.rx_engine_state = UX_STATE_RESET; //reset RX state machine
      usbx_cdc_tx_rx.rx_pending = 0;
      usbx_cdc_tx_rx.rx_scheduled = 1; //keep engine active
    }
    else if (status == UX_STATE_NEXT)
    {
      //one full usb packet received
      usbx_cdc_tx_rx.rx_result = usbx_success;
      usbx_cdc_tx_rx.rx_engine_state = UX_STATE_RESET;
      //usbx_cdc_tx_rx.rx_count        += actual_length; // or process data here
      usbx_cdc_tx_rx.rx_pending = 0;
      usbx_cdc_tx_rx.rx_command_buffer = cdc_command_buffer;
      usbx_cdc_tx_rx.rx_command_length += usbx_cdc_tx_rx.rx_count;
      memcpy(usbx_cdc_tx_rx.rx_command_buffer, usbx_cdc_tx_rx.rx_buffer,
          usbx_cdc_tx_rx.rx_command_length);
      memset(usbx_cdc_tx_rx.rx_buffer, 0, usbx_cdc_tx_rx.rx_count);
      usbx_cdc_tx_rx.rx_count = 0;
      ShimTask_set(TASK_USB_PROCESS_CMD);
    }
    else if (status == UX_STATE_WAIT)
    {
      //Partial reception: mark pending
      usbx_cdc_tx_rx.rx_pending = 1;
    }
    //else still waiting, do nothing
    break;
  default: //Unknown state; reset
    usbx_cdc_tx_rx.rx_engine_state = UX_STATE_RESET;
    break;
  }
}

usbx_cdc_acm_result_t USBX_CDC_ACM_Receive(uint8_t *buffer, uint16_t size)
{
  for (uint32_t i = 0; i < size; i++)
  {
    ShimDock_rxCallback(buffer[i]);
  }
  return usbx_success;
}

/* Reports the COM port open state (configured + class active + DTR set) */
bool USBX_CDC_ACM_IsPortOpen(void)
{
  if (cdc_acm == UX_NULL)
  {
    return false;
  }
  if (_ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED)
  {
    return false;
  }
  return cdc_port_open;
}

/* USER CODE END 1 */
