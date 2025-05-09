/*
 * CYW20820.h
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#ifndef SRC_CYW20820_H_
#define SRC_CYW20820_H_

#include <stdbool.h>
#include <stdint.h>

#include "shimmer_definitions.h"

/* TODO decide if needed and remove it and associated code if not. Seems to be
 * needed or else BLE won't advertise just using ezs_cmd_gap_start_adv. */
#define USE_GET_SET_ADV_PARAM 0

#define BAUD_TO_USE           2000000L
#define FLOW_CONTROL          1

#if SUPPORT_SR48_6_0
#define BAUD_TO_USE_SR48_6_0 115200L
#endif //SUPPORT_SR48_6_0

enum BT_SET_COMMAND_STAGES
{
  WAIT_FOR_BOOT,
  UPDATE_UART_SETTINGS_STAGE1,
  UPDATE_UART_SETTINGS_STAGE2,
  UPDATE_UART_SETTINGS_STAGE3,
  UPDATE_UART_SETTINGS_STAGE4,
  UPDATE_UART_SETTINGS_STAGE5,
  PING,
  STOP_BLE_ADVERTISING_STAGE1,
  STOP_BLE_ADVERTISING_STAGE2,
  GET_FIRMWARE_VERSION,
  GET_BT_MAC_ID,
  UPDATE_LOCAL_ADVERTISING_NAMES,
  GET_DEVICE_NAME_BT,
  SET_DEVICE_NAME_BT,
  GET_DEVICE_NAME_BLE,
  SET_DEVICE_NAME_BLE,
  GET_TX_POWER,
  SET_TX_POWER,
  GET_SMP_PRIVACY_MODE,
  SET_SMP_PRIVACY_MODE,
#if USE_GET_SET_ADV_PARAM
  GET_ADVERTISING_PARAMETERS,
  SET_ADVERTISING_PARAMETERS,
#endif
  GET_CONN_PARAMETERS,
  SET_CONN_PARAMETERS,
  GET_SECURITY_PARAMETERS,
  START_BLE_ADVERTISING_STAGE1,
  START_BLE_ADVERTISING_STAGE2,
  FINISH
};

enum BT_FACTORY_RESET_COMMAND_STAGES
{
  FR_WAIT_FOR_BOOT,
  FR_GET_BT_MAC_ID,
  FACTORY_RESET,
  FR_WAIT_FOR_REBOOT_AFTER_FR,
  FR_UPDATE_UART,
  FR_PING,
  FR_RESET_BT_MAC_ID,
  FR_FINISH
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

void btInit(uint32_t baudRate, uint8_t factoryReset);
void btDeinit(void);
void btInitCommands(void);
void btFactoryResetCommands(void);
void progressToNextBtInCmd(void);
void setExpectedResponse(uint16_t idx);
bool isBtIsInitialised(void);
bool isBtIsFactoryResetted(void);
bool isBtInitCmdsRunning(void);
bool isBtFactoryResetCmdsRunning(void);
void setBtCysppState(bool state);
bool getBtCysppState(void);
uint8_t *BT_getCyw20820MacAddressPtr(void);
void BT_generateCyw20820FirmwareVersionStr(char *str);
void setBtConnectionState(bool state);
//connect to a specific device that was previously discovered
uint8_t BT_connect(uint8_t *addr);
//after this command is called there will be no link to the connected device
uint8_t BT_disconnect(void);
uint8_t BT_cancelConnection(void);
uint8_t BT_connectionFailed(uint8_t conn_handle, uint16_t reason);
#endif /* SRC_CYW20820_H_ */
