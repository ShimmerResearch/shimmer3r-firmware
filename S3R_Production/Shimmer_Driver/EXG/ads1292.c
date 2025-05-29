/*
 * Copyright (c) 2013, Shimmer Research, Ltd.
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of Shimmer Research, Ltd. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *    * You may not use or distribute this Software or any derivative works
 *      in any form for commercial purposes with the exception of commercial
 *      purposes when used in conjunction with Shimmer products purchased
 *      from Shimmer or their designated agent or with permission from
 *      Shimmer.
 *      Examples of commercial purposes would be running business
 *      operations, licensing, leasing, or selling the Software, or
 *      distributing the Software for use with commercial products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author Weibo Pan
 * @date July, 2016
 */

#include "ads1292.h"
#include "hal_Board.h"
#include <string.h>

void (*ADS1292_dataReadDone_cb)(void);

uint8_t *activeBuffer, ads1292_bbuf;
uint8_t chip1Buffer1[9], chip1Buffer2[9], chip2Buffer1[9], chip2Buffer2[9];
volatile uint8_t chip1CurrentFullBuffer, chip2CurrentFullBuffer;
uint8_t rxCount, chip1ReadPending, chip2ReadPending, chip2Enabled, chipBusy;
SPI_HandleTypeDef *hspi_exg;
uint8_t dummy_tx_buf[9] = {
  0,
};

void ADS1292_init()
{
  Board_EXG_RESET_N(1);
  HAL_Delay(1000); //Datasheet states to wait 1s for power-on reset
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  activeBuffer = chip1Buffer1;
  chip1ReadPending = 0;
  chip2ReadPending = 0;
  chip1CurrentFullBuffer = 0;
  chip2CurrentFullBuffer = 0;
  rxCount = 0;
  chip2Enabled = 0;
  chipBusy = 0;
}

void setSpiHandle(SPI_HandleTypeDef *hspi)
{
  hspi_exg = hspi;
}

void ADS1292_enableChip2(uint8_t en)
{
  chip2Enabled = en;
}

HAL_StatusTypeDef ADS1292_Tx1Byte(uint8_t data)
{
  HAL_StatusTypeDef res = HAL_OK;
  //one byte write needs 7.8us in total
  uint8_t tx_buf[] = { data };
  res = HAL_SPI_Transmit(hspi_exg, tx_buf, 1, 100);
  return res;
}

HAL_StatusTypeDef ADS1292_Rx1Byte(uint8_t *buf)
{
  HAL_StatusTypeDef res = HAL_OK;
  res = HAL_SPI_Receive(hspi_exg, buf, 1, 100);
  return res;
}

HAL_StatusTypeDef ADS1292_regRead(uint8_t startaddress, uint8_t size, uint8_t *rdata)
{
  HAL_StatusTypeDef res = HAL_OK;

  uint8_t tx_buf[2];
  tx_buf[0] = startaddress | RREG;
  tx_buf[1] = size - 1;

  res = ADS1292_Tx1Byte(tx_buf[0]);
  if (res != HAL_OK)
  {
    return res;
  }
  res = ADS1292_Tx1Byte(tx_buf[1]);
  if (res != HAL_OK)
  {
    return res;
  }
  while (size--)
  {
    res = ADS1292_Rx1Byte(rdata++);
    if (res != HAL_OK)
    {
      return res;
    }
  }
  return res;
}

HAL_StatusTypeDef ADS1292_regWrite(uint8_t startaddress, uint8_t size, uint8_t *wdata)
{
  HAL_StatusTypeDef res = HAL_OK;

  uint8_t tx_buf[2];
  tx_buf[0] = startaddress | WREG;
  tx_buf[1] = size - 1;

  res = ADS1292_Tx1Byte(tx_buf[0]);
  if (res != HAL_OK)
  {
    return res;
  }
  res = ADS1292_Tx1Byte(tx_buf[1]);
  if (res != HAL_OK)
  {
    return res;
  }

  while (size--)
  {
    res = ADS1292_Tx1Byte(*(wdata++));
    if (res != HAL_OK)
    {
      return res;
    }
  }
  return res;
}

void ADS1292_powerOn(void)
{
  Board_EXG_RESET_N(1);
  //TODO flow chart in datasheet says to wait 1s after setting the reset pin high
  HAL_Delay(10);
}

void ADS1292_powerOff(void)
{
  Board_EXG_RESET_N(0);
  HAL_Delay(10);
}

