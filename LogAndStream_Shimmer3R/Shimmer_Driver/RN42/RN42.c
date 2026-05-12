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
 * @date May, 2016
 */

//#include "msp430.h"
#include "RN42.h"
#include "hal_Power.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h> /* strtoul */
#include <string.h>

//copied as directly as possible from TinyOS RovingNetworksP.nc
//http://code.google.com/p/tinyos-main/source/browse/trunk/tos/platforms/shimmer/chips/bluetooth/RovingNetworksP.nc

void setCommandsContinue(void);

static uint8_t deInit = 0;
volatile uint8_t setCommandsStep, setTempBaudRateStep;
UART_HandleTypeDef *huart_BT = 0;

uint8_t messageInProgress, transmissionOverflow;
uint8_t bt_txBuf[TX_BUFF_MAX], bt_txBuBuf[TX_BACKUP_BUFF_MAX]; //max possible size
uint8_t bt_rxBuf[MAX_COMMAND_ARG_SIZE];
uint16_t bt_txBufLen;

char expectedCommandResponse[12], newName[17], newAutoMaster[13], newPIN[17],
    newSvcClass[5], newDevClass[5], newSvcName[17], newRawBaudrate[5], newBaudrate[5],
    newInquiryTime[5], newPagingTime[5], newFriendlyName[16], rn42Mac[14];
char commandbuf[32];

uint8_t radioMode, charsSent, charsReceived;

uint8_t discoverable, authenticate, encrypt, setNameRequest, setPINRequest,
    resetDefaultsRequest, setSvcClassRequest, setDevClassRequest,
    setSvcNameRequest, setRawBaudrate, setBaudrate, disableRemoteConfig, newMode,
    setCustomInquiryTime, setCustomPagingTime, newAuthMode, setFriendlyNameRequest;

//master mode stuff
uint8_t btConnected, deviceConn;
char targetBt[16];

uint8_t (*dataAvailableFuncPtr)(uint8_t data) = 0;
uint8_t (*BtExtCb)(uint8_t *data) = 0;

uint8_t txIe;

uint8_t slowRate;

uint8_t btConfiguring = 0;

void initRN()
{
  GPIO_InitTypeDef GPIO_InitStruct;
  //powerup state is reset == low (true); mike conrad of roving networks sez:
  //wait about 1s to 2s after reset toggle
  HAL_GPIO_WritePin(BT_GPIO_FACTORY, BT_PIN_FACTORY, GPIO_PIN_RESET); //FACTORY
  HAL_GPIO_WritePin(BT_GPIO_CTS, BT_PIN_CTS, GPIO_PIN_RESET);         //CTS
  //RSTN PC1
  HAL_GPIO_WritePin(BT_GPIO_RSTN, BT_PIN_RSTN, GPIO_PIN_SET);   //RST_N
  HAL_GPIO_WritePin(BT_GPIO_POWER, BT_PIN_POWER, GPIO_PIN_SET); //POWER
  //wait 2s
  HAL_Delay(2000);

  //UART_RTS interrupt: RTS raises when BT has trans overflow
  //PIO2_CONNECT interrupt
  if (deInit)
  {
    /*Configure GPIO pins : BT_RTS_Pin BT_STAT_Pin */
    GPIO_InitStruct.Pin = BT_PIN_RTS | BT_PIN_STATUS; //RTS, STATUS
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BT_GPIO_RTS, &GPIO_InitStruct);
    deInit = 0;
  }

  //CTS PB4
  //toggling cts wakes it up
  HAL_GPIO_WritePin(BT_GPIO_CTS, BT_PIN_CTS, GPIO_PIN_SET); //CTS
  //wait 5ms (assuming 24MHz MCLK)
  HAL_Delay(5);
  HAL_GPIO_WritePin(BT_GPIO_CTS, BT_PIN_CTS, GPIO_PIN_RESET); //CTS
}

