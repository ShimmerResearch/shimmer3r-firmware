/*
 * CYW20820.c
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#include "stm32u5xx.h"

#include "CYW20820.h"

#include "stdio.h"
#include "string.h"

#include "EZ-Serial/ezsapi.h"
#include "EZ-Serial/handlers.h"

//TODO remove the need for this include
#include "main.h"
#include "usart.h"

/* convenience functions for pretty-printing binary data as zero-padded hexadecimal */
#define printHex8(VARIABLE)            printHex((uint8_t *) &VARIABLE, 1, 1, 0)
#define printHex16(VARIABLE)           printHex((uint8_t *) &VARIABLE, 2, 1, 0)
#define printHex32(VARIABLE)           printHex((uint8_t *) &VARIABLE, 4, 1, 0)
#define printHexMac(VARIABLE)          printHex((uint8_t *) &VARIABLE, 6, 1, ':')

#define ENABLE_BT_INIT_RX_DEBUG_PRINTS 0

/*
 * Index: {1,2,3,4,5,6,7,8}
 * BR:    {-2,0,2,4,6,8,10,12},
 * EDR:   {-2,0,2,4,6,8,10,12},
 * BLE:   {-2,0,2,4,6,8,10,10}, */
#define BT_TX_POWER                    8

uint8_t advNameMacIdStartIdx = 11;
static char advNameBt[] = { 17, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', 'r',
  '-', 'X', 'X', 'X', 'X', '-', 'B', 'T' };
static char advNameBle[] = { 18, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', 'r',
  '-', 'X', 'X', 'X', 'X', '-', 'B', 'L', 'E' };

//uint8_t advNameMacIdStartIdx = 10;
//static char advNameBt[] = {16, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', '-',
//'X', 'X', 'X', 'X', '-', 'B', 'T'}; static char advNameBle[] = {17, 'S', 'h',
//'i', 'm', 'm', 'e', 'r', '3', '-', 'X', 'X', 'X', 'X', '-', 'B', 'L', 'E'};

uint16_t expectedResponseIdx;

ezs_rsp_system_ping_t rsp_system_ping;
ezs_rsp_system_query_firmware_version_t rsp_system_query_firmware_version;
ezs_rsp_system_get_bluetooth_address_t rsp_system_get_bluetooth_address;
ezs_rsp_gap_get_device_name_t rsp_gap_get_device_name_bt;
ezs_rsp_gap_get_device_name_t rsp_gap_get_device_name_ble;
ezs_rsp_system_get_tx_power_t rsp_system_get_tx_power;
ezs_rsp_smp_get_privacy_mode_t rsp_smp_get_privacy_mode;
#if USE_GET_SET_ADV_PARAM
ezs_rsp_gap_get_adv_parameters_t rsp_gap_get_adv_parameters;
#endif
ezs_rsp_system_get_uart_parameters_t rsp_system_get_uart_parameters;
ezs_rsp_gap_get_conn_parameters_t rsp_gap_get_conn_parameters;

static ezs_rsp_system_get_uart_parameters_t rsp_system_get_uart_parameters_ref
    = { .result = 0,
        .baud = BAUD_TO_USE,
        .autobaud = 0,
        .autocorrect = 0,
        .flow = FLOW_CONTROL,
        .databits = 8,
        .parity = 0,
        .stopbits = 1 };

static ezs_rsp_gap_get_conn_parameters_t rsp_gap_get_conn_parameters_ref = { .result = 0,
  .interval = 6, //Minimum = 0x0006 (6 * 1.25 ms = 7.5 ms)
  .slave_latency = 0,
  .supervision_timeout = 100,
  .scan_interval = 256,
  .scan_window = 256,
  .scan_timeout = 0 };

static ezs_rsp_gap_get_adv_parameters_t rsp_gap_get_adv_parameters_ref = {
  .result = 0,              //(Default=)
  .mode = 2,                //(Default=)
  .type = 3,                //(Default=)
  .channels = 7,            //(Default=)
  .high_interval = 50,      //(Default=48), units of 0.625 ms. 50 = 31.24ms
  .high_duration = 0,       //(Default=30), 0 = infinite advertising
  .low_interval = 50,       //(Default=2048), units of 0.625 ms. 50 = 31.24ms
  .low_duration = 0,        //(Default=60), 0 = infinite advertising
  .flags = 0,               //(Default=)
  .direct_addr = { { 0 } }, //(Default=)
  .direct_address_type = 0  //(Default=)
};

static ezs_rsp_smp_get_privacy_mode_t rsp_smp_get_privacy_mode_ref = {
  .mode = 4,       //(Default=4)
  .interval = 300, //A value of 300 was read from eval kit. Datasheet states setting isn't supported.
};

