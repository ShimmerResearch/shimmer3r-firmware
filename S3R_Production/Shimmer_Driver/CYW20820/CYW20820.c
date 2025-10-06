/*
 * CYW20820.c
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#include "stdio.h"
#include "stm32u5xx.h"
#include "string.h"
#include "time.h"
#include <CYW20820/CYW20820.h>
#include <CYW20820/EZ-Serial/ezsapi.h>
#include <CYW20820/hal_CYW20820.h>

#include "main.h"
#include "usart.h"

#include "log_and_stream_includes.h"

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

//#define CONNECTION_TIMEOUT_MS          10000 //10 seconds timeout
#define CONNECTION_TIMEOUT_S           10 //10 seconds timeout

uint8_t advNameMacIdStartIdx = 11;
static char advNameBt[] = { 17, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', 'R',
  '-', 'X', 'X', 'X', 'X', '-', 'B', 'T' };
static char advNameBle[] = { 18, 'S', 'h', 'i', 'm', 'm', 'e', 'r', '3', 'R',
  '-', 'X', 'X', 'X', 'X', '-', 'B', 'L', 'E' };
//Legacy pin code for RN42, RN4678 compatibility
static char pin_code[] = { 4, '1', '2', '3', '4' };

uint16_t expectedResponseIdx;
uint8_t active_conn_handle = 0xFF; //no active connection

ezs_rsp_system_ping_t rsp_system_ping;
ezs_rsp_system_query_firmware_version_t rsp_system_query_firmware_version;
ezs_rsp_system_get_bluetooth_address_t rsp_system_get_bluetooth_address;
ezs_rsp_gap_get_device_name_t rsp_gap_get_device_name_bt;
ezs_rsp_gap_get_device_name_t rsp_gap_get_device_name_ble;
ezs_rsp_system_get_tx_power_t rsp_system_get_tx_power;
ezs_rsp_smp_get_privacy_mode_t rsp_smp_get_privacy_mode;
ezs_rsp_bt_get_parameters_t rsp_bt_get_parameters;
ezs_rsp_bt_cancel_connection_t rsp_bt_cancel_connection;
ezs_rsp_bt_disconnect_t rsp_bt_disconnect;
#if USE_GET_SET_ADV_PARAM
ezs_rsp_gap_get_adv_parameters_t rsp_gap_get_adv_parameters;
#endif
ezs_rsp_system_get_uart_parameters_t rsp_system_get_uart_parameters;
ezs_rsp_gap_get_conn_parameters_t rsp_gap_get_conn_parameters;
ezs_rsp_smp_get_security_parameters_t rsp_smp_get_security_parameters;

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
/* Mode = 0x41
 * - 0x4X = BT Classic -> MITM Protection Not Required - Single Profiles/general
 *   bonding. Numeric comparison with automatic accept allowed.
 * - 0xX1 = BLE -> Required - General Bond
 */
static ezs_rsp_smp_get_security_parameters_t rsp_smp_get_security_parameters_ref
    = { .mode = 0x41,
        .bonding = 1,
        .flags = 0x01,
        .keysize = 16,
        .io = 3, //3 = No Input + No Output – no ability to display or input anything (factory default)
        .pairprop = 0 }; //Default values
/* Mode = 0x40
 * - 0x4X = BT Classic -> MITM Protection Not Required - Single Profiles/general
 *   bonding. Numeric comparison with automatic accept allowed.
 * - 0xX0 = BLE -> Not required - No Bond
 */
static ezs_rsp_smp_get_security_parameters_t rsp_smp_get_security_parameters_ref_sd_sync
    = { .mode = 0x40,
        .bonding = 0,
        .flags = 0x04, //0x04 = Enable use of legacy PIN code during paring for BT classic device.
        .keysize = 16,
        .io = 3, //3 = No Input + No Output – no ability to display or input anything (factory default)
        .pairprop = 0 };

ezs_rsp_smp_get_pin_code_t rsp_smp_get_pin_code;

uint8_t *btInitCmdsSteps;
uint8_t btInitCmdsRunning, btInitCmdsStep, btInitCmdsStepIdx, btFactoryResetCmdsRunning;
uint8_t btNameTypeBeingRead;
bool btIsFactoryResetted, btCysppState, btUartSettingsChanged;

