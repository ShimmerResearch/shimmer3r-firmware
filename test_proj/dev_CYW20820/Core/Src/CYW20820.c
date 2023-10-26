/*
 * CYW20820.c
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#include "stm32u5xx.h"

#include "CYW20820.h"

#include "string.h"

#include "EZ-Serial/ezsapi.h"
#include "EZ-Serial/handlers.h"

/* convenience functions for pretty-printing binary data as zero-padded hexadecimal */
#define printHex8(VARIABLE)     printHex((uint8_t *)&VARIABLE, 1, 1, 0)
#define printHex16(VARIABLE)    printHex((uint8_t *)&VARIABLE, 2, 1, 0)
#define printHex32(VARIABLE)    printHex((uint8_t *)&VARIABLE, 4, 1, 0)
#define printHexMac(VARIABLE)   printHex((uint8_t *)&VARIABLE, 6, 1, ':')

ezs_rsp_system_ping_t rsp_system_ping;
ezs_rsp_system_query_firmware_version_t rsp_system_query_firmware_version;
ezs_rsp_system_get_bluetooth_address_t rsp_system_get_bluetooth_address;
ezs_rsp_gap_get_device_name_t rsp_gap_get_device_name_bt;
ezs_rsp_gap_get_device_name_t rsp_gap_get_device_name_ble;
ezs_rsp_system_get_tx_power_t rsp_system_get_tx_power;
#if USE_GET_SET_ADV_PARAM
ezs_rsp_gap_get_adv_parameters_t rsp_gap_get_adv_parameters;
#endif
ezs_rsp_system_get_uart_parameters_t rsp_system_get_uart_parameters;

#define TX_POWER 0x08 // tx_power max BR: 12dBm,  EBR: 12dBm, BLE: 10dBm.

/* power array of size  3*8: User guide says only need when tx_power = 0 hence set to 0
 BR:{-2,0,2,4,6,8,10,12},
 EDR:{-2,0,2,4,6,8,10,12},
 BLE:{-2,0,2,4,6,8,10,10}, */
#define POWER_ARRAY 0x00

