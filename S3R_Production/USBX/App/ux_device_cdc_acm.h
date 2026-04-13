/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    ux_device_cdc_acm.h
 * @author  MCD Application Team
 * @brief   USBX Device CDC ACM interface header file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UX_DEVICE_CDC_ACM_H__
#define __UX_DEVICE_CDC_ACM_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"

  /* Private includes ----------------------------------------------------------*/
  /* USER CODE BEGIN Includes */
#include "Comms/shimmer_dock_usart.h"
#include "TaskList/shimmer_taskList.h"
#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"
  /* USER CODE END Includes */

  /* Exported types ------------------------------------------------------------*/
  /* USER CODE BEGIN ET */
#define APP_RX_DATA_SIZE       1024 * 4
#define APP_TX_DATA_SIZE       1024 * 4
#define RX_COMMAND_BUFFER_SIZE 512

  typedef struct usbx_cdc_acm
  {
    uint8_t *tx_buffer;
    uint8_t *rx_buffer;
    uint16_t tx_length;
    uint16_t rx_length;
    uint16_t tx_count;
    uint16_t rx_count;
    uint16_t tx_engine_state;
    uint16_t rx_engine_state;
    uint16_t tx_result;
    uint16_t rx_result;
    uint8_t tx_active;
    uint8_t rx_active;
    uint8_t tx_scheduled;
    uint8_t rx_scheduled;
    uint8_t tx_pending;
    uint8_t rx_pending;
    uint8_t *rx_command_buffer;
    uint8_t rx_command_length;
    volatile uint8_t rx_command_ready;
  } usbx_cdc_acm_t;

  typedef enum
  {
    usbx_success = 0,
    usbx_error = 1,
    usbx_busy = 2
  } usbx_cdc_acm_result_t;

  /* USER CODE END ET */

  /* Exported constants --------------------------------------------------------*/
  /* USER CODE BEGIN EC */
  extern UX_SLAVE_CLASS_CDC_ACM *cdc_acm;
  extern usbx_cdc_acm_t usbx_cdc_tx_rx;
  extern uint8_t cdc_tx_buffer[APP_TX_DATA_SIZE];
  extern uint8_t cdc_rx_buffer[APP_RX_DATA_SIZE];
  extern uint8_t user_command_buffer[RX_COMMAND_BUFFER_SIZE];
  /* USER CODE END EC */

  /* Exported macro ------------------------------------------------------------*/
  /* USER CODE BEGIN EM */

  /* USER CODE END EM */

  /* Exported functions prototypes ---------------------------------------------*/
  VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_instance);
  VOID USBD_CDC_ACM_Deactivate(VOID *cdc_acm_instance);
  VOID USBD_CDC_ACM_ParameterChange(VOID *cdc_acm_instance);

  /* USER CODE BEGIN EFP */

  usbx_cdc_acm_result_t USBX_CDC_ACM_Transmit(uint8_t *buffer, uint16_t size);
  VOID cdc_acm_write_task(VOID);
  VOID cdc_acm_read_task(VOID);
  usbx_cdc_acm_result_t USBX_CDC_ACM_Receive_To_RxBuf(VOID);
  usbx_cdc_acm_result_t USBX_CDC_ACM_Receive(uint8_t *buffer, uint16_t size);
  bool USBX_CDC_ACM_IsPortOpen(void);

  /* USER CODE END EFP */

  /* Private defines -----------------------------------------------------------*/
  /* USER CODE BEGIN PD */

  /* USER CODE END PD */

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /* __UX_DEVICE_CDC_ACM_H__ */