void BT_setupUART(char *baudRate)
{
  USART_TypeDef *temp_Instance = huart_BT->Instance;

  HAL_UART_DeInit(huart_BT);

  huart_BT->Instance = temp_Instance;
  if ((strlen(baudRate) == 4) && (strncmp(baudRate, "115K", 4)))
  {
    if (!strncmp(baudRate, "1200", 4))
    {
      huart_BT->Init.BaudRate = 1200;
    }
    else if (!strncmp(baudRate, "2400", 4))
    {
      huart_BT->Init.BaudRate = 2400;
    }
    else if (!strncmp(baudRate, "4800", 4))
    {
      huart_BT->Init.BaudRate = 4800;
    }
    else if (!strncmp(baudRate, "9600", 4))
    {
      huart_BT->Init.BaudRate = 9600;
    }
    else if (!strncmp(baudRate, "19.2", 4))
    {
      huart_BT->Init.BaudRate = 19200;
    }
    else if (!strncmp(baudRate, "38.4", 4))
    {
      huart_BT->Init.BaudRate = 38400;
    }
    else if (!strncmp(baudRate, "57.6", 4))
    {
      huart_BT->Init.BaudRate = 57600;
    }
    else if (!strncmp(baudRate, "230K", 4))
    {
      huart_BT->Init.BaudRate = 230400;
    }
    else if (!strncmp(baudRate, "460K", 4))
    {
      huart_BT->Init.BaudRate = 460800;
    }
    else if (!strncmp(baudRate, "921K", 4))
    {
      huart_BT->Init.BaudRate = 921600;
    }
  }
  else
  {
    huart_BT->Init.BaudRate = 115200;
  } //115200 default
  //huart->Init.BaudRate = 115200;
  huart_BT->Init.WordLength = UART_WORDLENGTH_8B;
  huart_BT->Init.StopBits = UART_STOPBITS_1;
  huart_BT->Init.Parity = UART_PARITY_NONE;
  huart_BT->Init.Mode = UART_MODE_TX_RX;
  huart_BT->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart_BT->Init.OverSampling = UART_OVERSAMPLING_16;
  huart_BT->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart_BT->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  HAL_UART_Init(huart_BT);
}

void disableRN(UART_HandleTypeDef *huart)
{
  //hold in reset
  //P4OUT &= ~BIT4;                        //Shimmer3 board
  HAL_GPIO_WritePin(BT_GPIO_RSTN, BT_PIN_RSTN, GPIO_PIN_RESET);

  HAL_UART_DeInit(huart);

  //P1IE &= ~BIT3;                         //disable RTS interrupt
  //P1IE &= ~BIT0;                         //disable Connection interrupt
  /*Configure GPIO pins : BT_RTS_Pin BT_STAT_Pin */
  HAL_GPIO_DeInit(BT_GPIO_RTS, BT_PIN_RTS);                       //RTS
  HAL_GPIO_DeInit(BT_GPIO_STATUS, BT_PIN_STATUS);                 //STATUS
  HAL_GPIO_WritePin(BT_GPIO_POWER, BT_PIN_POWER, GPIO_PIN_RESET); //POWER
  deInit = 1;
}

HAL_StatusTypeDef writeCommand(char *cmd, char *response)
{
  memcpy(bt_txBuf, cmd, strlen(cmd));
  strcpy(expectedCommandResponse, response);
  BT_setRxLen(strlen(response));
  return BT_write(bt_txBuf, strlen(cmd));
}

HAL_StatusTypeDef writeCommandSpecialRsp(char *cmd, char *response, uint8_t rsp_len)
{
  memcpy(bt_txBuf, cmd, strlen(cmd));
  strcpy(expectedCommandResponse, response);
  BT_setRxLen(rsp_len);
  return BT_write(bt_txBuf, strlen(cmd));
}

//Connect and Disconnect commands are exceptional commands in that
//they automatically return to data mode once they are issued
HAL_StatusTypeDef writeCommandNoRsp(char *cmd)
{
  return BT_write(bt_txBuf, strlen(cmd));
}