//Issues a reset pulse
//Remains powered on afterwards
//Delays match the sample code from TI
void ADS1292_resetPulse(void)
{
  Board_EXG_RESET_N(1);
  HAL_Delay(1);
  Board_EXG_RESET_N(0);
  HAL_Delay(1);
  Board_EXG_RESET_N(1);
  HAL_Delay(7);
}

void ADS1292_chip1CsEnable(uint8_t enable)
{
  if (enable)
  {
    //Ensure chip 2 is not enabled
    if (HAL_GPIO_ReadPin(EXG_CHIP2_CS_GPIO_Port, EXG_CHIP2_CS_Pin) == GPIO_PIN_RESET)
    {
      //Disable chip 2
      Board_delayMicros(6); //wait 5.875us (assuming 24MHz clock)
                            //i.e. 3tCLKs (5.86us)
      Board_EXG_CHIP2_CS(1);
    }
    Board_EXG_CHIP1_CS(0);
    //need to wait 10ns here, 2 clk cycles @ 216MHz
    while (0)
      ;
  }
  else
  {
    Board_delayMicros(6); //wait 5.875us (assuming 24MHz clock)
                          //i.e. 3tCLKs (5.86us)
                          //Board_ECG_CS(1);
    Board_EXG_CHIP1_CS(1);
  }
}

void ADS1292_chip2CsEnable(uint8_t enable)
{
  if (enable)
  {
    //Ensure chip 1 is not enabled
    if (HAL_GPIO_ReadPin(EXG_CHIP1_CS_GPIO_Port, EXG_CHIP1_CS_Pin) == GPIO_PIN_RESET)
    {
      //Disable chip 2
      Board_delayMicros(6); //wait 5.875us (assuming 24MHz clock)
                            //i.e. 3tCLKs (5.86us)
      Board_EXG_CHIP1_CS(1);
    }
    Board_EXG_CHIP2_CS(0);
    //need to wait 10ns here, 2 clk cycles @ 216MHz
    while (0)
      ;
  }
  else
  {
    Board_delayMicros(6); //wait 5.875us  (assuming 24MHz clock)
                          //i.e. 3tCLKs (5.86us)
    Board_EXG_CHIP2_CS(1);
  }
}

HAL_StatusTypeDef ADS1292_readDataContinuousMode(uint8_t enable)
{
  uint8_t tx_buf;
  HAL_StatusTypeDef res = HAL_OK;
  if (enable)
  {
    tx_buf = RDATAC;
  }
  else
  {
    tx_buf = SDATAC;
  }
  res = ADS1292_Tx1Byte(tx_buf);
  return res;
}

HAL_StatusTypeDef ADS1292_start(uint8_t start)
{
  uint8_t tx_buf;
  HAL_StatusTypeDef res = HAL_OK;
  if (start)
  {
    tx_buf = START;
  }
  else
  {
    tx_buf = STOP;
  }
  res = ADS1292_Tx1Byte(tx_buf);
  return res;
}

HAL_StatusTypeDef ADS1292_resetRegs(void)
{
  uint8_t tx_buf = RESET;
  HAL_StatusTypeDef res = HAL_OK;
  res = ADS1292_Tx1Byte(tx_buf);
  Board_delayMicros(65); //wait 65us
  return res;
}

HAL_StatusTypeDef ADS1292_offsetCal(void)
{
  uint8_t tx_buf = OFFSETCAL;
  HAL_StatusTypeDef res = HAL_OK;
  res = ADS1292_Tx1Byte(tx_buf);
  return res;
}

HAL_StatusTypeDef ADS1292_enableInternalReference(void)
{
  uint8_t data[] = { 0x88 };
  HAL_StatusTypeDef res = HAL_OK;
  res = ADS1292_regWrite(ADS1x9x_REG_CONFIG2, 1, data);
  HAL_Delay(100);
  return res;
}

void ADS1292_enableDrdyInterrupts(uint8_t mask)
{
  if (mask & ADS1292_DRDY_INT_CHIP1)
  {
    HAL_NVIC_EnableIRQ(INT_LINE_GPIO_INTERNAL1);
  }
  if (mask & ADS1292_DRDY_INT_CHIP2)
  {
    HAL_NVIC_EnableIRQ(INT_LINE_GPIO_INTERNAL0);
  }
}