static uint8_t btBootStagesFirstBoot[] =
{ WAIT_FOR_BOOT_STAGE1, WAIT_FOR_BOOT_STAGE2, UPDATE_UART_SETTINGS_STAGE1,
    UPDATE_UART_SETTINGS_STAGE2, UPDATE_UART_SETTINGS_STAGE3,
    UPDATE_UART_SETTINGS_STAGE4, UPDATE_UART_SETTINGS_STAGE5, PING,
    GET_BT_PARAMETERS, STOP_BT_ADVERTISING, STOP_BLE_ADVERTISING_STAGE1,
    STOP_BLE_ADVERTISING_STAGE2, GET_FIRMWARE_VERSION, GET_BT_MAC_ID,
    UPDATE_LOCAL_ADVERTISING_NAMES, GET_DEVICE_NAME_BT, SET_DEVICE_NAME_BT,
    GET_DEVICE_NAME_BLE, SET_DEVICE_NAME_BLE, GET_TX_POWER, SET_TX_POWER,
    GET_SMP_PRIVACY_MODE, SET_SMP_PRIVACY_MODE,
#if USE_GET_SET_ADV_PARAM
  GET_ADVERTISING_PARAMETERS, SET_ADVERTISING_PARAMETERS,
#endif
    GET_CONN_PARAMETERS, SET_CONN_PARAMETERS, GET_SECURITY_PARAMETERS,
    SET_SECURITY_PARAMETERS, GET_PIN_CODE, SET_PIN_CODE,
    START_BLE_ADVERTISING_STAGE1, START_BLE_ADVERTISING_STAGE2,
    START_BT_ADVERTISING, FINISH };

static uint8_t btBootStagesSubsequentBoot[] = { WAIT_FOR_BOOT_STAGE1,
  WAIT_FOR_BOOT_STAGE2, /* PING,*/
  GET_SECURITY_PARAMETERS,
  SET_SECURITY_PARAMETERS, //Any command to get module into binary command mode. Added set, get security parameters here to get SD sync working
  FINISH };

static uint8_t btBootStagesFactoryReset[] = { WAIT_FOR_BOOT_STAGE1,
  WAIT_FOR_BOOT_STAGE2, GET_BT_MAC_ID, FACTORY_RESET, FR_WAIT_FOR_REBOOT_AFTER_FR,
  FR_UPDATE_UART, FR_PING, FR_RESET_BT_MAC_ID, FINISH };

void (*btIsInitialised_cb)(void);

static char hexdigit2int(uint8_t xd)
{
  if (xd <= 9)
  {
    return xd + '0';
  }
  if (xd == 10)
  {
    return 'A';
  }
  if (xd == 11)
  {
    return 'B';
  }
  if (xd == 12)
  {
    return 'C';
  }
  if (xd == 13)
  {
    return 'D';
  }
  if (xd == 14)
  {
    return 'E';
  }
  if (xd == 15)
  {
    return 'F';
  }
  return '0';
}

static void printHex(uint8_t *data, uint8_t bytes, uint8_t reverse, char separator)
{
  if (reverse)
  {
    data += bytes;
  }
  while (bytes)
  {
    if (reverse)
    {
      data--;
    }
    //printf(((*data >> 4) & 0xF) < 10 ? ('0' + ((*data >> 4) & 0xF)) : ('A' - 10 + ((*data >> 4) & 0xF)));
    //printf(( *data       & 0xF) < 10 ? ('0' + ( *data       & 0xF)) : ('A' - 10 + ( *data       & 0xF)));

    printf("%c",
        ((*data >> 4) & 0xF) < 10 ? ('0' + ((*data >> 4) & 0xF)) :
                                    ('A' - 10 + ((*data >> 4) & 0xF)));
    printf("%c", (*data & 0xF) < 10 ? ('0' + (*data & 0xF)) : ('A' - 10 + (*data & 0xF)));

    if (!reverse)
    {
      data++;
    }
    bytes--;
    if (bytes && separator)
    {
      printf("%c", separator);
    }
  }
}