//void BT_txIsr(UART_HandleTypeDef *huart){
//   if(bt_txBufLen>0){
//      uint16_t len_to_tx = bt_txBufLen>TX_BUFF_MAX ? TX_BUFF_MAX:bt_txBufLen;
//      memcpy(bt_txBuf, bt_txBuBuf, len_to_tx);
//      HAL_UART_Transmit_DMA(huart_BT, bt_txBuf, len_to_tx);
//      memcpy(bt_txBuBuf, bt_txBuBuf+len_to_tx, bt_txBufLen - len_to_tx);
//      bt_txBufLen -= len_to_tx;
//   }
//}
void BT_txIsr(UART_HandleTypeDef *huart)
{
  PeriStat_Clr(STAT_PERI_BT);
}

void BT_rxIsr(UART_HandleTypeDef *huart)
{
  if (!*expectedCommandResponse && btConnected)
  {
    if (BtExtCb)
    { //ensure this has been set
      (*BtExtCb)(bt_rxBuf);
    }
  }
  else
  {
    if (setCommandsStep)
    {
      if (*expectedCommandResponse == 'M')
      {
        //Getting MAC address
        memcpy(rn42Mac, bt_rxBuf, 14);
        *expectedCommandResponse = '\0';
        setCommandsContinue();
      }
      else
      {
        if ((huart->RxXferSize == strlen(expectedCommandResponse))
            || (!memcmp(bt_rxBuf, expectedCommandResponse, huart->RxXferSize)))
        {
          *expectedCommandResponse = '\0';
          //continue
          setCommandsContinue();
        }
      }
    }
    else if (setTempBaudRateStep)
    {
      if ((huart->RxXferSize == strlen(expectedCommandResponse))
          || (!memcmp(bt_rxBuf, expectedCommandResponse, huart->RxXferSize)))
      {
        *expectedCommandResponse = '\0';
        //continue
        setTempBaudRateStep++;
      }
    }
  }
}

void BT_setRxLen(uint8_t len)
{
  HAL_UART_Receive_DMA(huart_BT, bt_rxBuf, len);
}

void setCommandsStart()
{
  setCommandsStep = 1;
  writeCommand("$$$", "CMD\r\n");
}