uint8_t btInitCmdsRunning, btInitCmdsStep, btFactoryResetCmdsRunning, btFactoryResetCmdsStep;
uint8_t btNameTypeBeingRead;
bool btIsInitialised, btIsFactoryResetted, btCysppState, btUartSettingsChanged;

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
  if (reverse)
    data += bytes;
  while (bytes)
  {
    if (reverse)
      data--;
    //printf(((*data >> 4) & 0xF) < 10 ? ('0' + ((*data >> 4) & 0xF)) : ('A' - 10 + ((*data >> 4) & 0xF)));
    //printf(( *data       & 0xF) < 10 ? ('0' + ( *data       & 0xF)) : ('A' - 10 + ( *data       & 0xF)));

    printf("%c",
        ((*data >> 4) & 0xF) < 10 ? ('0' + ((*data >> 4) & 0xF)) :
                                    ('A' - 10 + ((*data >> 4) & 0xF)));
    printf("%c", (*data & 0xF) < 10 ? ('0' + (*data & 0xF)) : ('A' - 10 + (*data & 0xF)));

    if (!reverse)
      data++;
    bytes--;
    if (bytes && separator)
      printf("%c", separator);
  }
}

void btInit(void)
{
  btInitCmdsRunning = 1;
  btInitCmdsStep = WAIT_FOR_BOOT;
  btIsInitialised = false;

  /* packet pointer for working with response/event data */
  //ezs_packet_t *packet;

  //printf("\r\nEZ-Serial API communication demo started\r\n");

  resetEzsPendingResponse();

  /* initialize EZ-Serial interface and callbacks */
  EZSerial_Init(appHandler, appOutput, appInput);

  HAL_StatusTypeDef status = setBtRxDmaWaitingForResponse(1);

  btInitCommands();

  //if ((packet = EZS_SEND_AND_WAIT(ezs_cmd_system_query_firmware_version(), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) == 0)
  //{
  //  /* "system_ping" response packet not received */
  //  printf("FW request timed out, check communication settings and reset module\r\n");
  //}
  //else
  //{
  //  /* "system_ping" response packet received */
  //  printf("FW request successful\r\n");
  //}
  //
  //if ((packet = EZS_SEND_AND_WAIT(ezs_cmd_system_get_bluetooth_address(), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) != 0)
  //{
  //  /* "system_ping" response packet received */
  //  printf("BT address request successful\r\n");
  //}
  //
  //if ((packet = EZS_SEND_AND_WAIT(ezs_cmd_gap_set_device_name(1U, &nameAry[0]), COMMAND_TIMEOUT_MS*HAL_GetTickFreq())) != 0)
  //{
  // /*"system_ping" response packet received*/
  //printf("Device name set successful\r\n");
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
void btInitCommands(void)
{
  if (btInitCmdsStep == WAIT_FOR_BOOT)
  {
    btInitCmdsStep++;
    //// TODO will be needed once we can turn the module on/off
    //setExpectedResponse(EZS_IDX_RSP_SYSTEM_REBOOT);
    //return;
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE1)
  {
    btInitCmdsStep++;
    printf("Update UART Stage1\r\n");
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_GET_UART_PARAMETERS);
    ezs_cmd_system_get_uart_parameters(UART_TYPE_PUART);
    return;
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE2)
  {
    btInitCmdsStep++;
    printf("Current Baud=%lu\r\n", rsp_system_get_uart_parameters.baud);
    btUartSettingsChanged = false;
    //No need to set if the current settings are correct.
    if (memcmp(&rsp_system_get_uart_parameters_ref.result,
            &rsp_system_get_uart_parameters.result, sizeof(rsp_system_get_uart_parameters_ref))
        != 0)
    {
      printf("Update UART Stage2\r\n");
      printf("Setting Baud to %lu\r\n", rsp_system_get_uart_parameters_ref.baud);
      btUartSettingsChanged = true;
      setExpectedResponse(EZS_IDX_RSP_SYSTEM_SET_UART_PARAMETERS);
      ezs_cmd_system_set_uart_parameters(rsp_system_get_uart_parameters_ref.baud,
          rsp_system_get_uart_parameters_ref.autobaud,
          rsp_system_get_uart_parameters_ref.autocorrect,
          rsp_system_get_uart_parameters_ref.flow,
          rsp_system_get_uart_parameters_ref.databits,
          rsp_system_get_uart_parameters_ref.parity,
          rsp_system_get_uart_parameters_ref.stopbits, UART_TYPE_PUART);
      return;
    }
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE3)
  {
    btInitCmdsStep++;
    if (btUartSettingsChanged)
    {
      printf("Update UART Stage3\r\n");
      //TODO resolve reference
      usartBtUpdate(rsp_system_get_uart_parameters_ref.baud,
          rsp_system_get_uart_parameters_ref.flow);
    }
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE4)
  {
    if (btUartSettingsChanged)
    {
      /* Experimentally it was found that a delay was needed after the baud rate
       * is updated and before more commands are sent to the module, otherwise
       * the module reboots. We can't call HAL_delay here as it's being called
       * from an interrupt so this is done back in main.c */
      printf("Update UART Stage4\r\n");
      return;
    }
    else
    {
      btInitCmdsStep++;
    }
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE5)
  {
    btInitCmdsStep++;
    if (btUartSettingsChanged)
    {
      printf("Update UART Stage5\r\n");

      setExpectedResponse(EZS_IDX_RSP_SYSTEM_SET_UART_PARAMETERS);
      ezs_fcmd_system_set_uart_parameters(rsp_system_get_uart_parameters_ref.baud,
          rsp_system_get_uart_parameters_ref.autobaud,
          rsp_system_get_uart_parameters_ref.autocorrect,
          rsp_system_get_uart_parameters_ref.flow,
          rsp_system_get_uart_parameters_ref.databits,
          rsp_system_get_uart_parameters_ref.parity,
          rsp_system_get_uart_parameters_ref.stopbits, UART_TYPE_PUART);

      //setExpectedResponse(EZS_IDX_RSP_SYSTEM_STORE_CONFIG);
      //ezs_cmd_system_store_config();

      btUartSettingsChanged = false;
      return;
    }
  }

  if (btInitCmdsStep == PING)
  {
    printf("Ping\r\n");
    btInitCmdsStep++;
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_PING);
    ezs_cmd_system_ping();
    return;
  }

  if (btInitCmdsStep == STOP_BLE_ADVERTISING_STAGE1)
  {
    printf("Stop BLE Advertising\r\n");
    btInitCmdsStep++;
    setExpectedResponse(EZS_IDX_RSP_GAP_STOP_ADV);
    ezs_cmd_gap_stop_adv();
    return;
  }

  if (btInitCmdsStep == STOP_BLE_ADVERTISING_STAGE2)
  {
    printf("Wait for BLE stop\r\n");
    btInitCmdsStep++;
    setExpectedResponse(EZS_IDX_EVT_GAP_ADV_STATE_CHANGED);
    return;
  }


  if (btInitCmdsStep == GET_FIRMWARE_VERSION)
  {
    printf("Get FW Version\r\n");
    btInitCmdsStep++;
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_QUERY_FIRMWARE_VERSION);
    ezs_cmd_system_query_firmware_version();
    return;
  }

  if (btInitCmdsStep == GET_BT_MAC_ID)
  {
    printf("Get BT address\r\n");
    btInitCmdsStep++;
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_GET_BLUETOOTH_ADDRESS);
    ezs_cmd_system_get_bluetooth_address();
    return;
  }

  if (btInitCmdsStep == UPDATE_LOCAL_ADVERTISING_NAMES)
  {
    printf("Update local advertising name\r\n");
    btInitCmdsStep++;
    advNameBt[advNameMacIdStartIdx]
        = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[1] >> 4) & 0x0F);
    advNameBt[advNameMacIdStartIdx + 1]
        = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[1]) & 0x0F);
    advNameBt[advNameMacIdStartIdx + 2]
        = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[0] >> 4) & 0x0F);
    advNameBt[advNameMacIdStartIdx + 3]
        = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[0]) & 0x0F);

    advNameBle[advNameMacIdStartIdx]
        = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[1] >> 4) & 0x0F);
    advNameBle[advNameMacIdStartIdx + 1]
        = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[1]) & 0x0F);
    advNameBle[advNameMacIdStartIdx + 2]
        = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[0] >> 4) & 0x0F);
    advNameBle[advNameMacIdStartIdx + 3]
        = hexdigit2int((rsp_system_get_bluetooth_address.address.addr[0]) & 0x0F);
  }

  if (btInitCmdsStep == GET_DEVICE_NAME_BT)
  {
    printf("Get Device Name BT\r\n");
    btInitCmdsStep++;
    btNameTypeBeingRead = DEVICE_TYPE_BT;
    setExpectedResponse(EZS_IDX_RSP_GAP_GET_DEVICE_NAME);
    ezs_cmd_gap_get_device_name(DEVICE_TYPE_BT);
    return;
  }

  if (btInitCmdsStep == SET_DEVICE_NAME_BT)
  {
    btInitCmdsStep++;
    if (!strstr((char *) &rsp_gap_get_device_name_bt.name.data[0], &advNameBt[1]))
    {
      printf("Set Device Name BT\r\n");
      setExpectedResponse(EZS_IDX_RSP_GAP_SET_DEVICE_NAME);
      ezs_fcmd_gap_set_device_name(DEVICE_TYPE_BT, &advNameBt[0]);
      return;
    }
  }

  if (btInitCmdsStep == GET_DEVICE_NAME_BLE)
  {
    printf("Get Device Name BLE\r\n");
    btInitCmdsStep++;
    btNameTypeBeingRead = DEVICE_TYPE_BLE;
    setExpectedResponse(EZS_IDX_RSP_GAP_GET_DEVICE_NAME);
    ezs_cmd_gap_get_device_name(DEVICE_TYPE_BLE);
    return;
  }

  //TODO BLE advertising name won't update on-the-fly, need to either stop adv before name change and then start again or use F cmd and reset module.
  if (btInitCmdsStep == SET_DEVICE_NAME_BLE)
  {
    btInitCmdsStep++;
    if (!strstr((char *) &rsp_gap_get_device_name_ble.name.data[0], &advNameBle[1]))
    {
      printf("Set Device Name BLE\r\n");
      setExpectedResponse(EZS_IDX_RSP_GAP_SET_DEVICE_NAME);
      ezs_fcmd_gap_set_device_name(DEVICE_TYPE_BLE, &advNameBle[0]);
      return;
    }
  }

  if (btInitCmdsStep == GET_TX_POWER)
  {
    printf("Get TX Power\r\n");
    btInitCmdsStep++;
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_GET_TX_POWER);
    ezs_cmd_system_get_tx_power();
    return;
  }

  if (btInitCmdsStep == SET_TX_POWER)
  {
    btInitCmdsStep++;
    if (rsp_system_get_tx_power.power != BT_TX_POWER)
    {
      printf("Set TX Power\r\n");
      rsp_system_get_tx_power.power = BT_TX_POWER;
      setExpectedResponse(EZS_IDX_RSP_SYSTEM_SET_TX_POWER);
      ezs_fcmd_system_set_tx_power(BT_TX_POWER, &rsp_system_get_tx_power.power_level_arrays);
      return;
    }
  }

  if (btInitCmdsStep == GET_SMP_PRIVACY_MODE)
  {
    printf("Get SMP Privacy mode\r\n");
    btInitCmdsStep++;
    setExpectedResponse(EZS_IDX_RSP_SMP_GET_PRIVACY_MODE);
    ezs_cmd_smp_get_privacy_mode();
    return;
  }

  if (btInitCmdsStep == SET_SMP_PRIVACY_MODE)
  {
    btInitCmdsStep++;
    if (rsp_smp_get_privacy_mode.mode != rsp_smp_get_privacy_mode_ref.mode)
    {
      printf("Set SMP Privacy mode\r\n");
      setExpectedResponse(EZS_IDX_RSP_SMP_SET_PRIVACY_MODE);
      //Leaving the interval value unchanged from what is set in the module
      ezs_fcmd_smp_set_privacy_mode(
          rsp_smp_get_privacy_mode_ref.mode, rsp_smp_get_privacy_mode.interval);
      return;
    }
  }