void btInit(void)
{
  btInitCmdsStepIdx = 0;
  btInitCmdsStep = WAIT_FOR_BOOT_STAGE1;

  initBtPins();
  //Enable BT power
  setBtPower(1);
  //TODO Delay found to be needed, unsure why. Arbitrary value of 10ms used.
  HAL_Delay(10);
  Board_BT_LP_MODE(1);
  Board_BT_CP_ROLE(1);

  uint32_t baudRate = ShimBt_getBtBaudRateToUse();
  uint8_t hwFlowControl = baudRate == 115200 ? 0 : FLOW_CONTROL;
  SHIMMER_PRINTF("BT Init: Baud=%lu, HW Flow Control=%d\r\n", baudRate, hwFlowControl);
  BtUart_init(baudRate, hwFlowControl);

  resetEzsPendingResponse();

  /* initialize EZ-Serial interface and callbacks */
  EZSerial_Init(appHandler, appOutput, appInput);

  //TODO Delay found to be needed, unsure why. Arbitrary value of 10ms used.
  HAL_Delay(10);
  /* Setting DMA waiting for first char from BT module */
  HAL_StatusTypeDef status = setBtRxDmaWaitingForResponse(1);
  /* Allow BT module to boot */
  Board_BT_RST_N(1);

  btInitCommands();
}

void btDeinit(void)
{
  setBtPower(0);
  btUart_deint();
  deinitBtPins();
}