static char advNameBt[] = {17, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', 'r', '-', 'X', 'X', 'X', 'X', '-', 'B', 'T'};
static char advNameBle[] = {18, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', 'r', '-', 'X', 'X', 'X', 'X', '-', 'B', 'L', 'E'};

uint8_t btSetCommandsStart, btSetCommandsStep;
uint8_t btNameTypeBeingRead;
bool btIsInitialised, btConnected, btCysppState;

static char hexdigit2int(uint8_t xd)
{
  if (xd <= 9)
    return xd + '0';
  if (xd == 10)
    return 'A';
  if (xd == 11)
    return 'B';
  if (xd == 12)
    return 'C';
  if (xd == 13)
    return 'D';
  if (xd == 14)
    return 'E';
  if (xd == 15)
    return 'F';
  return '0';
}

static void printHex(uint8_t *data, uint8_t bytes, uint8_t reverse, char separator)
{
    if (reverse) data += bytes;
    while (bytes)
    {
        if (reverse) data--;
//        printf(((*data >> 4) & 0xF) < 10 ? ('0' + ((*data >> 4) & 0xF)) : ('A' - 10 + ((*data >> 4) & 0xF)));
//        printf(( *data       & 0xF) < 10 ? ('0' + ( *data       & 0xF)) : ('A' - 10 + ( *data       & 0xF)));

        printf("%c", ((*data >> 4) & 0xF) < 10 ? ('0' + ((*data >> 4) & 0xF)) : ('A' - 10 + ((*data >> 4) & 0xF)));
        printf("%c", ( *data       & 0xF) < 10 ? ('0' + ( *data       & 0xF)) : ('A' - 10 + ( *data       & 0xF)));

        if (!reverse) data++;
        bytes--;
        if (bytes && separator) printf("%c", separator);
    }
}

void btInit(void)
{
  btSetCommandsStart = 1;
  btSetCommandsStep = PING;
  btIsInitialised = false;

  /* packet pointer for working with response/event data */
//  ezs_packet_t *packet;

//  printf("\r\nEZ-Serial API communication demo started\r\n");

  /* initialize EZ-Serial interface and callbacks */
  EZSerial_Init(appHandler, appOutput, appInput);

  HAL_StatusTypeDef status = setDmaRx(1);

  btSetCommands();

//  if ((packet = EZS_SEND_AND_WAIT(ezs_cmd_system_query_firmware_version(), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) == 0)
//  {
//    /* "system_ping" response packet not received */
//    printf("FW request timed out, check communication settings and reset module\r\n");
//  }
//  else
//  {
//    /* "system_ping" response packet received */
//    printf("FW request successful\r\n");
//  }
//
//  if ((packet = EZS_SEND_AND_WAIT(ezs_cmd_system_get_bluetooth_address(), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) != 0)
//  {
//    /* "system_ping" response packet received */
//    printf("BT address request successful\r\n");
//  }
//
//if ((packet = EZS_SEND_AND_WAIT(ezs_cmd_gap_set_device_name(1U, &nameAry[0]), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) != 0)
//{
//   /*"system_ping" response packet received*/
//  printf("Device name set successful\r\n");
//}

/*
  uint8_t nameAry[] = {14, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', 'r', '-', '7', '4', '6', '2'};
  if ((packet = EZS_SEND_AND_WAIT(ezs_fcmd_gap_set_device_name(1U, &nameAry[0]), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) != 0)
  {
    printf("Device name set successful\r\n");
  }

  if ((packet = EZS_SEND_AND_WAIT(ezs_fcmd_gap_set_device_name(0U, &nameAry[0]), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) != 0)
  {
    printf("Device name set successful\r\n");
  }

  if ((packet = EZS_SEND_AND_WAIT(ezs_fcmd_gap_get_device_name(1U), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) != 0)
  {
    printf("Device name get successful\r\n");
  }

  if ((packet = EZS_SEND_AND_WAIT(ezs_fcmd_gap_set_device_appearance(0x0540), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) != 0)
  {
    printf("Device Appearance set successful\r\n");
  }
*/

}
//TODO set appropriate values for setDmaRx() calls
void btSetCommands(void)
{
  if (btSetCommandsStep == PING)
  {
    btSetCommandsStep++;
    ezs_cmd_system_ping();
    return;
  }

  if (btSetCommandsStep == GET_FIRMWARE_VERSION)
  {
    btSetCommandsStep++;
    ezs_cmd_system_query_firmware_version();
    return;
  }

  if (btSetCommandsStep == GET_BLUETOOTH_ADDRESS)
  {
    btSetCommandsStep++;
    ezs_cmd_system_get_bluetooth_address();
    return;
  }

  if (btSetCommandsStep == UPDATE_LOCAL_ADVERTISING_NAMES)
  {
    btSetCommandsStep++;
    advNameBt[11] = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[4] >> 4) & 0x0F);
    advNameBt[12] = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[4]) & 0x0F);
    advNameBt[13] = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[5] >> 4) & 0x0F);
    advNameBt[14] = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[5]) & 0x0F);

    advNameBle[11] = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[4] >> 4) & 0x0F);
    advNameBle[12] = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[4]) & 0x0F);
    advNameBle[13] = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[5] >> 4) & 0x0F);
    advNameBle[14] = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[5]) & 0x0F);
  }

  if (btSetCommandsStep == GET_DEVICE_NAME_BT)
  {
    btSetCommandsStep++;
    btNameTypeBeingRead = DEVICE_TYPE_BT;
    ezs_cmd_gap_get_device_name(DEVICE_TYPE_BT);
    return;
  }

  if (btSetCommandsStep == SET_DEVICE_NAME_BT)
  {
    btSetCommandsStep++;
    if (!strstr((char*) &rsp_gap_get_device_name_bt.name.data[0], &advNameBt[1]))
    {
      ezs_fcmd_gap_set_device_name(DEVICE_TYPE_BT, &advNameBt[0]);
      return;
    }
  }

  if (btSetCommandsStep == GET_DEVICE_NAME_BLE)
  {
    btSetCommandsStep++;
    btNameTypeBeingRead = DEVICE_TYPE_BLE;
    ezs_cmd_gap_get_device_name(DEVICE_TYPE_BLE);
    return;
  }

  //TODO BLE advertising name won't update on-the-fly, need to either stop adv before name change and then start again or use F cmd and reset module.
  if (btSetCommandsStep == SET_DEVICE_NAME_BLE)
  {
    btSetCommandsStep++;
    if (!strstr((char*) &rsp_gap_get_device_name_ble.name.data[0], &advNameBle[1]))
    {
      ezs_fcmd_gap_set_device_name(DEVICE_TYPE_BLE, &advNameBle[0]);
      return;
    }
  }

  if(btSetCommandsStep == GET_TX_POWER)
  {
    btSetCommandsStep++;
    ezs_fcmd_system_get_tx_power();
    return;
  }

  if(btSetCommandsStep == SET_TX_POWER)
  {
    btSetCommandsStep++;
   // uint8_t txPower = 0x08;
		if (rsp_system_get_tx_power.power != TX_POWER)
		{
			ezs_fcmd_system_set_tx_power(TX_POWER, POWER_ARRAY);
		}
    return;
  }