#if USE_GET_SET_ADV_PARAM
  if (btInitCmdsStep == GET_ADVERTISING_PARAMETERS)
  {
    btInitCmdsStep++;
    printf("Get Advertising Parameters\r\n");
    setExpectedResponse(EZS_IDX_RSP_GAP_GET_ADV_PARAMETERS);
    ezs_cmd_gap_get_adv_parameters();
    return;
  }

  if (btInitCmdsStep == SET_ADVERTISING_PARAMETERS)
  {
    btInitCmdsStep++;

    if (memcmp(&rsp_gap_get_adv_parameters_ref.result,
            &rsp_gap_get_adv_parameters.result, sizeof(rsp_gap_get_adv_parameters_ref))
        != 0)
    {
      printf("Set Advertising Parameters\r\n");
      setExpectedResponse(EZS_IDX_RSP_GAP_SET_ADV_PARAMETERS);
      ezs_fcmd_gap_set_adv_parameters(rsp_gap_get_adv_parameters_ref.mode,
          rsp_gap_get_adv_parameters_ref.type, rsp_gap_get_adv_parameters_ref.channels,
          rsp_gap_get_adv_parameters_ref.high_interval,
          rsp_gap_get_adv_parameters_ref.high_duration,
          rsp_gap_get_adv_parameters_ref.low_interval,
          rsp_gap_get_adv_parameters_ref.low_duration,
          rsp_gap_get_adv_parameters_ref.flags, &rsp_gap_get_adv_parameters_ref.direct_addr,
          rsp_gap_get_adv_parameters_ref.direct_address_type);
      return;
    }
  }
