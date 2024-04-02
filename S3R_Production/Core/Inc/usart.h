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

#include "s4.h"
#include "s4__cfg.h"

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */

void Uart_init(void);

void usartBtUpdate(uint32_t baudRate, uint32_t hwFlowCtrl);

#if !IS_SHIMMER3R
void BtUart_init(void);
uint8_t BtUart_callBack(uint8_t* data_buf);
void BtUart_sendRsp(void);
void BtUart_processCmd(void);
//HAL_StatusTypeDef BtUart_Transmit_DMA(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
//uint8_t BtUart_isConnected(void);
#endif

void DockUart_init(void);
void DockUart_disable(void);
void DockUart_enable(void);
void DockUart_rxCallback(uint8_t data);
void DockUart_processCmd(void);
void DockUart_sendRsp(void);
uint8_t DockUart_getStep(void);
void DockUart_setStep(uint8_t val);

#if !IS_SHIMMER3R
void ExpUart_init(void);
void ExpUart_rxCallback(uint8_t data);
uint8_t ExpUart_TxIT(uint8_t *pData, uint16_t Size);
#endif

#if !IS_SHIMMER3R
uint8_t  BtUart_connectIntCheck(void);
#define  BtUart_rtsIntCheck() BT_rtsInterrupt(HAL_GPIO_ReadPin(BT_RTS_GPIO_Port, BT_RTS_Pin))
#endif
uint8_t  DockUart_interruptCheck(void);
void     DockUart_setup(void);

void dockUartRxCallback(UART_HandleTypeDef *huart);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

