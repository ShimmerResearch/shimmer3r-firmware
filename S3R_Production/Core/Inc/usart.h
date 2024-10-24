/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    usart.h
 * @brief   This file contains all the function prototypes for
 *          the usart.c file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#include "shimmer_definitions.h"
#include <shimmer_include.h>

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */

  void setUartPeripheralPointers(void);

  void BtUart_Init(uint32_t baudRate, uint32_t hwFlowCtrl);
  void usartBtUpdate(uint32_t baudRate, uint32_t hwFlowCtrl);
  void btUart_Deint(void);
  uint8_t BtUart_isInitialised(void);

#if defined(SHIMMER4_SDK)
  void BtUart_init(void);
  uint8_t BtUart_callBack(uint8_t *data_buf);
  void BtUart_sendRsp(void);
  void BtUart_processCmd(void);
//HAL_StatusTypeDef BtUart_Transmit_DMA(UART_HandleTypeDef *huart, uint8_t
//*pData, uint16_t Size); uint8_t BtUart_isConnected(void);
#endif

  void DockUart_init(UART_HandleTypeDef *huart);
  void DockUart_deint(void);
  void DockUart_disable(void);
  void DockUart_enable(void);
  uint8_t DockUart_getStep(void);
  void DockUart_setStep(uint8_t val);

#if defined(SHIMMER4_SDK)
  void ExpUart_init(void);
  void ExpUart_rxCallback(uint8_t data);
  uint8_t ExpUart_TxIT(uint8_t *pData, uint16_t Size);
#endif

  uint8_t DockUart_isInitialised(void);
  uint8_t BtUart_connectIntCheck(void);
#if defined(SHIMMER4_SDK)
#define BtUart_rtsIntCheck() \
  BT_rtsInterrupt(HAL_GPIO_ReadPin(BT_RTS_GPIO_Port, BT_RTS_Pin))
#endif
  uint8_t DockUart_interruptCheck(void);

  void dockUartRxCallback(UART_HandleTypeDef *huart);
  void DockUart_writeBlocking(uint8_t *buf, uint8_t len);
  void DockUart_writeText(char *str);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

