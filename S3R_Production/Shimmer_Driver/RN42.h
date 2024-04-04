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
 * @date May, 2013
 */

#ifndef RN42_H
#define RN42_H

#include <stdint.h>
#include "stm32u5xx_hal.h"
#include "s4.h"

#if DEVICE_VER == 58 // Shimmer4SDK
#define BT_GPIO_FACTORY GPIOD 
#define BT_GPIO_STATUS  GPIOD
#define BT_GPIO_RSTN    GPIOD
#define BT_GPIO_CTS     GPIOD
#define BT_GPIO_RTS     GPIOD
#define BT_GPIO_POWER   GPIOF
#define BT_PIN_FACTORY  GPIO_PIN_10
#define BT_PIN_STATUS   GPIO_PIN_13
#define BT_PIN_RSTN     GPIO_PIN_14
#define BT_PIN_CTS      GPIO_PIN_11
#define BT_PIN_RTS      GPIO_PIN_12
#define BT_PIN_POWER    GPIO_PIN_2
#else
#define BT_GPIO_FACTORY GPIOA 
#define BT_GPIO_STATUS  GPIOB
#define BT_GPIO_RSTN    GPIOC
#define BT_GPIO_CTS     GPIOB
#define BT_GPIO_RTS     GPIOB
#define BT_GPIO_POWER   GPIOF
#define BT_PIN_FACTORY  GPIO_PIN_5
#define BT_PIN_STATUS   GPIO_PIN_13
#define BT_PIN_RSTN     GPIO_PIN_1
#define BT_PIN_CTS      GPIO_PIN_4
#define BT_PIN_RTS      GPIO_PIN_12
#define BT_PIN_POWER    GPIO_PIN_2
#endif

#define TX_BUFF_MAX        256
#define TX_BACKUP_BUFF_MAX 1024

enum {
   SLAVE_MODE,
   MASTER_MODE,
   TRIGGER_MASTER_MODE,
   AUTO_MASTER_MODE
};

void BT_init(UART_HandleTypeDef *huart, uint8_t (*receiveFuncPtr)(uint8_t* data));
void BT_configure(void);
void BT_disable(UART_HandleTypeDef *huart);

//write data to be transmitted to the Bluetooth module
//returns 0 if fails, else 1
//will only fail if a previous BT_write is still in progress

void BT_setRxLen(uint8_t len);   

HAL_StatusTypeDef BT_write(uint8_t *buf, uint8_t len) ;

//connect to a specific device that was previously discovered
uint8_t BT_connect(uint8_t *addr);

//after this command is called there will be no link to the connected device
uint8_t BT_disconnect(void);


//mode: SLAVE_MODE, MASTER_MODE, TRIGGER_MASTER_MODE, AUTO_MASTER_MODE
void BT_setRadioMode(uint8_t mode);

void BT_setDiscoverable(uint8_t disc);
void BT_setEncryption(uint8_t enc);
void BT_setAuthentication(uint8_t mode);
void BT_setName(char *name);             // max 16 chars
void BT_setFriendlyName(char *name);     //max 15 chars
void BT_setPIN(char *name);              // max 16 chars
void BT_setServiceClass(char *serviceClass); // max 4 chars (hex word)
void BT_setServiceName(char *name);         // max 16 chars
void BT_setDeviceClass(char *deviceClass);  // max 4 chars (hex word)
void BT_disableRemoteConfig(uint8_t disableConfig);

//rate_factor is baudrate * 0.004096, e.g. to set 115200, pass in "472"
void BT_setRawBaudrate(char *rateFactor);   // max 4 chars, must be integer

//provide one of the following as a string argument:
//1200, 2400, 4800, 9600, 19.2, 38.4, 57.6, 115K, 230K, 460K, 921K
//void BT_setBaudrate(char *newBaud);

//save power by minimising time Inquiry/Page scanning
//module reset necessary for changes to take effect
void BT_setPagingTime(char *hexvalTime);    // max 4 chars (hex word)
void BT_setInquiryTime(char *hexvalTime);   // max 4 chars (hex word)

//Set's the mac variable to be the RN42's mac address
//mac must be 12 bytes in length
//Returns 0  if mac address not valid
//Returns 1 if mac address valid
//Will only be valid *after* BT_Configure() is run
//BT_init() needs to be run before the return value
//can be trusted
uint8_t BT_getRn42MacAddressPtr(uint8_t *macPtr);

void BT_resetDefaults(void);

//set new baud rate. This change is effective immediately.
//This change is only temporary. Reverts to previously configured rate after reset.
//The string argument must be one of the following and EXACTLY 4 characters:
//1200, 2400, 4800, 9600, 19.2(K), 38.4(K), 57.6(K), 115K, 230K, 460K or 921K
//If any other value is used this function does nothing
void BT_setTempBaudRate(char * baudRate);

//pass in a pointer to the function that will get called when
//data arrives
//the passed in function returns a 1 if program execution should resume
//(i.e. clear LPM3 bits)
//otherwise return 0
void BT_receiveFunction(uint8_t (*receiveFuncPtr)(uint8_t data));


//this function needs to be called from within the BT_PIO ISR
//in order to inform the RN42 driver about the state change
//value needs to be 1 if interrupt was low to high else 0
void BT_connectionInterrupt(uint8_t value);

//this function needs to be called from within the BT_RTS ISR
//in order to inform the RN42 driver about the state change
//value needs to be 1 if interrupt was low to high else 0
void BT_rtsInterrupt(uint8_t value);

// put these two in the system ISR handler
void BT_txIsr(UART_HandleTypeDef *huart);
void BT_rxIsr(UART_HandleTypeDef *huart);
   


#endif //RN42_H
