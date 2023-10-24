/*
 * CYW20820.c
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#include "stm32u5xx.h"

#include "CYW20820.h"

#include "EZ-Serial/ezsapi.h"
#include "EZ-Serial/handlers.h"

/* convenience functions for pretty-printing binary data as zero-padded hexadecimal */
#define printHex8(VARIABLE)     printHex((uint8_t *)&VARIABLE, 1, 1, 0)
#define printHex16(VARIABLE)    printHex((uint8_t *)&VARIABLE, 2, 1, 0)
#define printHex32(VARIABLE)    printHex((uint8_t *)&VARIABLE, 4, 1, 0)
#define printHexMac(VARIABLE)   printHex((uint8_t *)&VARIABLE, 6, 1, ':')

static uint8_t nameAry[] = {14, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', 'r', '-', 'X', 'X', 'X', 'X'};

uint8_t btSetCommandsStep;
uint8_t btSetCommandsStart;

void btInit(void)
{
  btSetCommandsStart = 1;
  btSetCommandsStep = 1;

  /* packet pointer for working with response/event data */
//  ezs_packet_t *packet;

  printf("\r\nEZ-Serial API communication demo started\r\n");

  /* initialize EZ-Serial interface and callbacks */
  EZSerial_Init(appHandler, appOutput, appInput);

  btSetCommands();

//  if ((packet = EZS_SEND_AND_WAIT(ezs_cmd_system_query_firmware_version(), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) == 0)
//  {
//	  /* "system_ping" response packet not received */
//	  printf("FW request timed out, check communication settings and reset module\r\n");
//  }
//  else
//  {
//	  /* "system_ping" response packet received */
//	  printf("FW request successful\r\n");
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
//	 /*"system_ping" response packet received*/
//	printf("Device name set successful\r\n");
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

void btSetCommands(void)
{
  if(btSetCommandsStep == 1)
  {
    btSetCommandsStep++;
	HAL_StatusTypeDef status = setDmaRx(13);
    ezs_cmd_system_ping();
    return;
  }

  if(btSetCommandsStep == 2)
  {
    btSetCommandsStep++;
	HAL_StatusTypeDef status = setDmaRx(5);
    ezs_cmd_system_query_firmware_version();
    return;
  }

  if(btSetCommandsStep == 3)
  {
    btSetCommandsStep++;
	HAL_StatusTypeDef status = setDmaRx(5);
    ezs_cmd_system_get_bluetooth_address();
    return;
  }

  if(btSetCommandsStep == 4)
  {
    btSetCommandsStep++;
	HAL_StatusTypeDef status = setDmaRx(5);
    ezs_cmd_gap_get_device_name();
    return;
  }

  if(btSetCommandsStep == 5)
  {
    btSetCommandsStep++;
	HAL_StatusTypeDef status = setDmaRx(1);
    ezs_cmd_gap_set_device_name(1U, &nameAry[0]);
    return;
  }

  if(btSetCommands == 6)
  {
    btSetCommandsStart = 0;
    return;
  }

}

void ezsHandler(ezs_packet_t *packet)
{

}

void ezsHandlerShimmer(ezs_packet_t *packet)
{
    switch (packet->tbl_index)
    {
        case EZS_IDX_RSP_SYSTEM_PING:
//            printf("RX: rsp_system_ping: result=");
//            printHex16(packet->payload.rsp_system_ping.result);
//            printf(", runtime=");
//            printHex32(packet->payload.rsp_system_ping.runtime);
//            printf(", fraction=");
//            printHex16(packet->payload.rsp_system_ping.fraction);

            if(btSetCommandsStart)
            {
                btSetCommands();
            }

            break;

        case EZS_IDX_RSP_SYSTEM_QUERY_FIRMWARE_VERSION:
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

            if(btSetCommandsStart)
            {
                btSetCommands();
            }

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
//            printf("RX: rsp_system_query_firmware_version: Address=");
//            printHexMac(packet->payload.rsp_system_get_bluetooth_address.address);

            if(btSetCommandsStart)
            {
                btSetCommands();
            }

            break;

        case EZS_IDX_RSP_GAP_SET_DEVICE_NAME:
//        	printf("RX: rsp_gap_set_device_name: Result=");
//        	printHex16(packet->payload.rsp_gap_set_device_name.result);

            if(btSetCommandsStart)
            {
                btSetCommands();
            }

        	break;
            /* Shimmer added end */

        case EZS_IDX_RSP_GAP_GET_DEVICE_NAME:
        	printf("RX: rsp_gap_get_device_name: name=");
        	printHexMac(packet->payload.rsp_gap_get_device_name.name);
        	break;

        case EZS_IDX_RSP_GAP_SET_DEVICE_APPEARANCE:
        	printf("RX: rsp_gap_set_device_appearance: Result=");
        	printHex16(packet->payload.rsp_gap_set_device_appearance.result);
        	break;
            /* Shimmer added end */


        default:
            printf("RX: unhandled packet: ");
            printHex8(packet->header.group);
            printf("/");
            printHex8(packet->header.id);
            break;
    }

    printf("\r\n");
}

void printHex(uint8_t *data, uint8_t bytes, uint8_t reverse, char separator)
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