//TODO set appropriate values for setDmaRx() calls
void btInitCommands(void)
{
  if (btInitCmdsStep == WAIT_FOR_BOOT_STAGE1)
  {
    incrementBtInitCmdsStep();
    //Only ASCII boot message currently working so had to implement our own
    //approach setExpectedResponse(EZS_IDX_RSP_SYSTEM_REBOOT);
    setWaitingForBtBoot(1);
    return;
  }

  if (btInitCmdsStep == WAIT_FOR_BOOT_STAGE2)
  {
    incrementBtInitCmdsStep();
    printf("Boot Msgs=\r\n%s", getBtBootMsgPtr());
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE1)
  {
    incrementBtInitCmdsStep();

    printf("Update UART Stage1\r\n");
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_GET_UART_PARAMETERS);
    ezs_cmd_system_get_uart_parameters(UART_TYPE_PUART);
    return;
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE2)
  {
    incrementBtInitCmdsStep();
    printf("Current Baud=%lu\r\n", rsp_system_get_uart_parameters.baud);
    btUartSettingsChanged = false;
    //No need to set if the current settings are correct.
    if (memcmp(&rsp_system_get_uart_parameters_ref.result,
            &rsp_system_get_uart_parameters.result, sizeof(rsp_system_get_uart_parameters_ref))
        != 0)
    {
      printf("Update UART Stage2\r\n");
      uint32_t baudToUse = rsp_system_get_uart_parameters_ref.baud;
#if SUPPORT_SR48_6_0
      if (ShimBrd_isBoardSr48_6_0())
      {
        baudToUse = BAUD_TO_USE_SR48_6_0;
      }
#endif //SUPPORT_SR48_6_0
      uint8_t flowCtrl = baudToUse == 115200 ? 0 : FLOW_CONTROL;

      printf("Setting Baud to %lu\r\n", baudToUse);
      btUartSettingsChanged = true;
      setExpectedResponse(EZS_IDX_RSP_SYSTEM_SET_UART_PARAMETERS);
      ezs_cmd_system_set_uart_parameters(baudToUse,
          rsp_system_get_uart_parameters_ref.autobaud,
          rsp_system_get_uart_parameters_ref.autocorrect, flowCtrl,
          rsp_system_get_uart_parameters_ref.databits,
          rsp_system_get_uart_parameters_ref.parity,
          rsp_system_get_uart_parameters_ref.stopbits, UART_TYPE_PUART);
      return;
    }
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE3)
  {
    incrementBtInitCmdsStep();
    if (btUartSettingsChanged)
    {
      printf("Update UART Stage3\r\n");
      uint32_t baudToUse = rsp_system_get_uart_parameters_ref.baud;
#if SUPPORT_SR48_6_0
      if (ShimBrd_isBoardSr48_6_0())
      {
        baudToUse = BAUD_TO_USE_SR48_6_0;
      }
#endif //SUPPORT_SR48_6_0
      uint8_t flowCtrl = baudToUse == 115200 ? 0 : FLOW_CONTROL;
      BtUart_update(baudToUse, flowCtrl);
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
      incrementBtInitCmdsStep();
    }
  }

  if (btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE5)
  {
    incrementBtInitCmdsStep();
    if (btUartSettingsChanged)
    {
      printf("Update UART Stage5\r\n");

      uint32_t baudToUse = rsp_system_get_uart_parameters_ref.baud;
#if SUPPORT_SR48_6_0
      if (ShimBrd_isBoardSr48_6_0())
      {
        baudToUse = BAUD_TO_USE_SR48_6_0;
      }
#endif //SUPPORT_SR48_6_0
      uint8_t flowCtrl = baudToUse == 115200 ? 0 : FLOW_CONTROL;

      setExpectedResponse(EZS_IDX_RSP_SYSTEM_SET_UART_PARAMETERS);
      ezs_fcmd_system_set_uart_parameters(baudToUse,
          rsp_system_get_uart_parameters_ref.autobaud,
          rsp_system_get_uart_parameters_ref.autocorrect, flowCtrl,
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
    incrementBtInitCmdsStep();
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_PING);
    ezs_cmd_system_ping();
    return;
  }

  if (btInitCmdsStep == GET_BT_PARAMETERS)
  {
    printf("Get BT Parameters\r\n");
    incrementBtInitCmdsStep();
    setExpectedResponse(EZS_IDX_RSP_BT_GET_PARAMETERS);
    ezs_cmd_bt_get_parameters();
    return;
  }

  if (btInitCmdsStep == STOP_BT_ADVERTISING)
  {
    printf("Stop BT Advertising\r\n");
    incrementBtInitCmdsStep();

    setExpectedResponse(EZS_IDX_RSP_BT_SET_PARAMETERS);

    rsp_bt_get_parameters.discoverable = 0;
    rsp_bt_get_parameters.connectable = 0;
    ezs_cmd_bt_set_parameters(rsp_bt_get_parameters.link_super_time_out,
        rsp_bt_get_parameters.discoverable, rsp_bt_get_parameters.connectable,
        rsp_bt_get_parameters.flags, rsp_bt_get_parameters.scn,
        rsp_bt_get_parameters.active_bt_discoverability,
        rsp_bt_get_parameters.active_bt_connectability);
    return;
  }

  if (btInitCmdsStep == STOP_BLE_ADVERTISING_STAGE1)
  {
    printf("Stop BLE Advertising\r\n");
    incrementBtInitCmdsStep();
    setExpectedResponse(EZS_IDX_RSP_GAP_STOP_ADV);
    ezs_cmd_gap_stop_adv();
    return;
  }

  if (btInitCmdsStep == STOP_BLE_ADVERTISING_STAGE2)
  {
    printf("Wait for BLE stop\r\n");
    incrementBtInitCmdsStep();
    setExpectedResponse(EZS_IDX_EVT_GAP_ADV_STATE_CHANGED);
    return;
  }

  if (btInitCmdsStep == GET_FIRMWARE_VERSION)
  {
    printf("Get FW Version\r\n");
    incrementBtInitCmdsStep();
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_QUERY_FIRMWARE_VERSION);
    ezs_cmd_system_query_firmware_version();
    return;
  }

  if (btInitCmdsStep == GET_BT_MAC_ID)
  {
    printf("Get BT address\r\n");
    incrementBtInitCmdsStep();
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_GET_BLUETOOTH_ADDRESS);
    ezs_cmd_system_get_bluetooth_address();
    return;
  }

  if (btInitCmdsStep == UPDATE_LOCAL_ADVERTISING_NAMES)
  {
    printf("Update local advertising name\r\n");
    incrementBtInitCmdsStep();
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
    incrementBtInitCmdsStep();
    btNameTypeBeingRead = DEVICE_TYPE_BT;
    setExpectedResponse(EZS_IDX_RSP_GAP_GET_DEVICE_NAME);
    ezs_cmd_gap_get_device_name(DEVICE_TYPE_BT);
    return;
  }

  if (btInitCmdsStep == SET_DEVICE_NAME_BT)
  {
    incrementBtInitCmdsStep();
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
    incrementBtInitCmdsStep();
    btNameTypeBeingRead = DEVICE_TYPE_BLE;
    setExpectedResponse(EZS_IDX_RSP_GAP_GET_DEVICE_NAME);
    ezs_cmd_gap_get_device_name(DEVICE_TYPE_BLE);
    return;
  }

  //TODO BLE advertising name won't update on-the-fly, need to either stop adv before name change and then start again or use F cmd and reset module.
  if (btInitCmdsStep == SET_DEVICE_NAME_BLE)
  {
    incrementBtInitCmdsStep();
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
    incrementBtInitCmdsStep();
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_GET_TX_POWER);
    ezs_cmd_system_get_tx_power();
    return;
  }

  if (btInitCmdsStep == SET_TX_POWER)
  {
    incrementBtInitCmdsStep();
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
    incrementBtInitCmdsStep();
    setExpectedResponse(EZS_IDX_RSP_SMP_GET_PRIVACY_MODE);
    ezs_cmd_smp_get_privacy_mode();
    return;
  }

  if (btInitCmdsStep == SET_SMP_PRIVACY_MODE)
  {
    incrementBtInitCmdsStep();
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
    incrementBtInitCmdsStep();
    printf("Get Advertising Parameters\r\n");
    setExpectedResponse(EZS_IDX_RSP_GAP_GET_ADV_PARAMETERS);
    ezs_cmd_gap_get_adv_parameters();
    return;
  }

  if (btInitCmdsStep == SET_ADVERTISING_PARAMETERS)
  {
    incrementBtInitCmdsStep();

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
    incrementBtInitCmdsStep();
    printf("Get Conn Param\r\n");
    setExpectedResponse(EZS_IDX_RSP_GAP_GET_CONN_PARAMETERS);
    ezs_cmd_gap_get_conn_parameters();
    return;
  }

  if (btInitCmdsStep == SET_CONN_PARAMETERS)
  {
    incrementBtInitCmdsStep();
    //No need to set if the current settings are correct.
    if (memcmp(&rsp_gap_get_conn_parameters_ref.result,
            &rsp_gap_get_conn_parameters.result, sizeof(rsp_gap_get_conn_parameters_ref))
        != 0)
    {
      printf("Set Conn Param\r\n");
      setExpectedResponse(EZS_IDX_RSP_GAP_SET_CONN_PARAMETERS);
      ezs_fcmd_gap_set_conn_parameters(rsp_gap_get_conn_parameters_ref.interval,
          rsp_gap_get_conn_parameters_ref.slave_latency,
          rsp_gap_get_conn_parameters_ref.supervision_timeout,
          rsp_gap_get_conn_parameters_ref.scan_interval,
          rsp_gap_get_conn_parameters_ref.scan_window,
          rsp_gap_get_conn_parameters_ref.scan_timeout);
      return;
    }
  }

  if (btInitCmdsStep == GET_SECURITY_PARAMETERS)
  {
    incrementBtInitCmdsStep();
    printf("Get Security Param\r\n");
    setExpectedResponse(EZS_IDX_RSP_SMP_GET_SECURITY_PARAMETERS);
    ezs_cmd_smp_get_security_parameters();
    return;
  }

  if (btInitCmdsStep == SET_SECURITY_PARAMETERS)
  {
    incrementBtInitCmdsStep();

    ezs_rsp_smp_get_security_parameters_t *securityParametersPtr;
    /* Stored config hasn't been read yet on first boot so default sec params
     * are used during initial boot */
    if (!shimmerStatus.initialising && ShimConfig_getStoredConfig()->syncEnable)
    {
      securityParametersPtr = &rsp_smp_get_security_parameters_ref_sd_sync;
    }
    else
    {
      securityParametersPtr = &rsp_smp_get_security_parameters_ref;
    }

    if (memcmp(&securityParametersPtr->result, &rsp_smp_get_security_parameters.result,
            sizeof(rsp_smp_get_security_parameters_ref))
        != 0)
    {
      printf("Set Security Param\r\n");
      setExpectedResponse(EZS_IDX_RSP_SMP_SET_SECURITY_PARAMETERS);
      ezs_cmd_smp_set_security_parameters(securityParametersPtr->mode,
          securityParametersPtr->bonding, securityParametersPtr->keysize,
          securityParametersPtr->pairprop, securityParametersPtr->io,
          securityParametersPtr->flags);
      return;
    }
  }

  if (btInitCmdsStep == GET_PIN_CODE)
  {
    incrementBtInitCmdsStep();
    printf("Get Pin Code\r\n");
    setExpectedResponse(EZS_IDX_RSP_SMP_GET_PIN_CODE);
    ezs_cmd_smp_get_pin_code();
    return;
  }

  if (btInitCmdsStep == SET_PIN_CODE)
  {
    incrementBtInitCmdsStep();

    if (memcmp(&rsp_smp_get_pin_code.pin_code, &pin_code[0], sizeof(pin_code)) != 0)
    {
      printf("Set Pin Code\r\n");
      setExpectedResponse(EZS_IDX_RSP_SMP_SET_PIN_CODE);
      ezs_fcmd_smp_set_pin_code(&pin_code[0]);
      return;
    }
  }

  if (btInitCmdsStep == START_BLE_ADVERTISING_STAGE1)
  {
    incrementBtInitCmdsStep();
    if (ShimBt_isBleCurrentlyEnabled())
    {
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
    else
    {
      printf("BLE disabled - Skipping start advertising\r\n");
      }
  }

  if (btInitCmdsStep == START_BLE_ADVERTISING_STAGE2)
  {
    incrementBtInitCmdsStep();
    if (ShimBt_isBleCurrentlyEnabled())
    {
      printf("Wait for BLE start\r\n");
      setExpectedResponse(EZS_IDX_EVT_GAP_ADV_STATE_CHANGED);
      return;
    }
  }

  if (btInitCmdsStep == START_BT_ADVERTISING)
  {
    incrementBtInitCmdsStep();
    if (ShimBt_isBtClassicCurrentlyEnabled())
    {
      printf("Start BT Advertising\r\n");
      setExpectedResponse(EZS_IDX_RSP_BT_SET_PARAMETERS);

      rsp_bt_get_parameters.discoverable = 2; // 2 = default = general discoverable
      rsp_bt_get_parameters.connectable = 1; // 1 = default = connectable
      ezs_cmd_bt_set_parameters(rsp_bt_get_parameters.link_super_time_out,
          rsp_bt_get_parameters.discoverable, rsp_bt_get_parameters.connectable,
          rsp_bt_get_parameters.flags, rsp_bt_get_parameters.scn,
          rsp_bt_get_parameters.active_bt_discoverability,
          rsp_bt_get_parameters.active_bt_connectability);
      return;
    }
    else
    {
      printf("BT disabled - Skipping start advertising\r\n");
    }
  }

  if (btInitCmdsStep == FACTORY_RESET)
  {
    incrementBtInitCmdsStep();
    printf("Factory Reset\r\n");
    setExpectedResponse(EZS_IDX_EVT_SYSTEM_FACTORY_RESET_COMPLETE);
    ezs_cmd_system_factory_reset();
    return;
  }

  if (btInitCmdsStep == FR_WAIT_FOR_REBOOT_AFTER_FR)
  {
    return;
  }

  if (btInitCmdsStep == FR_UPDATE_UART)
  {
    incrementBtInitCmdsStep();
    printf("Update UART to factory default\r\n");
    //TODO resolve reference
    BtUart_update(115200, 0);
    HAL_StatusTypeDef status = setBtRxDmaWaitingForResponse(1);
  }

  if (btInitCmdsStep == FR_PING)
  {
    incrementBtInitCmdsStep();
    printf("Ping\r\n");
    setExpectedResponse(EZS_IDX_RSP_SYSTEM_PING);
    ezs_cmd_system_ping();
    return;
  }

  if (btInitCmdsStep == FR_RESET_BT_MAC_ID)
  {
    incrementBtInitCmdsStep();
    printf("Reset BT Address\r\n");
    setExpectedResponse(EZS_IDX_CMD_SYSTEM_SET_BLUETOOTH_ADDRESS);

    //TODO EZ-Serial user guide states that setting to 0 will restore original
    //MAC ID but I haven't been able to get this working macaddr_t addr; memset(addr.addr,
    //0, sizeof(addr.addr)); ezs_fcmd_system_set_bluetooth_address(&addr);

    ezs_fcmd_system_set_bluetooth_address(
        &rsp_system_get_bluetooth_address.address.addr);

    return;
  }

  if (btInitCmdsStep == FINISH)
  {
    if (btFactoryResetCmdsRunning)
    {
      btFactoryResetCmdsRunning = 0;
      btIsFactoryResetted = true;
    }
    else
    {
      btInitCmdsRunning = 0;
      btIsInitialised_cb();
    }
    return;
  }
}