#endif

  if (btInitCmdsStep == GET_CONN_PARAMETERS)
  {
    btInitCmdsStep++;
    printf("Get Conn Param\r\n");
    setExpectedResponse(EZS_IDX_RSP_GAP_GET_CONN_PARAMETERS);
    ezs_cmd_gap_get_conn_parameters();
    return;
  }

  if (btInitCmdsStep == SET_CONN_PARAMETERS)
  {
    btInitCmdsStep++;
    //No need to set if the current settings are correct.
    if (memcmp(&rsp_gap_get_conn_parameters_ref.result,
            &rsp_gap_get_conn_parameters.result, sizeof(rsp_gap_get_conn_parameters_ref))
        != 0)
    {
      printf("Set Conn Param\r\n");
      setExpectedResponse(EZS_IDX_RSP_GAP_SET_CONN_PARAMETERS);
      ezs_cmd_gap_set_conn_parameters(rsp_gap_get_conn_parameters_ref.interval,
          rsp_gap_get_conn_parameters_ref.slave_latency,
          rsp_gap_get_conn_parameters_ref.supervision_timeout,
          rsp_gap_get_conn_parameters_ref.scan_interval,
          rsp_gap_get_conn_parameters_ref.scan_window,
          rsp_gap_get_conn_parameters_ref.scan_timeout);
      return;
    }
  }

  if (btInitCmdsStep == START_BLE_ADVERTISING_STAGE1)
  {
    btInitCmdsStep++;
    printf("Start BLE Advertising\r\n");
    setExpectedResponse(EZS_IDX_RSP_GAP_START_ADV);
    ezs_cmd_gap_start_adv(rsp_gap_get_adv_parameters_ref.mode,
        rsp_gap_get_adv_parameters_ref.type, rsp_gap_get_adv_parameters_ref.channels,
        rsp_gap_get_adv_parameters_ref.high_interval,
        rsp_gap_get_adv_parameters_ref.high_duration,
        rsp_gap_get_adv_parameters_ref.low_interval,
        rsp_gap_get_adv_parameters_ref.low_duration,
        rsp_gap_get_adv_parameters_ref.flags, &rsp_gap_get_adv_parameters_ref.direct_addr,
        rsp_gap_get_adv_parameters_ref.direct_address_type);
    return;
  }

  if (btInitCmdsStep == START_BLE_ADVERTISING_STAGE2)
  {
    btInitCmdsStep++;
    printf("Wait for BLE start\r\n");
    setExpectedResponse(EZS_IDX_EVT_GAP_ADV_STATE_CHANGED);
    return;
  }

  if (btInitCmdsStep == FINISH)
  {
    btInitCmdsRunning = 0;
    btIsInitialised = true;
    return;
  }
}

