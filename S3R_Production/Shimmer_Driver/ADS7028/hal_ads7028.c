/**
 * \copyright Copyright (C) 2019-2021 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "gpio.h"
#include <ADS7028/hal_ads7028.h>

//****************************************************************************
//
// Internal function prototypes
//
//****************************************************************************

#define FIXED_OUTPUT 0xA5A

static void initGPIO(void);
static void initSPI(void);
static void initTIMER(void);
void TIMER0IntHandler(void);

//****************************************************************************
//
// Function Definitions
//
//****************************************************************************

//TODO implement test here.
self_test_result_t ads7028_self_test(void)
{
  uint8_t* adcTestVal;
  self_test_result_t self_test_result = SELF_TEST_PASS;
  resetDevice();
  if (!initADS7028())
  {
    self_test_result = SELF_TEST_FAIL_CHIP_DETECTION;
  }
  else
  {
    setAds7028CS(LOW);
    //configure in manual mode for VBATT channel
    //Write registor setting selecting channels.
    //writeSingleRegister(CHANNEL_SEL_ADDRESS, CHANNEL_SEL_MANUAL_CHID_7);
    //Set all Channels as Analog Inputs.
    //writeSingleRegister(PIN_CFG_ADDRESS, PIN_CFG_PIN_CFG_CH7_ANALOG_INPUT);
    setRegisterBits(DATA_CFG_FIX_PAT_ENABLED, DATA_CFG_FIX_PAT_MASK); //Device outputs fixed code 0xA5A repetitively when reading ADC data.
    setAds7028CS(HIGH);
    if(!ads7028GetTestData(adcTestVal)== FIXED_OUTPUT)
    {
      self_test_result = SELF_TEST_FAIL_SIGNAL_ISSUE;
    }
  }
  return self_test_result;
}

void ads7028GetTestData(uint8_t *buff)
{
  setAds7028CS(LOW);
  //setRegisterBits(SEQUENCE_CFG_SEQ_START_ENABLED, SEQUENCE_CFG_SEQ_START_MASK);
  *buff = readData(buff);
  setAds7028CS(HIGH);
}

//*****************************************************************************
//
//!\brief  Sends SPI byte array on MOSI pin and captures MISO data to a byte array.
//!
//!\fn void spiSendReceiveArray(const uint8_t dataTx[], uint8_t dataRx[], const uint8_t byteLength)
//!
//!\param dataTx[] byte array of SPI data to send on MOSI.
//!
//!\param dataRx[] byte array of SPI data captured on MISO.
//!
//!\param byteLength number of bytes to send & receive.
//!
//!NOTE: Make sure 'dataTx[]' and 'dataRx[]' contain at least as many bytes of data,
//!as indicated by 'byteLength'.
//!
//!\return None.
//
//*****************************************************************************
void spiSendReceiveArray(void *handle, uint8_t *txBufp, uint8_t *rxBufp, uint8_t byteLength)
{
  /*  --- INSERT YOUR CODE HERE ---
   *
   *  This function should send and receive multiple bytes over the SPI.
   *
   *  A typical SPI send/receive sequence may look like the following:
   *  1) Make sure SPI receive buffer is empty
   *  2) Set the /CS pin low (if controlled by GPIO)
   *  3) Send command bytes to SPI transmit buffer
   *  4) Wait for SPI receive interrupt
   *  5) Retrieve data from SPI receive buffer
   *  6) Set the /CS pin high (if controlled by GPIO)
   */
  HAL_StatusTypeDef ret;

  //Send all dataTx[] bytes on MOSI, and capture all MISO bytes in dataRx[]

  ret = HAL_SPI_TransmitReceive_DMA(handle, txBufp, rxBufp, byteLength);
}

//*****************************************************************************
//
//!\brief  Sends SPI byte on MOSI pin and captures MISO return byte value.
//!
//!\fn uint8_t spiSendReceiveByte(const uint8_t dataTx)
//!
//!\param dataTx data byte to send on MOSI pin.
//!
//!NOTE: This function is called by spiSendReceiveArray(). If it is called
//!directly, then the /CS pin must also be directly controlled.
//!
//!\return Captured MISO response byte.
//
//*****************************************************************************

//*****************************************************************************
//
//!Initializes timer to interrupt at specified frequency
//!
//!\fn void startTimer(uint32_t timerFreqHz)
//!\param timerFreqHz interrupt frequency in units of Hz (or SPS)
//!
//!\return None.
//
//*****************************************************************************

//*****************************************************************************
//
//!Disables the timer
//!
//!\fn void stopTimer(void)
//!
//!\return None.
//
//*****************************************************************************

//*****************************************************************************
//
// The interrupt handler for the timer interrupt.
//
//*****************************************************************************

//****************************************************************************
//
// GPIO helper functions
//
//****************************************************************************

//*****************************************************************************
//
//!\brief  Reads that current state of the /CS GPIO pin.
//!
//!\fn bool getCS(void)
//!
//!\return boolean ('true' if /CS is high, 'false' if /CS is low).
//
//*****************************************************************************
bool getAds7028CS(void)
{
  /* --- INSERT YOUR CODE HERE --- */
  return HAL_GPIO_ReadPin(CS_ADS7028_GPIO_Port, CS_ADS7028_Pin);
}

//*****************************************************************************
//
//!\brief  Controls the state of the /CS GPIO pin.
//!
//!\fn void setCS(const bool state)
//!
//!\param state boolean indicating which state to set the /CS pin (0=low, 1=high)
//!
//!NOTE: The 'HIGH' and 'LOW' macros defined in hal.h can be passed to this
//!function for the 'state' parameter value.
//!
//!\return None.
//
//*****************************************************************************
void setAds7028CS(const bool state)
{
  /* --- INSERT YOUR CODE HERE --- */

  /*   // t(QUIET) delay
     if (state) { SysCtlDelay(2); }

     uint8_t value = (uint8_t) (state ? nCS_PIN : 0);
     MAP_GPIOPinWrite(nCS_PORT, nCS_PIN, value);

     // delay
     if (!state) { SysCtlDelay(2); }*/
  if (state)
  {
    HAL_GPIO_WritePin(CS_ADS7028_GPIO_Port, CS_ADS7028_Pin, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(CS_ADS7028_GPIO_Port, CS_ADS7028_Pin, GPIO_PIN_RESET);
  }
}