uint8_t isEzsBaudRateDelayPending(void)
{
  return btInitCmdsStep == UPDATE_UART_SETTINGS_STAGE4 && btUartSettingsChanged;
}

uint8_t isEzsFactoryRebootDelayPending(void)
{
  return btInitCmdsStep == FR_WAIT_FOR_REBOOT_AFTER_FR;
}

void setBtInitCmdsSteps(uint8_t *steps)
{
  btInitCmdsSteps = steps;
}

void incrementBtInitCmdsStep(void)
{
  if (btInitCmdsStep != FINISH)
  {
    btInitCmdsStepIdx++;
    btInitCmdsStep = btInitCmdsSteps[btInitCmdsStepIdx];
  }
}

void setBtBootModeFactoryReset(void)
{
  btFactoryResetCmdsRunning = 1;
  setBtInitCmdsSteps(&btBootStagesFactoryReset[0]);

  btIsFactoryResetted = false;
}

void setBtBootModeFirstBoot(void)
{
  btInitCmdsRunning = 1;
  setBtInitCmdsSteps(&btBootStagesFirstBoot[0]);
}

void setBtBootModeSubsequentBoot(void)
{
  btInitCmdsRunning = 1;
  setBtInitCmdsSteps(&btBootStagesSubsequentBoot[0]);
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

  case EZS_IDX_RSP_SMP_GET_SECURITY_PARAMETERS:
    rsp_smp_get_security_parameters = packet->payload.rsp_smp_get_security_parameters;
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: rsp_smp_get_security_parameters: Result=");
    printHex16(packet->payload.rsp_smp_get_security_parameters.result);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_SMP_SET_SECURITY_PARAMETERS:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: rsp_smp_set_security_parameters: Result=");
    printHex16(packet->payload.rsp_smp_set_security_parameters.result);
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

  case EZS_IDX_RSP_BT_CONNECT:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: idx_rsp_bt_connected: conn_handle=");
    printHex8(packet->payload.rsp_bt_connect.conn_handle);
    printf(", Result=");
    printHex16(packet->payload.rsp_bt_connect.result);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_BT_CANCEL_CONNECTION:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    rsp_bt_cancel_connection = packet->payload.rsp_bt_cancel_connection;
#endif
    break;
  case EZS_IDX_RSP_BT_DISCONNECT:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    rsp_bt_disconnect = packet->payload.rsp_bt_disconnect;
#endif
    break;

  case EZS_IDX_RSP_GAP_CONNECT:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: idx_rsp_gap_connect: conn_handle=");
    printHex8(packet->payload.rsp_gap_connect.conn_handle);
    printf(", Result=");
    printHex16(packet->payload.rsp_gap_connect.result);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_EVT_BT_CONNECTION_FAILED:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    BT_connectionFailed(packet->payload.evt_bt_connection_failed.conn_handle,
        packet->payload.evt_bt_connection_failed.reason);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_EVT_SMP_PAIRING_RESULT:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: evt_smp_pairing_result: conn_handle=");
    printHex8(packet->payload.evt_smp_pairing_result.conn_handle);
    printf(", Reason=");
    printHex16(packet->payload.evt_smp_pairing_result.result);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_SMP_GET_PIN_CODE:
    rsp_smp_get_pin_code = packet->payload.rsp_smp_get_pin_code;
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: rsp_smp_get_pin_code: pin_code=");
    printf("%.*s", (int) packet->payload.rsp_smp_get_pin_code.pin_code.length,
        packet->payload.rsp_smp_get_pin_code.pin_code.data);
    printf(", Result=");
    printHex16(packet->payload.rsp_smp_get_pin_code.result);
    printf("\r\n");
#endif
    break;

  case EZS_IDX_RSP_SMP_SET_PIN_CODE:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_smp_set_pin_code.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_smp_set_pin_code: Result=");
      printHex16(packet->payload.rsp_smp_set_pin_code.result);
      printf("\r\n");
    }