void btFactoryResetInit(void)
{
  btFactoryResetCmdsRunning = 1;
  btFactoryResetCmdsStep = FR_WAIT_FOR_BOOT;
  btIsFactoryResetted = false;

  resetEzsPendingResponse();

  /* initialize EZ-Serial interface and callbacks */
  EZSerial_Init(appHandler, appOutput, appInput);

  HAL_StatusTypeDef status = setBtRxDmaWaitingForResponse(1);

  btFactoryResetCommands();
}

void btFactoryResetCommands(void)
{
  if (btFactoryResetCmdsStep == FR_WAIT_FOR_BOOT)
  {
    btFactoryResetCmdsStep++;
    //// TODO will be needed once we can turn the module on/off
    //setExpectedResponse(EZS_IDX_RSP_SYSTEM_REBOOT);
    //return;
  }

  if (btFactoryResetCmdsStep == FR_GET_BT_MAC_ID)
  {
    printf("Get BT address\r\n");
    btFactoryResetCmdsStep++;
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_GET_BLUETOOTH_ADDRESS);
    ezs_cmd_system_get_bluetooth_address();
    return;
  }

  if (btFactoryResetCmdsStep == FACTORY_RESET)
  {
    btFactoryResetCmdsStep++;
    printf("Factory Reset\r\n");
    setExpectedResponse(EZS_IDX_EVT_SYSTEM_FACTORY_RESET_COMPLETE);
    ezs_cmd_system_factory_reset();
    return;
  }

  if (btFactoryResetCmdsStep == FR_WAIT_FOR_REBOOT_AFTER_FR)
  {
    return;
  }

  if (btFactoryResetCmdsStep == FR_UPDATE_UART)
  {
    btFactoryResetCmdsStep++;
    printf("Update UART to factory default\r\n");
    //TODO resolve reference
    usartBtUpdate(115200, 0);
    HAL_StatusTypeDef status = setBtRxDmaWaitingForResponse(1);
  }

  if (btFactoryResetCmdsStep == FR_PING)
  {
    printf("Ping\r\n");
    btFactoryResetCmdsStep++;
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_PING);
    ezs_cmd_system_ping();
    return;
  }

  if (btFactoryResetCmdsStep == FR_RESET_BT_MAC_ID)
  {
    btFactoryResetCmdsStep++;
    printf("Reset BT Address\r\n");
    setExpectedResponse(EZS_IDX_CMD_SYSTEM_SET_BLUETOOTH_ADDRESS);

    //TODO EZ-Serial user guide states that setting to 0 will restore original
    //MAC ID but I haven't been able to get this working macaddr_t addr; memset(addr.addr,
    //0, sizeof(addr.addr)); ezs_fcmd_system_set_bluetooth_address(&addr);

    ezs_fcmd_system_set_bluetooth_address(
        &rsp_system_get_bluetooth_address.address.addr);

    return;
  }

  if (btFactoryResetCmdsStep == FR_FINISH)
  {
    btFactoryResetCmdsRunning = 0;
    btIsFactoryResetted = true;
    return;
  }
}