void ADS1292_disableDrdyInterrupts(uint8_t mask)
{
  if (mask & ADS1292_DRDY_INT_CHIP1)
  {
    HAL_NVIC_DisableIRQ(INT_LINE_GPIO_INTERNAL1);
  }
  if (mask & ADS1292_DRDY_INT_CHIP2)
  {
    HAL_NVIC_DisableIRQ(INT_LINE_GPIO_INTERNAL0);
  }
}

uint8_t ADS1292_readDataChip1(uint8_t *data)
{
  if (chip1CurrentFullBuffer == 1)
  {
    memcpy(data, chip1Buffer1, 9);
  }
  else if (chip1CurrentFullBuffer == 2)
  {
    memcpy(data, chip1Buffer2, 9);
  }
  else
  {
    return 0;
  }
  return 1;
}

uint8_t ADS1292_readDataChip2(uint8_t *data)
{
  if (chip2CurrentFullBuffer == 1)
  {
    memcpy(data, chip2Buffer1, 9);
  }
  else if (chip2CurrentFullBuffer == 2)
  {
    memcpy(data, chip2Buffer2, 9);
  }
  else
  {
    return 0;
  }
  return 1;
}

//Tell the driver that the data is ready to be read from chipX
void ADS1292_dataReadyChip1(void)
{
  if (chip2ReadPending)
  {
    chip1ReadPending = 1;
  }
  else
  {
    if (!chip1ReadPending)
    {
      chip1ReadPending = 1;
      if (chip1CurrentFullBuffer == 1)
      {
        activeBuffer = chip1Buffer2;
      }
      else
      {
        activeBuffer = chip1Buffer1;
      }

      ADS1292_dataReadFromChip1(activeBuffer);
    }
  }
}

void ADS1292_dataReadyChip2(void)
{
  if (chip1ReadPending)
  {
    chip2ReadPending = 1;
  }
  else
  {
    if (!chip2ReadPending)
    {
      chip2ReadPending = 1;
      if (chip2CurrentFullBuffer == 1)
      {
        activeBuffer = chip2Buffer2;
      }
      else
      {
        activeBuffer = chip2Buffer1;
      }
      ADS1292_dataReadFromChip2(activeBuffer);
    }
  }
}

void ADS1292_readDataComplete(void)
{
  if ((activeBuffer == chip1Buffer1) || (activeBuffer == chip1Buffer2))
  {
    chip1ReadPending = 0;
    if (activeBuffer == chip1Buffer1)
    {
      chip1CurrentFullBuffer = 1;
    }
    else
    {
      chip1CurrentFullBuffer = 2;
    }
    if (chip2ReadPending)
    {
      chip2ReadPending = 0;
      ADS1292_dataReadyChip2();
    }
  }
  else
  {
    chip2ReadPending = 0;
    if (activeBuffer == chip2Buffer1)
    {
      chip2CurrentFullBuffer = 1;
    }
    else
    {
      chip2CurrentFullBuffer = 2;
    }
    if (chip1ReadPending)
    {
      chip1ReadPending = 0;
      ADS1292_dataReadyChip1();
    }
  }
}

void ADS1292_dataReadFromChip1(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  ADS1292_chip1CsEnable(1);
#if IS_EXG_DATA_READ_BLOCKING
  ret = HAL_SPI_TransmitReceive(
      hspi_exg, dummy_tx_buf, buf, ADS1292_DATA_PACKET_LENGTH, 1000);
  ADS1292_chip1CsEnable(0);
  ADS1292_readDataComplete();
#else
  ret = HAL_SPI_TransmitReceive_DMA(hspi_exg, dummy_tx_buf, buf, ADS1292_DATA_PACKET_LENGTH);
#endif
}

void ADS1292_dataReadFromChip2(uint8_t *buf)
{
  HAL_StatusTypeDef ret;
  ADS1292_chip2CsEnable(1);
#if IS_EXG_DATA_READ_BLOCKING
  ret = HAL_SPI_TransmitReceive(
      hspi_exg, dummy_tx_buf, buf, ADS1292_DATA_PACKET_LENGTH, 1000);
  ADS1292_chip2CsEnable(0);
  ADS1292_readDataComplete();
#else
  ret = HAL_SPI_TransmitReceive_DMA(hspi_exg, dummy_tx_buf, buf, ADS1292_DATA_PACKET_LENGTH);
#endif
}

void ADS1292_gatherDataInit(void (*done_cb)(void))
{
  ADS1292_dataReadDone_cb = done_cb;
}