void setCommandsContinue(void)
{
  //reset factory defaults
  if (setCommandsStep == 1)
  {
    setCommandsStep++;
    if (resetDefaultsRequest)
    {
      writeCommand("SF,1\r", "AOK\r\n");
      return;
    }
  }

  //default is slave (== 0), otherwise set mode
  if (setCommandsStep == 2)
  {
    setCommandsStep++;
    if (newMode)
    {
      sprintf(commandbuf, "SM,%d\r", radioMode);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 3)
  {
    setCommandsStep++;
    writeCommandSpecialRsp("GB\r", "M", 14);
    return; //wait until response is received
  }

  if (setCommandsStep == 4)
  {
    setCommandsStep++;
    if (radioMode == AUTO_MASTER_MODE)
    {
      sprintf(commandbuf, "SR,%s\r", newAutoMaster);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 5)
  {
    setCommandsStep++;
    //device is discoverable with a non-zero inquiry scan window
    //default "time" is 0x0200 (units unspecified)
    if (!discoverable)
    {
      writeCommand("SI,0000\r", "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 6)
  {
    setCommandsStep++;
    //device default is off
    if (authenticate)
    {
      writeCommand("SA,1\r", "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 7)
  {
    setCommandsStep++;
    //device default is off
    if (encrypt)
    {
      writeCommand("SE,1\r", "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 8)
  {
    setCommandsStep++;
    //default is none
    if (setNameRequest)
    {
      sprintf(commandbuf, "SN,%s\r", newName);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 9)
  {
    setCommandsStep++;
    //default is none
    if (setFriendlyNameRequest)
    {
      sprintf(commandbuf, "S-,%s\r", newFriendlyName);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 10)
  {
    setCommandsStep++;
    //default is none
    if (setPINRequest)
    {
      sprintf(commandbuf, "SP,%s\r", newPIN);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 11)
  {
    setCommandsStep++;
    if (setSvcClassRequest)
    {
      sprintf(commandbuf, "SC,%s\r", newSvcClass);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 12)
  {
    setCommandsStep++;
    if (setDevClassRequest)
    {
      sprintf(commandbuf, "SD,%s\r", newDevClass);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 13)
  {
    setCommandsStep++;
    if (setSvcNameRequest)
    {
      sprintf(commandbuf, "SS,%s\r", newSvcName);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 14)
  {
    setCommandsStep++;
    if (setRawBaudrate)
    {
      //set the baudrate to suit the MSP430 running at 8Mhz
      sprintf(commandbuf, "SZ,%s\r", newRawBaudrate);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 15)
  {
    setCommandsStep++;
    if (disableRemoteConfig)
    {
      //disable remote configuration to enhance throughput
      writeCommand("ST,0\r", "AOK\r\n");
    }
    else
    {
      //disable remote configuration to enhance throughput
      writeCommand("ST,255\r", "AOK\r\n");
    }
    return;
  }

  if (setCommandsStep == 16)
  {
    setCommandsStep++;
    if (setCustomInquiryTime)
    {
      sprintf(commandbuf, "SI,%s\r", newInquiryTime);
      writeCommand(commandbuf, "AOK\r\n");
    }
    else
    {
      //to save power only leave inquiry on for approx 40msec (every 1.28 secs)
      writeCommand("SI,0040\r", "AOK\r\n");
    }
    return;
  }

  if (setCommandsStep == 17)
  {
    setCommandsStep++;
    if (setCustomPagingTime)
    {
      sprintf(commandbuf, "SJ,%s\r", newPagingTime);
      writeCommand(commandbuf, "AOK\r\n");
    }
    else
    {
      //to save power only leave paging on for approx 80msec (every 1.28 secs)
      writeCommand("SJ,0080\r", "AOK\r\n");
    }
    return;
  }

  if (setCommandsStep == 18)
  {
    setCommandsStep++;
    if (setBaudrate)
    {
      //set the baudrate to suit the MSP430
      sprintf(commandbuf, "SU,%s\r", newBaudrate);
      writeCommand(commandbuf, "AOK\r\n");
      return;
    }
  }

  if (setCommandsStep == 19)
  {
    setCommandsStep++;
    //exit command mode
    writeCommand("---\r", "END\r\n");
    return;
  }

  //arriving here =  all done perfectly
  if (setCommandsStep == 20)
  {
    //P1IE |= BIT0;        //enable interrupt
    setCommandsStep = 0;
    BT_setRxLen(1);
  }
}

//IMPORTANT: Connect and Disconnect commands are exceptional commands
//in that they automatically return to data mode once they are issued
//so no response and no "---" needed to return to data mode
/*void runMasterCommands() {
   saveInterruptSettings();

   writeCommand("$$$", "CMD\r\n");
   __bis_SR_register(LPM3_bits + GIE); //wait until response is received

   //Connect
   if(deviceConn && (!btConnected)){  //Connect
      sprintf(commandbuf, "C,%s\r", targetBt);
      writeCommandNoRsp(commandbuf);
   } else if((!deviceConn) && (btConnected)) { //Disconnect
      writeCommandNoRsp("K,\r");
   } else { //exit command mode
      //not needed for connect and disconnect commands
      writeCommand("---\r", "END\r\n");
      __bis_SR_register(LPM3_bits + GIE); //wait until response is received
      deviceConn = 0;
   }

   restoreInterruptSettings();
}*/

void BT_init(UART_HandleTypeDef *huart, uint8_t (*receiveFuncPtr)(uint8_t *data))
{

  //Turn on power (SW_BT P4.3 on Shimmer3)
  //P4OUT |= BIT3;

  bt_txBufLen = 0;
  setTempBaudRateStep = 0;
  setCommandsStep = 0;
  newMode = 0;
  radioMode = SLAVE_MODE;
  discoverable = 1;
  authenticate = 0;
  encrypt = 0;
  resetDefaultsRequest = 0;
  setNameRequest = 0;
  setFriendlyNameRequest = 0;
  setPINRequest = 0;
  setSvcClassRequest = 0;
  setSvcNameRequest = 0;
  setDevClassRequest = 0;
  setRawBaudrate = 0;
  disableRemoteConfig = 0;
  setCustomInquiryTime = 0;
  setCustomPagingTime = 0;
  setBaudrate = 0;
  newAuthMode = 0;
  txIe = 0;
  slowRate = 0;
  *rn42Mac = '\0';

  huart_BT = huart;
  BtExtCb = receiveFuncPtr;

  //connect/disconnect commands
  deviceConn = btConnected = 0;

  *expectedCommandResponse = '\0'; //NULL pointer
  charsReceived = 0;
  transmissionOverflow = messageInProgress = 0;

  initRN();

  //HAL_UART_Receive_DMA(huartDock, uartDockRxBuf, 1);

  btConfiguring = 0;
  BT_setupUART("115K");
}

void BT_configure()
{
  HAL_Delay(15);
  setCommandsStart();
  while (setCommandsStep)
  {
    //Power_SleepUntilInterrupt();
    HAL_Delay(100);
  }
}

void BT_disable(UART_HandleTypeDef *huart)
{
  disableRN(huart);
  //Turn off power (SW_BT P4.3 on Shimmer3)
  //P4OUT &= ~BIT3;
}

//write data to be transmitted to the Bluetooth module
//returns 0 if fails, else 1
//will only fail if a previous BT_write is still in progress
//HAL_StatusTypeDef BT_write(uint8_t *buf, uint8_t len) {
//   if((bt_txBufLen != 0)||
//      (huart_BT->gState == HAL_UART_STATE_BUSY_TX) ||
//      transmissionOverflow){
//      if(bt_txBufLen+len<TX_BACKUP_BUFF_MAX){
//         memcpy(bt_txBuBuf+bt_txBufLen, buf, len);
//         bt_txBufLen += len;
//      }
//      return HAL_BUSY;
//   }else if((huart_BT->gState == HAL_UART_STATE_READY) && !transmissionOverflow) {
//      memcpy(bt_txBuf, buf, len);
//      return HAL_UART_Transmit_DMA(huart_BT, bt_txBuf, len);
//   }else{
//      return HAL_BUSY;   //fail
//   }
//   return HAL_BUSY;
//}
HAL_StatusTypeDef BT_write(uint8_t *buf, uint8_t len)
{
  HAL_StatusTypeDef ret_val;
  memcpy(bt_txBuf, buf, len);
  ret_val = HAL_UART_Transmit_DMA(huart_BT, bt_txBuf, len);
  if (ret_val == HAL_OK)
  {
    PeriStat_Set(STAT_PERI_BT);
  }
  return ret_val;
}

/*
uint8_t BT_connect(uint8_t *addr) {
   deviceConn = 1;   //True
   strcpy(targetBt, (const char *)addr);
   runMasterCommands();
   return 1;         //success
}


uint8_t BT_disconnect() {
    //Delay: If any bytes are seen before or after $$$ in a 1
    //second window, command mode will not be entered and these
    //bytes will be passed on to other side
    __delay_cycles(288000000);   //wait 8s (assuming 24MHz MCLK)

    deviceConn = 0;
    runMasterCommands();
    return 1;
}
*/

void BT_setRadioMode(uint8_t mode)
{
  newMode = 1;
  radioMode = mode;
}

void BT_setAutoMaster(char *master)
{
  snprintf(newAutoMaster, 13, "%s", master);
}

void BT_setDiscoverable(uint8_t disc)
{
  discoverable = disc;
}

void BT_setEncryption(uint8_t enc)
{
  encrypt = enc;
}

void BT_setAuthentication(uint8_t mode)
{
  authenticate = 1;
  newAuthMode = mode;
}

void BT_setName(char *name)
{
  setNameRequest = 1;
  snprintf(newName, 17, "%s", name);
}

void BT_setFriendlyName(char *name)
{
  setFriendlyNameRequest = 1;
  snprintf(newFriendlyName, 16, "%s", name);
}

void BT_setPIN(char *PIN)
{
  setPINRequest = 1;
  snprintf(newPIN, 17, "%s", PIN);
}

void BT_setServiceClass(char *class)
{
  setSvcClassRequest = 1;
  snprintf(newSvcClass, 5, "%s", class);
}

void BT_setServiceName(char *name)
{
  setSvcNameRequest = 1;
  snprintf(newSvcName, 5, "%s", name);
}

void BT_setDeviceClass(char *class)
{
  setDevClassRequest = 1;
  snprintf(newDevClass, 5, "%s", class);
}

void BT_disableRemoteConfig(uint8_t disableConfig)
{
  disableRemoteConfig = disableConfig;
}

//this one makes sense only to roving networks
//the supplied "rate_factor" is the baudrate * 0.004096
//this factor must be an integer value...
void BT_setRawBaudrate(char *rate_factor)
{
  setRawBaudrate = 1;
  snprintf(newRawBaudrate, 5, "%s", rate_factor);
}

//to set the baudrate of the BT to MSP serial interface
//as per RovingNetworks command spec EG "SU,96" or "SU,230"
//SU,<rate> - Baudrate, {1200, 2400, 4800, 9600, 19.2,
//38.4, 57.6, 115K, 230K, 460K, 921K }
void BT_setBaudrate(char *new_baud)
{
  setBaudrate = 1;
  snprintf(newBaudrate, 5, "%s", new_baud);
}

//Sets the Paging Scan Window - amount of time device
//spends enabling page scan (connectability).
//Minimum = (hex word) "0012", corresponding to about 1% duty cycle.
//Maximum = (hex word) "1000"
void BT_setPagingTime(char *hexval_time)
{
  setCustomPagingTime = 1;
  snprintf(newPagingTime, 5, "%s", hexval_time);
}

//Sets the Inquiry Scan Window - amount of time device
//spends enabling inquiry scan (discoverability).
//Minimum = (hex word) "0012", corresponding to about 1% duty cycle.
//Maximum = (hex word) "1000"
void BT_setInquiryTime(char *hexval_time)
{
  setCustomInquiryTime = 1;
  snprintf(newInquiryTime, 5, "%s", hexval_time);
}

uint8_t BT_getRn42MacAddressPtr(uint8_t *macPtr)
{
  if (*rn42Mac)
  {
    macPtr = rn42Mac;
    return 1;
  }
  else
  {
    return 0;
  }
}

void BT_resetDefaults()
{
  resetDefaultsRequest = 1;
}

void BT_setTempBaudRate(char *baudRate)
{
  if ((strlen(baudRate) == 4)
      && (!strncmp(baudRate, "1200", 4) || !strncmp(baudRate, "2400", 4)
          || !strncmp(baudRate, "4800", 4) || !strncmp(baudRate, "9600", 4)
          || !strncmp(baudRate, "19.2", 4) || !strncmp(baudRate, "38.4", 4)
          || !strncmp(baudRate, "57.6", 4) || !strncmp(baudRate, "115K", 4)
          || !strncmp(baudRate, "230K", 4) || !strncmp(baudRate, "460K", 4)
          || !strncmp(baudRate, "921K", 4)))
  {

    sprintf(commandbuf, "U,%s,N\r", baudRate);

    //setTempBaudRateStart();
    setTempBaudRateStep = 1;
    writeCommand("$$$", "CMD\r\n");
    while (setTempBaudRateStep != 2)
    {
      Power_SleepUntilInterrupt();
    }
    //if(setCommandsStep == 2)
    if (slowRate)
    {
      writeCommandNoRsp(commandbuf);
      setTempBaudRateStep = 0;
      HAL_Delay(200);
    }
    else
    {
      writeCommand(commandbuf, "AOK\r\n");
    }
    while (setTempBaudRateStep != 3)
    {
      Power_SleepUntilInterrupt();
    }
    setTempBaudRateStep = 0;
    HAL_Delay(100);
    BT_setupUART(baudRate);
    BT_setRxLen(1);
    if (!strncmp(baudRate, "1200", 4) || !strncmp(baudRate, "2400", 4))
    {
      slowRate = 1;
    }
    else
    {
      slowRate = 0;
    }
  }
}

void BT_connectionInterrupt(uint8_t value)
{
  btConnected = value;
}

void BT_rtsInterrupt(uint8_t value)
{
  transmissionOverflow = value;
  //if(transmissionOverflow) {
  //   //disable sending
  //   txIe = UCA1IE & UCTXIE;
  //   UCA1IE &= ~UCTXIE;
  //} else {
  //   //re-enable sending if appropriate
  //   if(txIe)
  //      UCA1IE |= UCTXIE;
  //}
}