uint8_t isEzsBaudRateDelayPending(void)
{
  return btInitCmdsRunning && btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE4
      && btUartSettingsChanged;
}

uint8_t isEzsFactoryRebootDelayPending(void)
{
  return btFactoryResetCmdsRunning && btFactoryResetCmdsStep == FR_WAIT_FOR_REBOOT_AFTER_FR;
}

void incrementBtInitCmdsStep(void)
{
  btInitCmdsStep++;
}

void incrementBtFactoryResetCmdsStep(void)
{
  btFactoryResetCmdsStep++;
}

void ezsHandler(ezs_packet_t *packet)
{
}

void ezsHandlerShimmer(ezs_packet_t *packet)
{

  switch (packet->tbl_index)
  {
  case EZS_IDX_RSP_SYSTEM_PING:
    rsp_system_ping = packet->payload.rsp_system_ping;
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_system_ping.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_system_ping: result=");
      printHex16(packet->payload.rsp_system_ping.result);
      printf(", runtime=");
      printHex32(packet->payload.rsp_system_ping.runtime);
      printf(", fraction=");
      printHex16(packet->payload.rsp_system_ping.fraction);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_RSP_SYSTEM_QUERY_FIRMWARE_VERSION:
    rsp_system_query_firmware_version = packet->payload.rsp_system_query_firmware_version;

#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: rsp_system_query_firmware_version: app=");
    printHex32(packet->payload.rsp_system_query_firmware_version.app);
    printf(", stack=");
    printHex32(packet->payload.rsp_system_query_firmware_version.stack);
    printf(", protocol=");
    printHex16(packet->payload.rsp_system_query_firmware_version.protocol);
    printf(", hardware=");
    printHex8(packet->payload.rsp_system_query_firmware_version.hardware);

    /* check for protocol version older than v1.3 */
    if (packet->payload.rsp_system_query_firmware_version.protocol < 0x0103)
    {
      printf("\r\n*** PLEASE UPDATE TARGET MODULE TO LATEST VERISON OF "
             "EZ-SERIAL FIRMWARE");
    }
    printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_SYSTEM_REBOOT:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_system_reboot.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_system_reboot: result=");
      printHex16(packet->payload.rsp_system_reboot.result);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_EVT_SYSTEM_BOOT:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
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
    printf("\r\n");
#endif
    break;

  case EZS_IDX_EVT_GAP_ADV_STATE_CHANGED:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: evt_gap_adv_state_changed: state=");
    printHex8(packet->payload.evt_gap_adv_state_changed.state);
    printf(", reason=");
    printHex8(packet->payload.evt_gap_adv_state_changed.reason);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_EVT_GAP_SCAN_STATE_CHANGED:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: evt_gap_scan_state_changed: state=");
    printHex8(packet->payload.evt_gap_scan_state_changed.state);
    printf(", reason=");
    printHex8(packet->payload.evt_gap_scan_state_changed.reason);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_EVT_GAP_CONNECTED:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
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
    printf("\r\n");
#endif
    setBtConnectionState(true);
    break;

  case EZS_IDX_EVT_GAP_DISCONNECTED:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: evt_gap_disconnected: conn_handle=");
    printHex8(packet->payload.evt_gap_disconnected.conn_handle);
    printf(", reason=");
    printHex16(packet->payload.evt_gap_disconnected.reason);
    printf("\r\n");
#endif
    setBtConnectionState(false);
    break;

  case EZS_IDX_EVT_P_CYSPP_STATUS:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: evt_p_cyspp_status: status=");
    printHex8(packet->payload.evt_p_cyspp_status.status);
    printf("\r\n");
#endif
    setBtCysppState(packet->payload.evt_p_cyspp_status.status);
    break;


  /* -------- Shimmer added start -------- */
  case EZS_IDX_RSP_SYSTEM_GET_BLUETOOTH_ADDRESS:
    rsp_system_get_bluetooth_address = packet->payload.rsp_system_get_bluetooth_address;
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
//printf("RX: rsp_system_query_firmware_version: Address=");
//printHexMac(packet->payload.rsp_system_get_bluetooth_address.address);
#endif
    break;

  case EZS_IDX_RSP_GAP_SET_DEVICE_NAME:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_gap_set_device_name.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_gap_set_device_name: Result=");
      printHex16(packet->payload.rsp_gap_set_device_name.result);
    }
#endif
    break;

  case EZS_IDX_RSP_GAP_GET_DEVICE_NAME:

    if (btNameTypeBeingRead == DEVICE_TYPE_BLE)
    {
      rsp_gap_get_device_name_ble = packet->payload.rsp_gap_get_device_name;
    }
    else
    {
      rsp_gap_get_device_name_bt = packet->payload.rsp_gap_get_device_name;
    }

#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
//printf("RX: rsp_gap_get_device_name: name=");
//printHexMac(packet->payload.rsp_gap_get_device_name.name);
#endif
    break;

  case EZS_IDX_RSP_SYSTEM_GET_TX_POWER:
    rsp_system_get_tx_power = packet->payload.rsp_system_get_tx_power;
    break;

  case EZS_IDX_RSP_SYSTEM_SET_TX_POWER:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_system_set_tx_power.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_system_set_tx_power: Result=");
      printHex16(packet->payload.rsp_system_set_tx_power.result);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_RSP_SYSTEM_GET_UART_PARAMETERS:
    rsp_system_get_uart_parameters = packet->payload.rsp_system_get_uart_parameters;
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
//printf("RX: rsp_gap_set_device_appearance: Result=");
//printHex16(packet->payload.rsp_gap_set_device_appearance.result);
//printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_GAP_GET_CONN_PARAMETERS:
    rsp_gap_get_conn_parameters = packet->payload.rsp_gap_get_conn_parameters;
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
//printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_GAP_SET_CONN_PARAMETERS:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_gap_set_conn_parameters.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_gap_set_conn_parameters: Result=");
      printHex16(packet->payload.rsp_gap_set_conn_parameters.result);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_RSP_SYSTEM_SET_UART_PARAMETERS:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_gap_set_device_appearance.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_gap_set_device_appearance: Result=");
      printHex16(packet->payload.rsp_gap_set_device_appearance.result);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_RSP_GAP_SET_DEVICE_APPEARANCE:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: rsp_gap_set_device_appearance: Result=");
    printHex16(packet->payload.rsp_gap_set_device_appearance.result);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_GAP_SET_ADV_PARAMETERS:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_gap_set_adv_parameters.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_gap_set_adv_parameters: Result=");
      printHex16(packet->payload.rsp_gap_set_adv_parameters.result);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_RSP_GAP_GET_ADV_PARAMETERS:
#if USE_GET_SET_ADV_PARAM
    rsp_gap_get_adv_parameters = packet->payload.rsp_gap_get_adv_parameters;
#endif
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
//printHex16(packet->payload.rsp_gap_get_adv_parameters.result);
//printf("\r\n");
#endif
    break;

  case EZS_IDX_EVT_GAP_CONNECTION_UPDATED:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: evt_gap_connection_updated: conn_handle=");
    printHex8(packet->payload.evt_gap_connection_updated.conn_handle);
    printf(", interval=");
    printHex16(packet->payload.evt_gap_connection_updated.interval);
    printf(", slave_latency=");
    printHex16(packet->payload.evt_gap_connection_updated.slave_latency);
    printf(", supervision_timeout=");
    printHex16(packet->payload.evt_gap_connection_updated.supervision_timeout);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_GAP_START_ADV:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_gap_start_adv.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_gap_start_adv: Result=");
      printHex16(packet->payload.rsp_gap_start_adv.result);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_RSP_GAP_STOP_ADV:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_gap_stop_adv.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_gap_stop_adv: Result=");
      printHex16(packet->payload.rsp_gap_stop_adv.result);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_EVT_SMP_ENCRYPTION_STATUS:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
//printf("RX: evt_smp_encryption_status: conn_handle=");
//printHex8(packet->payload.evt_smp_encryption_status.conn_handle);
//printf(", status=");
//printHex8(packet->payload.evt_smp_encryption_status.status);
//printf("\r\n");
#endif
    break;

  case EZS_IDX_EVT_BT_CONNECTED:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: evt_bt_connected: conn_handle=");
    printHex8(packet->payload.evt_bt_connected.conn_handle);
    printf(", Address=");
    printHexMac(packet->payload.evt_bt_connected.address);
    printf(", Type=");
    printHex8(packet->payload.evt_bt_connected.type);
    printf("\r\n");
#endif

    setBtConnectionState(true);
    setBtCysppState(true);
    break;

  case EZS_IDX_EVT_BT_DISCONNECTED:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: evt_bt_disconnected: conn_handle=");
    printHex8(packet->payload.evt_bt_disconnected.conn_handle);
    printf(", Reason=");
    printHex16(packet->payload.evt_bt_disconnected.reason);
    printf("\r\n");
#endif

    setBtCysppState(false);
    setBtConnectionState(false);
    break;

  case EZS_IDX_EVT_SYSTEM_ERROR:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("CYW20820 System Error\r\n");
#endif
    break;

  case EZS_IDX_RSP_SYSTEM_FACTORY_RESET:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("ACK factory reset\r\n");
#endif
    break;

  case EZS_IDX_EVT_SYSTEM_FACTORY_RESET_COMPLETE:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("Factory reset complete\r\n");
#endif
    break;

  case EZS_IDX_RSP_SYSTEM_STORE_CONFIG:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("Store config complete\r\n");
    printHex16(packet->payload.rsp_system_store_config.result);
#endif
    break;

  case EZS_IDX_RSP_SMP_GET_PRIVACY_MODE:
    rsp_smp_get_privacy_mode = packet->payload.rsp_smp_get_privacy_mode;
    break;

    /* -------- Shimmer added end -------- */

  default:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: unhandled packet: ");
    printHex8(packet->header.group);
    printf("/");
    printHex8(packet->header.id);
    printf("\r\n");
#endif
    break;
  }

  //printf("\r\n");

  if ((btInitCmdsRunning || btFactoryResetCmdsRunning) && packet->tbl_index == expectedResponseIdx)
  {
    /* Set the expected response to be invalid */
    setExpectedResponse(EZS_IDX_EVT_MAX);
    if (btInitCmdsRunning)
    {
      /* Continue with the next set command */
      btInitCommands();
    }
    else if (btFactoryResetCmdsRunning)
    {
      /* Continue with the next set command */
      btFactoryResetCommands();
    }
  }
}

