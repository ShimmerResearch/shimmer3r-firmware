/*
 * CYW20820.h
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#ifndef SRC_CYW20820_H_
#define SRC_CYW20820_H_

#include "stdbool.h"

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