#endif
    break;

  case EZS_IDX_RSP_BT_GET_PARAMETERS:
    rsp_bt_get_parameters = packet->payload.rsp_bt_get_parameters;
    break;

  case EZS_IDX_RSP_BT_SET_PARAMETERS:
#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    if (packet->payload.rsp_bt_set_parameters.result != EZS_ERR_SUCCESS)
    {
      printf("RX: rsp_bt_set_parameters: Result=");
      printHex16(packet->payload.rsp_bt_set_parameters.result);
      printf("\r\n");
    }
#endif
    break;

    /* -------- Shimmer added end -------- */

  default:
    //#if ENABLE_BT_INIT_RX_DEBUG_PRINTS
    printf("RX: unhandled packet: ");
    printHex8(packet->header.group);
    printf("/");
    printHex8(packet->header.id);
    printf("\r\n");
    //#endif
    break;
  }

  //printf("\r\n");

  if (packet->tbl_index == expectedResponseIdx)
  {
    progressToNextBtInCmd();
  }
}

void progressToNextBtInCmd(void)
{
  if (btInitCmdsRunning || btFactoryResetCmdsRunning)
  {
    /* Set the expected response to be invalid */
    setExpectedResponse(EZS_IDX_EVT_MAX);
    /* Continue with the next stage */
    btInitCommands();
  }
}

