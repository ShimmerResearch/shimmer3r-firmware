/*
 * hal_CRC.h
 *
 *  Created on: 23 Sep 2014
 *      Author: WeiboP
 */

#ifndef HAL_CRC_H_
#define HAL_CRC_H_

#include "stm32u5xx.h"
#include <stdint.h>

#define CRC_INIT 0xB0CA

//CRC modes - the ordinal corresponds to the number of CRC bytes
typedef enum
{
  CRC_OFF,
  CRC_1BYTES_ENABLED,
  CRC_2BYTES_ENABLED,
  CRC_MAX_SUPPORTED_BYTES
} COMMS_CRC_MODE;

uint32_t CRC_data(uint8_t *buf, uint8_t len);
void calculateCrcAndInsert(uint8_t crcMode, uint8_t *aryPtr, uint8_t len);
uint8_t checkCrc(uint8_t crcMode, uint8_t *aryPtr, uint8_t payloadLen);

void setCrcHandleToUse(CRC_HandleTypeDef *hcrc);

uint8_t testCrcDriver(void);
#endif /* HAL_CRC_H_ */
