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

uint32_t platform_CrcData(uint8_t *buf, uint8_t len)
{
  HAL_CRC_Calculate(hcrcToUse, (uint32_t *) buf, (uint32_t) len);

  /* In order to match what's done in other Shimmer3 microcontrollers, we need
   * to add a 0x00 padding byte if the length is an odd number. */
  if (len & 0x01)
  {
    /* Change CRC peripheral state */
    hcrcToUse->State = HAL_CRC_STATE_BUSY;

    *(__IO uint8_t *) (__IO void *) (&hcrcToUse->Instance->DR) = 0x00;

    /* Change CRC peripheral state */
    hcrcToUse->State = HAL_CRC_STATE_READY;
  }

  return hcrcToUse->Instance->DR;
}

void setCrcHandleToUse(CRC_HandleTypeDef *hcrc)
{
  hcrcToUse = hcrc;
}