#if USE_GET_SET_ADV_PARAM
  if(btSetCommandsStep == GET_ADVERTISING_PARAMETERS)
  {
    btSetCommandsStep++;
    ezs_cmd_gap_get_adv_parameters();
    return;
  }

  if(btSetCommandsStep == SET_ADVERTISING_PARAMETERS)
  {
    btSetCommandsStep++;

    //TODO decide whether we want to write in flash and then check in future whether we need to set on boot.

    //TODO temp here, original settings read from module
    ezs_fcmd_gap_set_adv_parameters(2U, 3U, 7U,
           (uint16_t)48U, (uint16_t)30U, (uint16_t)2048U, (uint16_t)60U,
           0U,
      rsp_gap_get_adv_parameters.direct_addr, 0U);

//    /* Advertising interval in units of 0.625 ms. 50 = 31.24ms, infinite advertising. */
//    ezs_fcmd_gap_set_adv_parameters(2U, 3U, 7U,
//            (uint16_t)50U, (uint16_t)0U, (uint16_t)50U, (uint16_t)0U,
//            0U,
//      rsp_gap_get_adv_parameters.direct_addr, 0U);

    return;
  }
#endif

  if(btSetCommandsStep == UPDATE_UART_SETTINGS_STAGE1)
  {
    btSetCommandsStep++;
    ezs_cmd_system_get_uart_parameters(UART_TYPE_PUART);
    return;
  }

  if(btSetCommandsStep == UPDATE_UART_SETTINGS_STAGE2)
  {
    btSetCommandsStep++;
    ezs_cmd_system_set_uart_parameters(
        BAUD_TO_USE,
        rsp_system_get_uart_parameters.autobaud,
        rsp_system_get_uart_parameters.autocorrect,
        rsp_system_get_uart_parameters.flow,
        rsp_system_get_uart_parameters.databits,
        rsp_system_get_uart_parameters.parity,
        rsp_system_get_uart_parameters.stopbits,
        UART_TYPE_PUART);
    return;
  }

  if (btSetCommandsStep == UPDATE_UART_SETTINGS_STAGE3)
  {
    btSetCommandsStep++;
    //TODO resolve reference
    usart2UartUpdate();

    ezs_cmd_system_ping();
  }

  if (btSetCommandsStep == START_BLE_ADVERTISING)
  {
    btSetCommandsStep++;

    /* Advertising interval in units of 0.625 ms. 50 = 31.24ms, infinite advertising. */
    ezs_cmd_gap_start_adv(2U, 3U, 7U,
            50U, 0U, 50U, 0U,
            0U,
            0U, 0U);
  }

  if (btSetCommandsStep == FINISH)
  {
    btSetCommandsStart = 0;
    btIsInitialised = true;
    return;
  }

}