void setExpectedResponse(uint16_t idx)
{
  expectedResponseIdx = idx;
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
  if (!state && ShimBt_getDataRateTestState())
  {
    ShimBt_setDataRateTestState(0);
    ShimBt_clearBtTxBuf(1);
  }
}

bool getBtCysppState(void)
{
  return btCysppState;
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

//TODO placeholder for now, implement this later
uint8_t BT_connect(uint8_t *addr)
{
  ezs_cmd_bt_connect_t bt_conn;
  clock_t start_time = clock();
  uint8_t addrReverse[6];

  /*//TODO how to distinguish between Master and slave
   if(bt_conn.type == MASTER)
   {
   memcpy(bt_conn.address.addr, addr, 6); //copying the MAC address
   }
   else if (bt_conn.type == SLAVE)
   {
   memset(bt_conn.address.addr, addr, 6);
   }*/

  addrReverse[0] = addr[5];
  addrReverse[1] = addr[4];
  addrReverse[2] = addr[3];
  addrReverse[3] = addr[2];
  addrReverse[4] = addr[1];
  addrReverse[5] = addr[0];

  memcpy(bt_conn.address.addr, addrReverse, 6);
  bt_conn.type = 1; //for SPP
  printf("Connecting to MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", addr[0], addr[1],
      addr[2], addr[3], addr[4], addr[5]);
  //connect

  //1 for SPP, other params are set to 0 as they are not implemented in the CYW20820 EZ-Serial firmware
  //ezs_cmd_gap_connect(&bt_conn.address.addr, 0, 0, 0, 0, 0, 0, CONNECTION_TIMEOUT_S);

  //setExpectedResponse(EZS_IDX_CMD_BT_CONNECT);

  uint8_t status = ezs_cmd_bt_connect(&bt_conn.address.addr, 1); //1 for SPP
  printf("Connection Status Code: %02X\n", status);

  /* Connection status codes :
 0x00 -> Success
 0x01 -> Failure
 0x02 -> Timeout
 0x03 -> Already connected
 else other error*/

  //while (1)
  //{
  //  if (status == 0) //Success case
  //  {
  //    printf("Connection success\n");
  //    active_conn_handle = 0;
  //    return active_conn_handle;
  //  }
  //  else if ((((clock() - start_time) * 1000 / CLOCKS_PER_SEC) > 5000))
  //  {
  //    printf("Connection failed\n");
  //    BT_connectionFailed(0xFF, 0x0008);
  //    active_conn_handle = 0xFF;
  //    return 0;
  //  }
  //  else
  //  {
  //    printf("Connection Status Code: %02X\n", status);
  //    return 0;
  //  }
  //}
  return active_conn_handle; //Feels unnecessary
}

uint8_t BT_disconnect(void)
{
  ezs_cmd_bt_disconnect_t bt_disc;
  uint8_t status;
  bt_disc.conn_handle = active_conn_handle; //this detects an active connection
  if (bt_disc.conn_handle == 0xFF)
  {
    printf("No active connection detected\n");
  }
  else
  {
    setExpectedResponse(EZS_IDX_CMD_BT_DISCONNECT);
    status = ezs_cmd_bt_disconnect(bt_disc.conn_handle); //status for debug to check if correct status code was achieved
  }
  return 0;
}

void BT_cancelConnection(void)
{
  uint8_t status;
  //expects a response of 11 bytes
  uint8_t response[11];
  setExpectedResponse(EZS_IDX_CMD_BT_CANCEL_CONNECTION);
  status = ezs_cmd_bt_cancel_connection();
  printf("Received response from BT cancel connection:\n");
  for (int i = 0; i < 11; i++)
  {
    printf("0x%02X ", response[i]);
  }
  printf("\n");
}

//TODO fix this
void BT_connectionFailed(uint8_t conn_handle, uint16_t reason)
{
  printf("Connection Failed! Conn Handle: %02X, Reason: %04X\n", conn_handle, reason);

  //Handling failure cases
  switch (reason)
  {
  case 0x0001:
    printf("Reason: Authentication Failed\n");
    break;
  case 0x0008:
    printf("Reason: Connection Timeout\n");
    break;
  case 0x0013:
    printf("Reason: Remote Device Terminated Connection\n");
    break;
  default:
    printf("Reason: Unknown Error\n");
    break;
  }
}

void BT_startDone_cb(void (*callback)(void))
{
  btIsInitialised_cb = callback;
}
