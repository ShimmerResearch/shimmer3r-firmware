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

#ifndef HAL_ADS7028_H_
#define HAL_ADS7028_H_

//****************************************************************************
//
// Insert processor specific header file(s) here
//
//****************************************************************************

/*  --- INSERT YOUR CODE HERE --- */
#include <assert.h>
//#include "ti/devices/msp432e4/driverlib/driverlib.h"
#include "hal_FactoryTest.h"
#include "stdbool.h"
#include "stdint.h"
#include <ADS7028/ads7028.h>
#include "spi.h"

//*****************************************************************************
//
// Macros
//
//*****************************************************************************
/** Alias used for setting GPIOs pins to the logic "high" state */
#define HIGH            ((bool) true)

/** Alias used for setting GPIOs pins to the logic "low" state */
#define LOW             ((bool) false)

/* SPI Peripheral Macros */

/* MCU System clock frequency */
#define SYSTEM_CLOCK_HZ ((uint32_t) 120000000)

//*****************************************************************************
//
// Function Prototypes
//
//*****************************************************************************

self_test_result_t ads7028_self_test(void);
void ads7028GetTestData(uint16_t *buff);
/* SPI peripheral functions */
void initAdcPeripherals(void);
void spiSendReceiveArray(void *handle, uint8_t *DataTx, uint8_t *DataRx, uint8_t byteLength);
uint8_t spiSendReceiveByte(const uint8_t dataTx);

/* GPIO functions */
void setAds7028CS(const bool state);
bool getAds7028CS(void); /*  Used for testing only */

#endif /* HAL_H_ */
