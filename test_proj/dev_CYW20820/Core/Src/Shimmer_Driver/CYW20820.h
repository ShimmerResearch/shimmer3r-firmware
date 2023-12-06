/*
 * CYW20820.h
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#ifndef SRC_CYW20820_H_
#define SRC_CYW20820_H_

#include "stdbool.h"

//TODO decide if needed and remove it and associated code if not. Seems to be needed or else BLE won't advertise just using ezs_cmd_gap_start_adv.
#define USE_GET_SET_ADV_PARAM 1

#define BAUD_TO_USE 1000000L //2000000L //1000000L //115200L
#define FLOW_CONTROL 0

enum BT_SET_COMMAND_STAGES
{
  WAIT_FOR_BOOT,
  FACTORY_RESET,
  UPDATE_UART_SETTINGS_STAGE1,
  UPDATE_UART_SETTINGS_STAGE2,
  UPDATE_UART_SETTINGS_STAGE3,
  UPDATE_UART_SETTINGS_STAGE4,
  UPDATE_UART_SETTINGS_STAGE5,
  PING,
  STOP_BLE_ADVERTISING,
  GET_FIRMWARE_VERSION,
  GET_BLUETOOTH_ADDRESS,
  UPDATE_LOCAL_ADVERTISING_NAMES,
  GET_DEVICE_NAME_BT,
  SET_DEVICE_NAME_BT,
  GET_DEVICE_NAME_BLE,
  SET_DEVICE_NAME_BLE,
  GET_TX_POWER,
  SET_TX_POWER,
#if USE_GET_SET_ADV_PARAM
  GET_ADVERTISING_PARAMETERS,
  SET_ADVERTISING_PARAMETERS,
#endif
  GET_CONN_PARAMETERS,
  SET_CONN_PARAMETERS,
  START_BLE_ADVERTISING,
  FINISH
};

enum BLUETOOTH_DEVICE_TYPE
{
  DEVICE_TYPE_BLE,
  DEVICE_TYPE_BT
};

enum BLUETOOTH_UART_TYPE
{
  UART_TYPE_PUART,
  UART_TYPE_HCI_UART
};

void btInit(void);
void btSetCommands(void);
void setExpectedResponse(uint16_t idx);
bool isBtIsInitialised(void);
void setBtCysppState(bool state);
bool getBtCysppState(void);

#endif /* SRC_CYW20820_H_ */
