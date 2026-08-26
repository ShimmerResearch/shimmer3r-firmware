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
/* Enabled so the boot sequence runs gap_set_adv_parameters: its flags bit 1 is
 * what makes the module use the user-defined advertisement/scan-response data
 * set below (guide 002-37528, gap_set_adv_data: "the data here remains unused"
 * without it). */
#define USE_GET_SET_ADV_PARAM          1
/* Enabled for the DEV-573 sniff/power experiments (2026-08-25): the module
 * boots at sleep level 1 and these steps force level 0 (sleep disabled,
 * flash-scoped, write guarded by a read-compare so it costs one flash write
 * ever). Transparent-mode throughput shows a sniff/power-nap signature
 * (~250 B per ~0.6 s) despite SBTP F=1, so module power management is the
 * prime suspect. */
#define USE_GET_SET_SYSTEM_SLEEP_PARAM 1

#define BAUD_TO_USE                    2000000L
#define FLOW_CONTROL                   1

//TODO this doesn't seem to work on v1.4.16.16 firmware, need to investigate further
/* 0 = non-transparent command mode (SPPM,M=3): every chunk is one SPP_SEND
 * command/response round trip - measured lossless 56 KB/s, radio-bound, on
 * v1.4.18.18 (bench 2026-08-25). THE SHIPPING DEFAULT.
 *
 * 1 = transparent SPP bridge (SPPM,M=1). Bench-tested on v1.4.18.18 and
 * PARKED: the module exits SPP data mode almost immediately after engaging
 * it (BT_CYSPP low then high within moments of a connection, never
 * re-entering) and the link degrades to ~0.4 KB/s with module-side 0x0207
 * command timeouts. Host->MCU still works throughout via EVT_SPP_DATA
 * events. The demux/TX-gating infrastructure for this mode (RX routed by the
 * CYSPP pin, raw TX refused during command-mode windows, connection state
 * from in-band events - BT_CONNECTION latches high and is diagnostic-only)
 * is in place and correct as far as it could be verified; what is missing is
 * Ezurio guidance on how GA3 transparent mode + the CYSPP pin are intended
 * to be driven, since v16-style behaviour (bridge stays up for the life of
 * the connection, 100 KB/s) does not happen on GA3. */
#define TRANSPARANT_MODE               1

#define ENABLE_BT_RX_DEBUG_PRINTS      0
#define ENABLE_BT_TX_DEBUG_PRINTS      0

#if SUPPORT_SR48_6_0
#define BAUD_TO_USE_SR48_6_0 115200L
#endif //SUPPORT_SR48_6_0

#define BT_DEVICE_CLASS_SPP 0x1F00

enum BT_SET_COMMAND_STAGES
{
  IDLE,
  WAIT_FOR_BOOT_STAGE1,
  WAIT_FOR_BOOT_STAGE2,
  ENTER_BINARY_MODE,
  UPDATE_UART_SETTINGS_STAGE1,
  UPDATE_UART_SETTINGS_STAGE2,
  UPDATE_UART_SETTINGS_STAGE3,
  UPDATE_UART_SETTINGS_STAGE4,
  UPDATE_UART_SETTINGS_STAGE5,
  PING,
  GET_BT_PARAMETERS,
  STOP_BT_ADVERTISING,
  STOP_BLE_ADVERTISING_STAGE1,
  STOP_BLE_ADVERTISING_STAGE2,
  GET_FIRMWARE_VERSION,
  GET_BT_DEVICE_CLASS,
  SET_BT_DEVICE_CLASS,
  /* GET_BT_MAC_ID to SET_DEVICE_NAME_BLE should be grouped together */
  GET_BT_MAC_ID,
  UPDATE_LOCAL_ADVERTISING_NAMES,
  GET_DEVICE_NAME_BT,
  SET_DEVICE_NAME_BT,
  GET_DEVICE_NAME_BLE,
  SET_DEVICE_NAME_BLE,
  SET_BLE_ADV_DATA,
  SET_BLE_SR_DATA,
  GET_TX_POWER,
  SET_TX_POWER,
#if USE_GET_SET_SYSTEM_SLEEP_PARAM
  GET_SYSTEM_SLEEP_PARAMETERS,
  SET_SYSTEM_SLEEP_PARAMETERS,
#endif
  GET_SMP_PRIVACY_MODE,
  SET_SMP_PRIVACY_MODE,
#if USE_GET_SET_ADV_PARAM
  GET_ADVERTISING_PARAMETERS,
  SET_ADVERTISING_PARAMETERS,
#endif
  GET_CONN_PARAMETERS,
  SET_CONN_PARAMETERS,
  GET_SECURITY_PARAMETERS,
  SET_SECURITY_PARAMETERS,
  GET_PIN_CODE,
  SET_PIN_CODE,
  SET_CYSPP_PACKETIZATION,
  START_BLE_ADVERTISING_STAGE1,
  START_BLE_ADVERTISING_STAGE2,
  START_BT_ADVERTISING,
  FACTORY_RESET,
  FR_WAIT_FOR_REBOOT_AFTER_FR,
  FR_UPDATE_UART,
  FR_PING,
  FR_RESET_BT_MAC_ID,
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
void btDeinit(void);
void btInitCommands(void);
void btFactoryResetCommands(void);
void progressToNextBtInCmd(void);
void setExpectedResponse(uint16_t idx);
bool isBtIsFactoryResetted(void);
bool isBtInitCmdsRunning(void);
bool isBtFactoryResetCmdsRunning(void);
void setBtCysppState(bool state);
bool getBtCysppState(void);
uint8_t *BT_getCyw20820MacAddressPtr(void);
void BT_generateCyw20820FirmwareVersionStr(char *str);
uint8_t BT_isFirmwareVersionAtLeast(uint8_t major, uint8_t minor, uint8_t patch);
void setBtConnectionState(bool state);
//connect to a specific device that was previously discovered
uint8_t BT_connect(uint8_t *addr);
//after this command is called there will be no link to the connected device
uint8_t BT_disconnect(void);
void BT_cancelConnection(void);
void BT_connectionFailed(uint8_t conn_handle, uint16_t reason);
void BT_startDone_cb(void (*callback)(void));
void BT_setConnectionHandle(uint8_t conn_handle);
uint8_t BT_getConnectionHandle(void);

#endif /* SRC_CYW20820_H_ */
