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

/**
 * @brief  Compute CRC over a buffer using the configured CRC peripheral.
 *
 * This function calculates a CRC over the provided buffer using the CRC
 * handle previously set via setCrcHandleToUse(). To maintain compatibility
 * with other Shimmer3 microcontrollers, a 0x00 padding byte is added when
 * the buffer length is an odd number of bytes.
 *
 * @param buf Pointer to the input buffer containing data bytes to be
 *            processed by the CRC peripheral.
 * @param len Length of the input buffer in bytes.
 *
 * @return The computed CRC value read from the CRC peripheral data register.
 */
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
