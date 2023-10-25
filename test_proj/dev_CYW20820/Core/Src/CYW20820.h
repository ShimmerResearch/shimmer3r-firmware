/*
 * CYW20820.h
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#ifndef SRC_CYW20820_H_
#define SRC_CYW20820_H_

#include "stdbool.h"

//TODO decide if needed and remove it and associated code if not
#define USE_GET_SET_ADV_PARAM 0

enum BT_SET_COMMAND_STAGES
{
  WAIT_FOR_BOOT,
  PING,
  GET_FIRMWARE_VERSION,
  GET_BLUETOOTH_ADDRESS,
  UPDATE_LOCAL_ADVERTISING_NAMES,
  GET_DEVICE_NAME_BT,
  SET_DEVICE_NAME_BT,
  GET_DEVICE_NAME_BLE,
  SET_DEVICE_NAME_BLE,
#if USE_GET_SET_ADV_PARAM
  GET_ADVERTISING_PARAMETERS,
  SET_ADVERTISING_PARAMETERS,
#endif
  START_BLE_ADVERTISING,
  FINISH
};

enum BLUETOOTH_DEVICE_TYPE
{
  DEVICE_TYPE_BLE,
  DEVICE_TYPE_BT
};

void btInit(void);
void btSetCommands(void);
bool isBtIsInitialised(void);
void setBtConnectionState(bool state);
void setBtCysppState(bool state);

#endif /* SRC_CYW20820_H_ */