void setExpectedResponse(uint16_t idx)
{
  expectedResponseIdx = idx;
}

bool isBtIsInitialised(void)
{
  return btIsInitialised;
}

bool isBtIsFactoryResetted(void)
{
  return btIsFactoryResetted;
}

bool isBtInitCmdsRunning(void)
{
  return btInitCmdsRunning;
}

bool isBtFactoryResetCmdsRunning(void)
{
  return btFactoryResetCmdsRunning;
}

void setBtCysppState(bool state)
{
  btCysppState = state;
  //TODO move out of this driver file
  //HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, btCysppState? GPIO_PIN_SET:GPIO_PIN_RESET);

  //TODO move out of this driver file and make common with implementation in shimmer_bt_comms.c
  if (!state && getBtDataRateTestState())
  {
    setBtDataRateTestState(0);
    clearBtTxBuf(1);
  }
}

bool getBtCysppState(void)
{
  return btCysppState;
}

void setBtLpMode(bool allowLowPower)
{
  //TODO get working and update default LP_MODE pin state appropriately in CubeMX
  //TODO move out of this driver file
  //HAL_GPIO_WritePin(BT_LP_MODE_GPIO_Port, BT_LP_MODE_Pin, allowLowPower ? GPIO_PIN_RESET : GPIO_PIN_SET);

  //TODO remove or update delay when value known
  //HAL_Delay(100);
}

uint8_t *BT_getCyw20820MacAddressPtr(void)
{
  return &rsp_system_get_bluetooth_address.address.addr[0];
}

void BT_generateCyw20820FirmwareVersionStr(char *str)
{
  sprintf(str, "CYW20820 app=v%02d.%02d.%02d.%02d, stack=0x%08x, protocol=0x%04x, hardware=0x%02x",
      (uint8_t) (rsp_system_query_firmware_version.app >> 24),
      (uint8_t) (rsp_system_query_firmware_version.app >> 16),
      (uint8_t) (rsp_system_query_firmware_version.app >> 8),
      (uint8_t) (rsp_system_query_firmware_version.app >> 0),
      (uint16_t) rsp_system_query_firmware_version.stack,
      rsp_system_query_firmware_version.protocol,
      rsp_system_query_firmware_version.hardware);
}
