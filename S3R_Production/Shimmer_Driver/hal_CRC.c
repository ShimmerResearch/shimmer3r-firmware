/*
 * hal_CRC.c
 *
 *  Created on: 23 Sep 2014
 *      Author: WeiboP
 *  Updated on: 23 Apr 2024
 *      Author: Mark Nolan
 */
#include "hal_CRC.h"
#include "stm32u5xx.h"

CRC_HandleTypeDef *hcrcToUse;

uint32_t CRC_data(uint8_t *buf, uint8_t len){
  HAL_CRC_Calculate(hcrcToUse, (uint32_t *)buf, (uint32_t)len);

  /* In order to match what's done in other Shimmer3 microcontrollers, we need
   * to add a 0x00 padding byte if the length is an odd number. */
  if (len & 0x01)
  {
    /* Change CRC peripheral state */
    hcrcToUse->State = HAL_CRC_STATE_BUSY;

    *(__IO uint8_t*) (__IO void*) (&hcrcToUse->Instance->DR) = 0x00;

    /* Change CRC peripheral state */
    hcrcToUse->State = HAL_CRC_STATE_READY;
  }

  return hcrcToUse->Instance->DR;
}

void calculateCrcAndInsert(uint8_t crcMode, uint8_t *aryPtr, uint8_t len)
{
    uint32_t crc_value;
    if (crcMode != CRC_OFF)
    {
        crc_value = CRC_data(aryPtr, len);
        *(aryPtr + len++) = crc_value & 0xFF;

        if (crcMode == CRC_2BYTES_ENABLED)
        {
            *(aryPtr + len++) = (crc_value & 0xFF00) >> 8;
        }
    }
}

uint8_t checkCrc(uint8_t crcMode, uint8_t *aryPtr, uint8_t payloadLen)
{
    uint32_t crc_value_calc;
    if (crcMode != CRC_OFF)
    {
        crc_value_calc = CRC_data(aryPtr, payloadLen);

        if ((crc_value_calc & 0xFF) != *(aryPtr + payloadLen))
        {
            return 0;
        }

        if (crcMode == CRC_2BYTES_ENABLED && (((crc_value_calc >> 8) & 0xFF) != *(aryPtr + payloadLen + 1)))
        {
            return 0;
        }
    }

    return 1;
}

void setCrcHandleToUse(CRC_HandleTypeDef *hcrc)
{
  hcrcToUse = hcrc;
}

uint8_t testCrcDriver(void)
{
  uint8_t crcTestArray0Remainder[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00};
  calculateCrcAndInsert(CRC_2BYTES_ENABLED, crcTestArray0Remainder, 8);
  if(crcTestArray0Remainder[8] != 0xAA || crcTestArray0Remainder[9] != 0x48)
  {
    return 1;
  }
  uint8_t crcTestArray1Remainder[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00};
  calculateCrcAndInsert(CRC_2BYTES_ENABLED, crcTestArray1Remainder, 9);
  if(crcTestArray1Remainder[9] != 0x5D || crcTestArray1Remainder[10] != 0x2A)
  {
    return 1;
  }
  uint8_t crcTestArray2Remainder[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x00};
  calculateCrcAndInsert(CRC_2BYTES_ENABLED, crcTestArray2Remainder, 10);
  if(crcTestArray2Remainder[10] != 0x17 || crcTestArray2Remainder[11] != 0x8B)
  {
    return 1;
  }
  uint8_t crcTestArray3Remainder[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x00, 0x00};
  calculateCrcAndInsert(CRC_2BYTES_ENABLED, crcTestArray3Remainder, 11);
  if(crcTestArray3Remainder[11] != 0x4E || crcTestArray3Remainder[12] != 0x79)
  {
    return 1;
  }
  return 0;
}