void ezsHandler(ezs_packet_t *packet)
{

}

void ezsHandlerShimmer(ezs_packet_t *packet)
{
  uint8_t rspParsed = 1;

    switch (packet->tbl_index)
    {
        case EZS_IDX_RSP_SYSTEM_PING:
            rsp_system_ping = packet->payload.rsp_system_ping;

//            printf("RX: rsp_system_ping: result=");
//            printHex16(packet->payload.rsp_system_ping.result);
//            printf(", runtime=");
//            printHex32(packet->payload.rsp_system_ping.runtime);
//            printf(", fraction=");
//            printHex16(packet->payload.rsp_system_ping.fraction);
            break;

        case EZS_IDX_RSP_SYSTEM_QUERY_FIRMWARE_VERSION:
          rsp_system_query_firmware_version = packet->payload.rsp_system_query_firmware_version;

//            printf("RX: rsp_system_query_firmware_version: app=");
//            printHex32(packet->payload.rsp_system_query_firmware_version.app);
//            printf(", stack=");
//            printHex32(packet->payload.rsp_system_query_firmware_version.stack);
//            printf(", protocol=");
//            printHex16(packet->payload.rsp_system_query_firmware_version.protocol);
//            printf(", hardware=");
//            printHex8(packet->payload.rsp_system_query_firmware_version.hardware);
//
//            /* check for protocol version older than v1.3 */
//            if (packet->payload.rsp_system_query_firmware_version.protocol < 0x0103)
//            {
//                printf("\r\n*** PLEASE UPDATE TARGET MODULE TO LATEST VERISON OF EZ-SERIAL FIRMWARE");
//            }
            break;

        case EZS_IDX_RSP_SYSTEM_REBOOT:
            printf("RX: rsp_system_reboot: result=");
            printHex16(packet->payload.rsp_system_ping.result);
            break;

        case EZS_IDX_EVT_SYSTEM_BOOT:
            printf("RX: evt_system_boot: app=");
            printHex32(packet->payload.evt_system_boot.app);
            printf(", stack=");
            printHex32(packet->payload.evt_system_boot.stack);
            printf(", protocol=");
            printHex16(packet->payload.evt_system_boot.protocol);
            printf(", hardware=");
            printHex8(packet->payload.evt_system_boot.hardware);
            printf(", cause=");
            printHex8(packet->payload.evt_system_boot.cause);
            printf(", address=");
            printHexMac(packet->payload.evt_system_boot.address);
            break;

        case EZS_IDX_EVT_GAP_ADV_STATE_CHANGED:
            printf("RX: evt_gap_adv_state_changed: state=");
            printHex8(packet->payload.evt_gap_adv_state_changed.state);
            printf(", reason=");
            printHex8(packet->payload.evt_gap_adv_state_changed.reason);
            break;

        case EZS_IDX_EVT_GAP_SCAN_STATE_CHANGED:
            printf("RX: evt_gap_scan_state_changed: state=");
            printHex8(packet->payload.evt_gap_scan_state_changed.state);
            printf(", reason=");
            printHex8(packet->payload.evt_gap_scan_state_changed.reason);
            break;

        case EZS_IDX_EVT_GAP_CONNECTED:
            printf("RX: evt_gap_connected: conn_handle=");
            printHex8(packet->payload.evt_gap_connected.conn_handle);
            printf(", address=");
            printHexMac(packet->payload.evt_gap_connected.address);
            printf(", type=");
            printHex8(packet->payload.evt_gap_connected.type);
            printf(", interval=");
            printHex16(packet->payload.evt_gap_connected.interval);
            printf(", slave_latency=");
            printHex16(packet->payload.evt_gap_connected.slave_latency);
            printf(", supervision_timeout=");
            printHex16(packet->payload.evt_gap_connected.supervision_timeout);
            printf(", bond=");
            printHex8(packet->payload.evt_gap_connected.bond);
            break;

        case EZS_IDX_EVT_GAP_DISCONNECTED:
            printf("RX: evt_gap_disconnected: conn_handle=");
            printHex8(packet->payload.evt_gap_disconnected.conn_handle);
            printf(", reason=");
            printHex16(packet->payload.evt_gap_disconnected.reason);
            break;

        case EZS_IDX_EVT_P_CYSPP_STATUS:
            printf("RX: evt_p_cyspp_status: status=");
            printHex8(packet->payload.evt_p_cyspp_status.status);
            break;


            /* Shimmer added start */
        case EZS_IDX_RSP_SYSTEM_GET_BLUETOOTH_ADDRESS:
          rsp_system_get_bluetooth_address = packet->payload.rsp_system_get_bluetooth_address;
//            printf("RX: rsp_system_query_firmware_version: Address=");
//            printHexMac(packet->payload.rsp_system_get_bluetooth_address.address);
            break;

        case EZS_IDX_RSP_GAP_SET_DEVICE_NAME:
//          printf("RX: rsp_gap_set_device_name: Result=");
//          printHex16(packet->payload.rsp_gap_set_device_name.result);
          break;

        case EZS_IDX_RSP_GAP_GET_DEVICE_NAME:

          if(btNameTypeBeingRead==DEVICE_TYPE_BLE)
          {
              rsp_gap_get_device_name_ble = packet->payload.rsp_gap_get_device_name;
          }
          else
          {
              rsp_gap_get_device_name_bt = packet->payload.rsp_gap_get_device_name;
          }

//          printf("RX: rsp_gap_get_device_name: name=");
//          printHexMac(packet->payload.rsp_gap_get_device_name.name);
          break;

        case EZS_IDX_RSP_SYSTEM_GET_TX_POWER:
        	rsp_system_get_tx_power = packet->payload.rsp_system_get_tx_power;
        	break;

        case EZS_IDX_RSP_SYSTEM_SET_TX_POWER:
        	break;

        case EZS_IDX_RSP_SYSTEM_GET_UART_PARAMETERS:
          rsp_system_get_uart_parameters = packet->payload.rsp_system_get_uart_parameters;
//          printf("RX: rsp_gap_set_device_appearance: Result=");
//          printHex16(packet->payload.rsp_gap_set_device_appearance.result);

          break;

        case EZS_IDX_RSP_SYSTEM_SET_UART_PARAMETERS:
//          printf("RX: rsp_gap_set_device_appearance: Result=");
//          printHex16(packet->payload.rsp_gap_set_device_appearance.result);
          break;

        case EZS_IDX_RSP_GAP_SET_DEVICE_APPEARANCE:
          printf("RX: rsp_gap_set_device_appearance: Result=");
          printHex16(packet->payload.rsp_gap_set_device_appearance.result);
          break;

        case EZS_IDX_RSP_GAP_SET_ADV_PARAMETERS:
          printHex16(packet->payload.rsp_gap_set_adv_parameters.result);
          break;

        case EZS_IDX_RSP_GAP_GET_ADV_PARAMETERS:
#if USE_GET_SET_ADV_PARAM
          rsp_gap_get_adv_parameters = packet->payload.rsp_gap_get_adv_parameters;
#endif
//          printHex16(packet->payload.rsp_gap_get_adv_parameters.result);
          break;

          /* Shimmer added end */

        default:
          rspParsed = 0;

          printf("RX: unhandled packet: ");
          printHex8(packet->header.group);
          printf("/");
          printHex8(packet->header.id);
          break;
    }

//    printf("\r\n");

    if(btSetCommandsStart && rspParsed)
    {
        btSetCommands();
    }
}

bool isBtIsInitialised(void)
{
  return btIsInitialised;
}

void setBtConnectionState(bool state)
{
  btConnected = state;
}

bool isBtConnected(void)
{
  return btConnected;
}

void setBtCysppState(bool state)
{
  btCysppState = state;
}

bool getBtCysppState(void)
{
  return btCysppState;
}

