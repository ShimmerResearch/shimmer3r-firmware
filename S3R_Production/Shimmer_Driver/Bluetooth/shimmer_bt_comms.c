/*
 * shimmer_bt_comms.c
 *
 *  Created on: 22 Jun 2022
 *      Author: MarkNolan
 */

#include "shimmer_bt_comms.h"
#include <stdio.h>
#include <string.h>

#if defined(SHIMMER3)
#include "msp430.h"

#include "../../shimmer_btsd.h"
#include "../5xx_HAL/hal_CRC.h"
#include "../5xx_HAL/hal_RTC.h"
#include "../5xx_HAL/hal_board.h"
#include "RN4X.h"
#include "hal_CRC.h"
#include "shimmer_btsd.h"

#if BT_DMA_USED_FOR_RX
//#include "../5xx_HAL/hal_DMA.h"
#endif
#elif defined(SHIMMER3R) || defined(SHIMMER4_SDK)
#include "bmp3_defs.h"
#include "hal_FactoryTest.h"
#include "s4_sensing.h"
#include "s4_taskList.h"
#include "shimmer_definitions.h"
#include "shimmer_externs.h"
#endif

#include "sd_sync.h"

uint8_t unwrappedResponse[256] = { 0 };
#if !BT_DMA_USED_FOR_RX
char *commandBufPtr;
#endif
#if defined(SHIMMER3)
uint8_t *expectedResponsePtr;
#endif
#if BT_DMA_USED_FOR_RX
uint8_t args[MAX_COMMAND_ARG_SIZE], waitingForArgs, waitingForArgsLength, argsSize, gAction;
#if defined(SHIMMER3)
volatile uint8_t btStatusStrIndex;

volatile char btStatusStr[BT_STAT_STR_LEN_LARGEST + 1U]; /* +1 to always have a null char */
volatile char btRxBuffFullResponse[BT_VER_RESPONSE_LARGEST + 1U]; /* +1 to always have a null char */
#endif
//uint8_t btRxBuff[MAX_COMMAND_ARG_SIZE], *btRxExp;
uint8_t *btRxBuffPtr;
#else
RingFifoRx_t *gBtRxFifoPtr;
/* isRn4678CmdDetectedOnBoot is a workaround to know if it's RN42 or RN4678 before version is parsed */
uint8_t isRn4678CmdDetectedOnBoot = 0;
#endif
volatile COMMS_CRC_MODE btCrcMode;

uint8_t *gActionPtr;
uint8_t *gArgsPtr;

#if defined(SHIMMER3)
volatile char btVerStrResponse[BT_VER_RESPONSE_LARGEST + 1U]; /* +1 to always have a null char */
#else
char btVerStrResponse[100U]; //CYW20820 app=v01.04.16.16 requires size = 76 chars
#endif

uint8_t (*newBtCmdToProcess_cb)(void);
#if defined(SHIMMER3)
void (*handleBtRfCommStateChange_cb)(uint8_t);
#endif
void (*setMacId_cb)(uint8_t *);

uint16_t numBytesInBtRxBufWhenLastProcessed = 0;
uint16_t indexOfFirstEol;
uint32_t firstProcessFailTicks = 0;

uint8_t sendAck, inquiryResponse, samplingRateResponse, lnAccelCalibrationResponse,
    gyroCalibrationResponse, magCalibrationResponse, wrAccelCalibrationResponse,
    allCalibrationResponse, deviceVersionResponse, fwVersionResponse,
    bufferSizeResponse, uniqueSerialResponse, configSetupBytesResponse,
    wrAccelRangeResponse, magGainResponse, magSamplingRateResponse,
    wrAccelSamplingRateResponse, wrAccelLpModeResponse, wrAccelHrModeResponse,
    gyroRangeResponse, gyroSamplingRateResponse, altAccelRangeResponse,
    mpu9150MagSensAdjValsResponse, bmpOversamplingRatioResponse, blinkLedResponse,
    gsrRangeResponse, internalExpPowerEnableResponse, exgRegsResponse,
    dcIdResponse, dcMemResponse, dockedResponse, trialConfigResponse,
    centerResponse, shimmerNameResponse, expIDResponse, nshimmerResponse,
    myIDResponse, configTimeResponse, dirResponse, btCommsBaudRateResponse,
    derivedChannelResponse, infomemResponse, rwcResponse, btVbattResponse,
    calibRamResponse, btVerResponse, useAckPrefixForInstreamResponses,
    btDataRateTestResponse, bmpGenericCalibrationCoefficientsResponse;
uint8_t bmp180CalibrationCoefficientsResponse, bmp280CalibrationCoefficientsResponse;
#if defined(SHIMMER4_SDK)
uint8_t i2cvBattBtRsp;
#endif

uint8_t infomemLength, dcMemLength, calibRamLength;
uint16_t infomemOffset, dcMemOffset, calibRamOffset;

//ExG
uint8_t exgLength, exgChip, exgStartAddr; /*, exgForcedOff;*/

uint16_t btRxWaitByteCount = 0;

#if BT_DMA_USED_FOR_RX
/* Return of 1 brings MSP out of low-power mode */
//uint8_t Dma2ConversionDone(void)
uint8_t Dma2ConversionDone(uint8_t *rxBuff)
{
  btRxBuffPtr = rxBuff;
#if defined(SHIMMER3)
  uint8_t bt_waitForStartCmd, bt_waitForMacAddress, bt_waitForVersion,
      bt_waitForInitialBoot, bt_waitForReturnNewLine;
  uint8_t expectedlen = 0U;

  DMA2AndCtsDisable();
  bt_waitForStartCmd = BT_getWaitForStartCmd();
  bt_waitForMacAddress = BT_getWaitForMacAddress();
  bt_waitForVersion = BT_getWaitForVersion();
  bt_waitForInitialBoot = BT_getWaitForInitialBoot();
  bt_waitForReturnNewLine = BT_getWaitForReturnNewLine();

  if (!*btRxExp
      && (areBtStatusStringsEnabled()
          || (isBtConnected() || bt_waitForStartCmd || bt_waitForMacAddress || bt_waitForVersion
              || bt_waitForInitialBoot || bt_waitForReturnNewLine)))
  {
    if (bt_waitForStartCmd)
    {
      /* RN42 responds with "CMD\r\n" and RN4678 with "CMD> " */
      uint8_t len = strlen((char *) btRxBuffPtr);
      if (len == 5U && btRxBuffPtr[0] == 'C' && btRxBuffPtr[1] == 'M'
          && btRxBuffPtr[2] == 'D')
      {
        BT_setWaitForStartCmd(0);
        setCommandModeActive(1U);
        memset(btRxBuffPtr, 0, len);
        BT_setGoodCommand();
      }
    }
    else if (bt_waitForReturnNewLine)
    {
      uint8_t btOffset = strlen(btRxBuffFullResponse);
      memcpy(btRxBuffFullResponse + btOffset, btRxBuffPtr, strlen((char *) btRxBuffPtr));
      memset(btRxBuffPtr, 0, strlen((char *) btRxBuffPtr));

      uint8_t responseLen = strlen(btRxBuffFullResponse);

      if (btRxBuffFullResponse[responseLen - 1U] == '\r')
      {
        /* Wait for "\n" */
        setDmaWaitingForResponse(1U);
        return 0;
      }
      //Wait for RN4678_CMD
      else if (isBtDeviceRn4678() && responseLen > RN4X_CMD_LEN
          && btRxBuffFullResponse[responseLen - 5U] == 'C'
          && btRxBuffFullResponse[responseLen - 4U] == 'M'
          && btRxBuffFullResponse[responseLen - 3U] == 'D'
          && btRxBuffFullResponse[responseLen - 2U] == '>'
          && btRxBuffFullResponse[responseLen - 1U] == ' ')
      {
        /* Return/New line received */
        BT_setWaitForReturnNewLine(0);
        BT_setGoodCommand();
      }
      else if (btRxBuffFullResponse[responseLen - 2U] == '\r'
          && btRxBuffFullResponse[responseLen - 1U] == '\n')
      {
        if (isBtDeviceRn41orRN42())
        {
          BT_setWaitForReturnNewLine(0);
          BT_setGoodCommand();
        }
        else
        {
          /* Wait for "CMD> " */
          setDmaWaitingForResponse(RN4X_CMD_LEN);
          return 0;
        }
      }
      else
      {
        /* Wait for "\r\n" */
        setDmaWaitingForResponse(2U);
        return 0;
      }
    }
    else if (bt_waitForInitialBoot)
    {
      if (isBtDeviceRn4678())
      {
        expectedlen = BT_STAT_STR_LEN_RN4678_REBOOT;
      }
      else
      {
        expectedlen = BT_STAT_STR_LEN_RN42_REBOOT;
      }
      memset(btRxBuffPtr, 0, expectedlen);
      BT_setWaitForInitialBoot(0);
      BT_setGoodCommand();
    }
    else if (bt_waitForMacAddress)
    {
      setMacId_cb(btRxBuffPtr);

      expectedlen = 14U;
      if (isBtDeviceRn4678())
      {
        expectedlen += RN4X_CMD_LEN; /* Allow for "CMD> " */
      }
      memset(btRxBuffPtr, 0, expectedlen);
      BT_setWaitForMacAddress(0);
      BT_setGoodCommand();
    }
    else if (bt_waitForVersion)
    {
      uint8_t btVerRemainingChars = 0;
      uint8_t btOffset = strlen(btRxBuffFullResponse);
      memcpy(btRxBuffFullResponse + btOffset, btRxBuffPtr, strlen((char *) btRxBuffPtr));
      memset(btRxBuffPtr, 0, strlen((char *) btRxBuffPtr));

      uint8_t btVerLen = strlen(btRxBuffFullResponse);
      enum BT_FIRMWARE_VERSION btFwVerNew = BT_FW_VER_UNKNOWN;

      /* RN41 or RN42 */
      if (btRxBuffFullResponse[0U] == 'V')
      {
        /* RN41_VERSION_RESPONSE_V4_77 or RN42_VERSION_RESPONSE_V4_77 */
        if (btRxBuffFullResponse[4U] == '4' && btRxBuffFullResponse[5U] == '.')
        {
          if (btRxBuffFullResponse[9U] == 'R' && btRxBuffFullResponse[10U] == 'N')
          {
            btVerRemainingChars = RN42_VERSION_RESPONSE_LEN_V4_77;
            btFwVerNew = RN42_V4_77;
          }
          else
          {
            btVerRemainingChars = RN41_VERSION_RESPONSE_LEN_V4_77;
            btFwVerNew = RN41_V4_77;
          }
        }
        /* RN42_VERSION_RESPONSE_V6_15 */
        else if (btRxBuffFullResponse[4U] == '6' && btRxBuffFullResponse[5U] == '.'
            && btRxBuffFullResponse[6U] == '1' && btRxBuffFullResponse[7U] == '5')
        {
          btVerRemainingChars = RN42_VERSION_RESPONSE_LEN_V6_15;
          btFwVerNew = RN42_V6_15;
        }
        /* V6.30 not supported */
        else if (btRxBuffFullResponse[4U] == '6' && btRxBuffFullResponse[5U] == '.'
            && btRxBuffFullResponse[6U] == '3' && btRxBuffFullResponse[7U] == '0')
        {
          triggerShimmerErrorState();
        }
      }
      /* RN4678 */
      else if (btRxBuffFullResponse[0U] == 'R')
      {
        /* RN4678_VERSION_RESPONSE_V1_00_5 */
        if (btRxBuffFullResponse[10U] == '0' && btRxBuffFullResponse[11U] == '0')
        {
          btVerRemainingChars = RN4678_VERSION_LEN_V1_00_5;
          btFwVerNew = RN4678_V1_00_5;
        }
        /* RN4678_VERSION_RESPONSE_V1_11_0 */
        else if (btRxBuffFullResponse[10U] == '1' && btRxBuffFullResponse[11U] == '1')
        {
          btVerRemainingChars = RN4678_VERSION_LEN_V1_11_0;
          btFwVerNew = RN4678_V1_11_0;
        }
        /* RN4678_VERSION_RESPONSE_V1_13_5 */
        else if (btRxBuffFullResponse[10U] == '1' && btRxBuffFullResponse[11U] == '3')
        {
          btVerRemainingChars = RN4678_VERSION_LEN_V1_13_5;
          btFwVerNew = RN4678_V1_13_5;
        }
        /* RN4678_VERSION_RESPONSE_V1_22_0 */
        else if (btRxBuffFullResponse[10U] == '2' && btRxBuffFullResponse[11U] == '2')
        {
          btVerRemainingChars = RN4678_VERSION_LEN_V1_22_0;
          btFwVerNew = RN4678_V1_22_0;
        }
        /* RN4678_VERSION_RESPONSE_V1_23_0 */
        else if (btRxBuffFullResponse[10U] == '2' && btRxBuffFullResponse[11U] == '3')
        {
          btVerRemainingChars = RN4678_VERSION_LEN_V1_23_0;
          btFwVerNew = RN4678_V1_23_0;
        }
      }
      else
      {
        /* Unkown BT module - bail */
        btVerRemainingChars = btVerLen;
      }

      btVerRemainingChars -= btVerLen;

      if (btVerRemainingChars)
      {
        setDmaWaitingForResponse(btVerRemainingChars);
        return 0;
      }
      else
      {
        setBtFwVersion(btFwVerNew);

        /* When storing the BT version, ignore from "\r" onwards */
        uint8_t btVerLen = strlen(btRxBuffFullResponse);
        uint8_t btVerIdx;
        for (btVerIdx = 0; btVerIdx < btVerLen; btVerIdx++)
        {
          if (btRxBuffFullResponse[btVerIdx] == '\r')
          {
            btVerLen = btVerIdx;
            break;
          }
        }
        memcpy(btVerStrResponse, btRxBuffFullResponse, btVerLen);

        memset(btRxBuffFullResponse, 0, strlen((char *) btRxBuffFullResponse));
        BT_setWaitForVersion(0);
        BT_setGoodCommand();
      }
    }
#if USE_OLD_SD_SYNC_APPROACH
    else if (getRcommVar())
    {
      /* SD Sync Center - get's into this case when the center is waiting for a 0x01 or 0xFF from a node */
      //1 byte of RC command
      setRcommResp(btRxBuffPtr, 1U);
      setRcommVar(0);
      setDmaWaitingForResponse(1U);
      return TaskSet(TASK_RCCENTERR1);
    }
#endif
    else
    {
#endif
      if (waitingForArgs)
      {
        if ((!waitingForArgsLength) && (waitingForArgs == 3)
            && (*(gActionPtr) == SET_INFOMEM_COMMAND || *(gActionPtr) == SET_CALIB_DUMP_COMMAND
                || *(gActionPtr) == SET_DAUGHTER_CARD_MEM_COMMAND
                || *(gActionPtr) == SET_EXG_REGS_COMMAND))
        {
          gArgsPtr[0] = btRxBuffPtr[0];
          gArgsPtr[1] = btRxBuffPtr[1];
          gArgsPtr[2] = btRxBuffPtr[2];
          if (*(gActionPtr) == SET_EXG_REGS_COMMAND)
          {
            waitingForArgsLength = gArgsPtr[2];
          }
          else
          {
            waitingForArgsLength = gArgsPtr[0];
          }
          setDmaWaitingForResponse(waitingForArgsLength);
          return 0;
        }
        else if ((!waitingForArgsLength) && (waitingForArgs == 2)
            && (*(gActionPtr) == SET_DAUGHTER_CARD_ID_COMMAND))
        {
          gArgsPtr[0] = btRxBuffPtr[0];
          gArgsPtr[1] = btRxBuffPtr[1];
          if (gArgsPtr[0])
          {
            waitingForArgsLength = gArgsPtr[0];
            setDmaWaitingForResponse(waitingForArgsLength);
          }
          return 0;
        }
        else if ((!waitingForArgsLength) && (waitingForArgs == 1)
            && (*(gActionPtr) == SET_CENTER_COMMAND || *(gActionPtr) == SET_CONFIGTIME_COMMAND
                || *(gActionPtr) == SET_EXPID_COMMAND || *(gActionPtr) == SET_SHIMMERNAME_COMMAND))
        {
          gArgsPtr[0] = btRxBuffPtr[0];
          if (gArgsPtr[0])
          {
            waitingForArgsLength = gArgsPtr[0];
            setDmaWaitingForResponse(waitingForArgsLength);
            return 0;
          }
        }

#if defined(SHIMMER3)
        else if (*(gActionPtr) == RN4678_STATUS_STRING_SEPARATOR)
        {
          return parseRn4678Status();
        }
#endif

        else if (*(gActionPtr) == ACK_COMMAND_PROCESSED)
        {
#if USE_OLD_SD_SYNC_APPROACH
          /* Store local time as early as possible after sync bytes have been received */
          saveLocalTime();
#else
      /* If waiting for command byte */
      if (!waitingForArgsLength)
      {
        /* Save command byte */
        gArgsPtr[0] = btRxBuffPtr[0];

        if (btRxBuffPtr[0] == SD_SYNC_RESPONSE)
        {
          /* Wait for flag to be received */
          waitingForArgsLength = 1U;
          setDmaWaitingForResponse(waitingForArgsLength);
          return 0;
        }
      }
#endif
        }

        if (waitingForArgsLength)
        {
          memcpy(gArgsPtr + waitingForArgs, btRxBuffPtr, waitingForArgsLength);
        }
        else
        {
          memcpy(gArgsPtr, btRxBuffPtr, waitingForArgs);
        }

        waitingForArgsLength = 0;
        waitingForArgs = 0;
        argsSize = 0;
        setDmaWaitingForResponse(1U);
        if (newBtCmdToProcess_cb)
        {
          return newBtCmdToProcess_cb();
        }
        else
        {
          return 1;
        }
      }
      else
      {
        uint8_t data = btRxBuffPtr[0];
        uint8_t wakeupMcu = 0;

        switch (data)
        {
#if FW_IS_LOGANDSTREAM
        case INQUIRY_COMMAND:
        case DUMMY_COMMAND:
        case GET_SAMPLING_RATE_COMMAND:
        case TOGGLE_LED_COMMAND:
        case START_STREAMING_COMMAND:
        case GET_STATUS_COMMAND:
        case GET_VBATT_COMMAND:
        case GET_TRIAL_CONFIG_COMMAND:
        case START_SDBT_COMMAND:
        case GET_CONFIG_SETUP_BYTES_COMMAND:
        case STOP_STREAMING_COMMAND:
        case STOP_SDBT_COMMAND:
        case START_LOGGING_COMMAND:
        case STOP_LOGGING_COMMAND:
        case GET_LN_ACCEL_CALIBRATION_COMMAND:
        case GET_GYRO_CALIBRATION_COMMAND:
        case GET_MAG_CALIBRATION_COMMAND:
        case GET_WR_ACCEL_CALIBRATION_COMMAND:
        case GET_GSR_RANGE_COMMAND:
        case GET_ALL_CALIBRATION_COMMAND:
        case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
        case GET_DEVICE_VERSION_COMMAND:
        case GET_FW_VERSION_COMMAND:
        case GET_CHARGE_STATUS_LED_COMMAND:
        case GET_BUFFER_SIZE_COMMAND:
        case GET_UNIQUE_SERIAL_COMMAND:
        case GET_WR_ACCEL_RANGE_COMMAND:
        case GET_MAG_GAIN_COMMAND:
        case GET_MAG_SAMPLING_RATE_COMMAND:
        case GET_WR_ACCEL_SAMPLING_RATE_COMMAND:
        case GET_WR_ACCEL_LPMODE_COMMAND:
        case GET_WR_ACCEL_HRMODE_COMMAND:
        case GET_GYRO_RANGE_COMMAND:
        case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
        case GET_BMP280_CALIBRATION_COEFFICIENTS_COMMAND:
        case GET_PRESSURE_CALIBRATION_COEFFICIENTS_COMMAND:
        case GET_GYRO_SAMPLING_RATE_COMMAND:
        case GET_ALT_ACCEL_RANGE_COMMAND:
        case GET_PRESSURE_OVERSAMPLING_RATIO_COMMAND:
        case GET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
        case RESET_TO_DEFAULT_CONFIGURATION_COMMAND:
        case RESET_CALIBRATION_VALUE_COMMAND:
        case GET_MPU9150_MAG_SENS_ADJ_VALS_COMMAND:
        case GET_BT_COMMS_BAUD_RATE:
        case GET_CENTER_COMMAND:
        case GET_SHIMMERNAME_COMMAND:
        case GET_EXPID_COMMAND:
        case GET_MYID_COMMAND:
        case GET_NSHIMMER_COMMAND:
        case GET_CONFIGTIME_COMMAND:
        case GET_DIR_COMMAND:
        case GET_DERIVED_CHANNEL_BYTES:
        case GET_RWC_COMMAND:
        case UPD_SDLOG_CFG_COMMAND:
        case UPD_CALIB_DUMP_COMMAND:
          //case UPD_FLASH_COMMAND:
        case GET_BT_VERSION_STR_COMMAND:
          *(gActionPtr) = data;
          if (newBtCmdToProcess_cb)
          {
            newBtCmdToProcess_cb();
          }
          setDmaWaitingForResponse(1U);
          /* Wake-up MCU so that the get command can be processed */
          wakeupMcu = 1U;
          break;
        case SET_WR_ACCEL_RANGE_COMMAND:
        case SET_WR_ACCEL_SAMPLING_RATE_COMMAND:
        case SET_MAG_GAIN_COMMAND:
        case SET_CHARGE_STATUS_LED_COMMAND:
        case SET_MAG_SAMPLING_RATE_COMMAND:
        case SET_WR_ACCEL_LPMODE_COMMAND:
        case SET_WR_ACCEL_HRMODE_COMMAND:
        case SET_GYRO_RANGE_COMMAND:
        case SET_GYRO_SAMPLING_RATE_COMMAND:
        case SET_ALT_ACCEL_RANGE_COMMAND:
        case SET_PRESSURE_OVERSAMPLING_RATIO_COMMAND:
        case SET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
        case SET_GSR_RANGE_COMMAND:
        case SET_BT_COMMS_BAUD_RATE:
        case SET_CENTER_COMMAND:
        case SET_SHIMMERNAME_COMMAND:
        case SET_EXPID_COMMAND:
        case SET_MYID_COMMAND:
        case SET_NSHIMMER_COMMAND:
        case SET_CONFIGTIME_COMMAND:
        case SET_CRC_COMMAND:
        case SET_INSTREAM_RESPONSE_ACK_PREFIX_STATE:
        case SET_DATA_RATE_TEST:
        case SET_FACTORY_TEST:
          *(gActionPtr) = data;
          waitingForArgs = 1U;
          break;
        case SET_SAMPLING_RATE_COMMAND:
        case GET_DAUGHTER_CARD_ID_COMMAND:
        case SET_DAUGHTER_CARD_ID_COMMAND:
          *(gActionPtr) = data;
          waitingForArgs = 2U;
          break;
        case SET_SENSORS_COMMAND:
        case GET_EXG_REGS_COMMAND:
        case SET_EXG_REGS_COMMAND:
        case GET_DAUGHTER_CARD_MEM_COMMAND:
        case SET_DAUGHTER_CARD_MEM_COMMAND:
        case SET_TRIAL_CONFIG_COMMAND:
        case GET_INFOMEM_COMMAND:
        case SET_INFOMEM_COMMAND:
        case GET_CALIB_DUMP_COMMAND:
        case SET_CALIB_DUMP_COMMAND:
          *(gActionPtr) = data;
          waitingForArgs = 3U;
          break;
        case SET_CONFIG_SETUP_BYTES_COMMAND:
          *(gActionPtr) = data;
          waitingForArgs = 4U;
          break;
        case SET_RWC_COMMAND:
        case SET_DERIVED_CHANNEL_BYTES:
          *(gActionPtr) = data;
          waitingForArgs = 8U;
          break;
        case SET_LN_ACCEL_CALIBRATION_COMMAND:
        case SET_GYRO_CALIBRATION_COMMAND:
        case SET_MAG_CALIBRATION_COMMAND:
        case SET_WR_ACCEL_CALIBRATION_COMMAND:
          *(gActionPtr) = data;
          waitingForArgs = 21U;
          break;
#endif
#if defined(SHIMMER3)
        case RN4678_STATUS_STRING_SEPARATOR:
          memset(btStatusStr, 0, sizeof(btStatusStr));
          btStatusStr[0U] = RN4678_STATUS_STRING_SEPARATOR;
          btStatusStrIndex = 1U;
          *(gActionPtr) = data;
          /* Minus 1 because we've already received 1 x RN4678_STATUS_STRING_SEPARATOR */
          waitingForArgs = BT_STAT_STR_LEN_SMALLEST - 1U;
          break;
#endif
#if USE_OLD_SD_SYNC_APPROACH
        case ACK_COMMAND_PROCESSED:
          /* SD Sync Node - gets here when a node receives a sync packet from a center */
          *(gActionPtr) = data;
          waitingForArgs = SYNC_PACKET_PAYLOAD_SIZE;
          break;
#else
    case ACK_COMMAND_PROCESSED:
      /* Wait for command byte */
      *(gActionPtr) = data;
      waitingForArgs = 1U;
      break;
    case SET_SD_SYNC_COMMAND:
      /* Store local time as early as possible after sync bytes have been received */
      saveLocalTime();

      *(gActionPtr) = data;
      waitingForArgs = SYNC_PACKET_PAYLOAD_SIZE + BT_SD_SYNC_CRC_MODE;
      break;
#endif
        default:
          setDmaWaitingForResponse(1U);
          break;
        }

        if (waitingForArgs)
        {
          setDmaWaitingForResponse(waitingForArgs);
        }

        return wakeupMcu;
      }
#if defined(SHIMMER3)
    }
  }
  else
  {
    uint8_t len = strlen((char *) btRxExp);
    if (!memcmp(btRxBuffPtr, btRxExp, len))
    {
      memset(btRxBuffPtr, 0, len);
      BT_setGoodCommand();
    }
    else
    {
      _NOP(); //bad command trap: reaching here = serious BT problem
    }
  }

  setDmaWaitingForResponseIfStatusStrEnabled();
#endif
  return 0;
}

#if defined(SHIMMER3)
uint8_t parseRn4678Status(void)
{
  uint8_t numberOfCharRemaining = 0U;
  uint8_t bringUcOutOfSleep = 0U;

  memcpy(btStatusStr + btStatusStrIndex, btRxBuffPtr, waitingForArgs);
  memset(btRxBuffPtr, 0, waitingForArgs);
  btStatusStrIndex += waitingForArgs;

  enum BT_FIRMWARE_VERSION btFwVer = getBtFwVersion();

  uint8_t firstChar = btStatusStr[1U];
  switch (firstChar)
  {
  case 'A':
    /* "%AUTHENTICATED%" */
    if (btStatusStr[14U] == '%')
    {
      /* TODO */
    }
    /* "%AUTHEN" - Read outstanding bytes */
    else if (btStatusStr[6U] == 'N')
    {
      numberOfCharRemaining = BT_STAT_STR_LEN_AUTHENTICATED - BT_STAT_STR_LEN_SMALLEST;
    }
    /* "%AUTH_FAIL%" */
    else if (btStatusStr[10U] == '%')
    {
      /* TODO */
    }
    /* "%AUTH_FA" - Read outstanding bytes */
    else if (btStatusStr[6U] == 'F')
    {
      numberOfCharRemaining = BT_STAT_STR_LEN_AUTH_FAIL - BT_STAT_STR_LEN_SMALLEST;
    }
    break;
  case 'B':
    /* "%BONDED%" */
    if (btStatusStr[7U] == '%')
    {
      /* TODO */
    }
    /* "%BONDED" - Read outstanding bytes */
    else
    {
      numberOfCharRemaining = BT_STAT_STR_LEN_BONDED - BT_STAT_STR_LEN_SMALLEST;
    }
    break;
  case 'C':
    if (btStatusStr[5U] == 'E')
    {
      /* "%CONNECT,001BDC06A3D5%" - RN4678 */
      if (btStatusStr[21U] == '%')
      {
        setRn4678ConnectionState(RN4678_CONNECTED_CLASSIC);
      }
      /* "%CONNECT" for RN42 v4.77 or "%CONNECT,001BDC06A3D5," - RN42 v6.15 */
      else if ((btFwVer == RN41_V4_77 && btStatusStr[7U] == 'T')
          || (btFwVer == RN42_V4_77 && btStatusStr[7U] == 'T')
          || (btFwVer == RN42_V6_15 && btStatusStr[21U] == ','))
      {
        triggerBtRfCommStateChangeCallback(TRUE);
        bringUcOutOfSleep = 1U;
      }
      else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
      {
        if (btFwVer == RN41_V4_77 || btFwVer == RN42_V4_77)
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_RN42_v477_CONNECT;
        }
        else if (btFwVer == RN42_V6_15)
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_RN42_v615_CONNECT;
        }
        else
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_RN4678_CONNECT;
        }
        numberOfCharRemaining -= BT_STAT_STR_LEN_SMALLEST;
      }
    }
    else if (btStatusStr[5U] == '_')
    {
      /* "%CONN_PARAM,000C,0000,03C0%" - RN4678 */
      if (btStatusStr[26U] == '%')
      {
        //TODO
      }
      else
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_CONN_PARAM - BT_STAT_STR_LEN_SMALLEST;
      }
    }
    break;
  case 'D':
    /* "%DISCONN%" -> RN4678 */
    if (btStatusStr[8U] == '%')
    {
      /* This if is needed here for BLE connections as a
       * disconnect over BLE does not trigger an
       * RFCOMM_CLOSE status change as it does for classic
       * Bluetooth connections. */
      if (isBtConnected())
      {
        triggerBtRfCommStateChangeCallback(FALSE);
        bringUcOutOfSleep = 1U;
      }

      setRn4678ConnectionState(RN4678_DISCONNECTED);
    }
    /* "%DISCONNECT" -> RN42 */
    else if (btStatusStr[10U] == 'T')
    {
      triggerBtRfCommStateChangeCallback(FALSE);
      bringUcOutOfSleep = 1U;
    }
    /* "%DISCON" - Read outstanding bytes */
    else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
    {
      if (isBtDeviceRn41orRN42())
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_RN42_DISCONNECT;
      }
      else
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_RN4678_DISCONN;
      }
      numberOfCharRemaining -= BT_STAT_STR_LEN_SMALLEST;
    }
    break;
  case 'E':
    if (btStatusStr[2U] == 'N')
    {
      if (btStatusStr[5U] == 'I')
      {
        /* "%END_INQ%" */
        if (btStatusStr[8U] == '%')
        {
          /* TODO */
        }
        /* "%END_IN" - Read outstanding bytes */
        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_END_INQ - BT_STAT_STR_LEN_SMALLEST;
        }
      }
      else if (btStatusStr[5U] == 'S')
      {
        /* "%END_SCN%" */
        if (btStatusStr[8U] == '%')
        {
          /* TODO */
        }
        /* "%END_SC" - Read outstanding bytes */
        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_END_SCN - BT_STAT_STR_LEN_SMALLEST;
        }
      }
    }
    else if (btStatusStr[2U] == 'R')
    {
      if (btStatusStr[5U] == 'C')
      {
        /* %ERR_CON is common to two status strings. If detected, read two more chars to determine which one it is */
        /* "%ERR_CONN%" */
        if (btStatusStr[9U] == '%')
        {
          /* TODO */
        }
        /* "%ERR_CONN_PARAM%" */
        else if (btStatusStr[15U] == '%')
        {
          /* TODO */
        }
        /* "%ERR_CONN_" - Read outstanding bytes */
        else if (btStatusStr[9U] == '_')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_ERR_CONN_PARAM - BT_STAT_STR_LEN_ERR_CONN;
        }
        /* "%ERR_CON" - Read outstanding bytes */
        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_ERR_CONN - BT_STAT_STR_LEN_SMALLEST;
        }
      }
      else if (btStatusStr[5U] == 'L')
      {
        /* "%ERR_LSEC%" */
        if (btStatusStr[9U] == '%')
        {
          /* TODO */
        }
        /* "%ERR_LSE" - Read outstanding bytes */
        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_ERR_LSEC - BT_STAT_STR_LEN_SMALLEST;
        }
      }
      else if (btStatusStr[5U] == 'S')
      {
        /* "%ERR_SEC%" */
        if (btStatusStr[8U] == '%')
        {
          /* TODO */
        }
        /* "%ERR_SE" - Read outstanding bytes */
        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_ERR_SEC - BT_STAT_STR_LEN_SMALLEST;
        }
      }
    }
    break;
  case 'F':
    /* "%FACTORY_RESET%" */
    if (btStatusStr[14U] == '%')
    {
      /* TODO */
    }
    /* "%FACTOR" - Read outstanding bytes */
    else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
    {
      numberOfCharRemaining = BT_STAT_STR_LEN_FACTORY_RESET - BT_STAT_STR_LEN_SMALLEST;
    }
    break;
  case 'L':
    if (btStatusStr[2U] == 'C')
    {
      /* "%LCONNECT,001BDC06A3D5,1%" - RN4678 BLE mode */
      if (btStatusStr[24U] == '%')
      {
        setRn4678ConnectionState(RN4678_CONNECTED_BLE);

        /* RN4678 seems to assume charactertic is advice once BLE connected */
        triggerBtRfCommStateChangeCallback(TRUE);

        bringUcOutOfSleep = 1U;
      }
      else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_RN4678_LCONNECT - BT_STAT_STR_LEN_SMALLEST;
      }
      break;
    }
    else if (btStatusStr[2U] == 'B')
    {
      /* "%LBONDED%" */
      if (btStatusStr[8U] == '%')
      {
        /* TODO */
      }
      /* "%LBONDE" - Read outstanding bytes */
      else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_LBONDED - BT_STAT_STR_LEN_SMALLEST;
      }
    }
    else if (btStatusStr[2U] == 'S')
    {
      if (btStatusStr[6U] == 'R')
      {
        /* "%LSECURED%" */
        if (btStatusStr[9U] == '%')
        {
          /* TODO */
        }
        /* "%LSECURE_FAIL%" */
        else if (btStatusStr[13U] == '%')
        {
          /* TODO */
        }
        /* "%LSECURE_F" */
        else if (btStatusStr[9U] == 'F')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_LSECURE_FAIL - BT_STAT_STR_LEN_LSECURED;
        }
        /* "%LSECUR" - Read outstanding bytes */
        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_LSECURED - BT_STAT_STR_LEN_SMALLEST;
        }
      }
      else if (btStatusStr[6U] == 'A')
      {
        /* "%LSTREAM_OPEN%" */
        if (btStatusStr[13U] == '%')
        {
          /* TODO */
        }
        /* "%LSTREA" - Read outstanding bytes */
        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_LSTREAM_OPEN - BT_STAT_STR_LEN_SMALLEST;
        }
      }
    }
    break;
  case 'M':
    /* "%MLDP_MODE%" */
    if (btStatusStr[10U] == '%')
    {
      /* TODO */
    }
    /* "%MLDP_M" - Read outstanding bytes */
    else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
    {
      numberOfCharRemaining = BT_STAT_STR_LEN_MLDP_MODE - BT_STAT_STR_LEN_SMALLEST;
    }
    break;
  case 'R':
    if (btStatusStr[2U] == 'E')
    {
      /* "%REBOOT%" -> RN4678 */
      if (btStatusStr[7U] == '%')
      {
        /* TODO */
        _NOP();
      }
      else
      {
        if (isBtDeviceRn41orRN42())
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_RN42_REBOOT;
        }
        else
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_RN4678_REBOOT;
        }
        numberOfCharRemaining -= BT_STAT_STR_LEN_SMALLEST;
      }
    }
    else if (btStatusStr[2U] == 'F')
    {
      if (btStatusStr[8U] == 'C')
      {
        /* "%RFCOMM_CLOSE%" */
        if (btStatusStr[13U] == '%')
        {
          triggerBtRfCommStateChangeCallback(FALSE);
          bringUcOutOfSleep = 1U;
        }
        /* "%RFCOMM_CLOSE" - Read outstanding bytes */
        else if (btStatusStr[13U] == '\0')
        {
          numberOfCharRemaining = BT_STAT_STR_LEN_RFCOMM_CLOSE - BT_STAT_STR_LEN_RFCOMM_OPEN;
        }
      }
      /* "%RFCOMM_OPEN%" */
      else if (btStatusStr[12U] == '%')
      {
        triggerBtRfCommStateChangeCallback(TRUE);
        bringUcOutOfSleep = 1U;
      }
      /* "%RFCOMM" - Read outstanding bytes */
      else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_RFCOMM_OPEN - BT_STAT_STR_LEN_SMALLEST;
      }
    }
    break;
  case 'S':
    if (btStatusStr[3U] == 'C')
    {
      /* "%SECURED%" */
      if (btStatusStr[8U] == '%')
      {
        /* TODO */
      }
      /* "%SECURE_FAIL%" */
      else if (btStatusStr[12U] == '%')
      {
        /* TODO */
      }
      /* "%SECURE_F" - Read outstanding bytes */
      else if (btStatusStr[7U] == '_')
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_SECURE_FAIL - BT_STAT_STR_LEN_SECURED;
      }
      /* "%SECURE" - Read outstanding bytes */
      else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_SECURED - BT_STAT_STR_LEN_SMALLEST;
      }
    }
    else if (btStatusStr[3U] == 'S')
    {
      /* "%SESSION_OPEN%" */
      if (btStatusStr[13U] == '%')
      {
        /* TODO */
      }
      /* "%SESSION_CLOSE%" */
      else if (btStatusStr[14U] == '%')
      {
        /* TODO */
      }
      /* "%SESSION_CLOSE" - Read outstanding bytes */
      else if (btStatusStr[13U] == 'E')
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_SESSION_CLOSE - BT_STAT_STR_LEN_SESSION_OPEN;
      }
      /* "%SESSIO" - Read outstanding bytes */
      else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST] == '\0')
      {
        numberOfCharRemaining = BT_STAT_STR_LEN_SESSION_OPEN - BT_STAT_STR_LEN_SMALLEST;
      }
    }
    break;
  default:
    break;
  }

  if (numberOfCharRemaining)
  {
    waitingForArgs = numberOfCharRemaining;
  }
  else
  {
    waitingForArgs = 0;
    numberOfCharRemaining = 1U;
  }
  setDmaWaitingForResponse(numberOfCharRemaining);
  return bringUcOutOfSleep;
}
#endif

void resetBtRxVariablesOnConnect(void)
{
  /* Reset to unsupported command */
  *(gActionPtr) = ACK_COMMAND_PROCESSED - 1U;
  waitingForArgs = 0;
  waitingForArgsLength = 0;
}

//void resetBtRxBuff(void)
//{
//    memset(btRxBuffPtr, 0, sizeof(btRxBuffPtr));
//}

#else
void processBtUartBuf(void)
{
  uint8_t responseParsed;
  uint8_t firstByteInRxBuf;

  /* Continue parsing buffer until we've parsed everything that we can from it */
  do
  {
    responseParsed = 0;
    firstByteInRxBuf = getRxByteAtIndex(0);
    updateNumBytesInBtRxBufWhenLastProcessed();
    indexOfFirstEol = 0;

    if (numBytesInBtRxBufWhenLastProcessed)
    {
      /* Response from BT Module command mode */
      if (wasStartBtCmdModeSentAndReponseReceived())
      {
        processStartRnCmdResponse();
        responseParsed = 1U;
      }
      else if (isRnCommandModeActive()
          && (indexOfFirstEol = isNewLineDetectedInBtRxBuf()) > 0)
      {
        /* isRn4678CmdDetectedOnBoot and isBtDeviceUnknown() are needed
         * here because the first command that is sent is to get the
         * version number and so until the response is parsed, BtDevice
         * has not been set yet */
        if (((isBtDeviceRn4678() || isRn4678CmdDetectedOnBoot) && isFullRN4678CmdResponseReceived())
            || isBtDeviceRn41orRN42() || isBtDeviceUnknown())
        {
          responseParsed = processRnCmdResponse();
        }
      }
      /* Status responses begin with '%' */
      else if (firstByteInRxBuf == RN4678_STATUS_STRING_SEPARATOR
          && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SMALLEST)
      {
        responseParsed = processStatusString();
      }
      /* Shimmer command */
      else if (!areBtSetupCommandsRunning() && isShimmerBtCmd(firstByteInRxBuf))
      {
        responseParsed = processShimmerBtCmd();
      }

      /* Update reference time so that timeout can be detected */
      /* Don't enable timeout until BT is being initialised */
      if (isBtStarting() || responseParsed || getNumBytesInBtRxBuf() == 0)
      {
        /* Reset time if successfully parsed or buffer is empty */
        firstProcessFailTicks = 0;
      }
      else if (firstProcessFailTicks == 0)
      {
        /* Set the time when the first fail to parse occurred */
        firstProcessFailTicks = RTC_get32();
      }
    }

  } while (responseParsed == 1U);
}

/* TODO Status string check here only supports RN4678 responses at the moment but this is fine because we have them turned off for the RN42 */
/* TODO If it's the end % missing, this code will currently remove it from the start of the next status string if there's one in the buffer */
void handleBtRxTimeout(void)
{
  uint8_t numberOfCharToRemove = 0U;

  if ((numberOfCharToRemove = isBtRxBufLike("%RFCOMM_CLOSE%", 1)) > 0)
  {
    triggerBtRfCommStateChangeCallback(0);
  }
  else if ((numberOfCharToRemove = isBtRxBufLike("%RFCOMM_OPEN%", 1)) > 0)
  {
    triggerBtRfCommStateChangeCallback(1U);
  }
  else if ((numberOfCharToRemove = isBtRxBufLike("%CONNECT,XXXXXXXXXXXX%", 1)) > 0)
  {
    setRn4678ConnectionState(RN4678_CONNECTED_CLASSIC);
  }
  else if ((numberOfCharToRemove = isBtRxBufLike("%DISCONN%", 1)) > 0)
  {
    setRn4678ConnectionState(RN4678_DISCONNECTED);
  }
  else
  {
    //Still unable to parse puffer, clear the first byte
    numberOfCharToRemove = 1U;
  }

  clearBytesFromBtRxBuf(numberOfCharToRemove);

  /* Need to update this variable to indicate to main to process new bytes when they come in */
  updateNumBytesInBtRxBufWhenLastProcessed();

  /* Reset process fail time */
  firstProcessFailTicks = 0;
}

uint8_t hasBtRxTimeoutOccurred(void)
{
  return (firstProcessFailTicks != 0
      && ((RTC_get32() - firstProcessFailTicks) > BT_RX_COMMS_TIMEOUT_TICKS));
}

void processStartRnCmdResponse(void)
{
  //clearBytesFromBtRxBuf(RN4X_CMD_LEN);

  uint8_t buf[RN4X_CMD_LEN], i;
  for (i = 0; i < RN4X_CMD_LEN; i++)
  {
    readByteFromBtRxBuf(&buf[i]);
  }

  /* RN42 returns "CMD\r\n", RN4678 returns "CMD> " */
  if (buf[3] == '>' && buf[4] == ' ')
  {
    isRn4678CmdDetectedOnBoot = 1U;
  }

  setCommandModeActive(1U);
  clearBtCmdTxRxBuffsAndProceed();
}

uint8_t wasStartBtCmdModeSentAndReponseReceived(void)
{
  return (commandBufPtr[0] == '$' && commandBufPtr[1U] == '$' && commandBufPtr[2U] == '$'
      && numBytesInBtRxBufWhenLastProcessed >= RN4X_CMD_LEN && getRxByteAtIndex(0) == 'C'
      && getRxByteAtIndex(1U) == 'M' && getRxByteAtIndex(2U) == 'D');
}

//uint8_t waitingForRnBtCmdResponse(void)
//{
//    return expectedResponsePtr[0]!='\0';
//}
//
//uint8_t isExpectedRnBtCmdResponseInFifo(void)
//{
//    uint8_t i = strlen(expectedResponsePtr);
//    if(getNumBytesInBtRxBuf()>=i)
//    {
//        for(;i>0;i--)
//        {
//            if (expectedResponsePtr[i-1] != getRxByteAtIndex(i-1))
//            {
//                return 0;
//            }
//        }
//        return 1;
//    }
//    return 0;
//}

void clearBytesFromBtRxBuf(uint16_t numBytes)
{
  uint8_t buf;
  for (; numBytes > 0; numBytes--)
  {
    readByteFromBtRxBuf(&buf);
  }
}

/*
 * Gets a byte from the BT RX buffer without removing it from it.
 *
 */
uint8_t getRxByteAtIndex(uint16_t index)
{
  return gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx + index)];
}

uint8_t processStatusString(void)
{
  uint8_t numberOfCharToRemove = 0U;
  uint8_t triggerBtSetGoodCommand = 0U;

  enum BT_FIRMWARE_VERSION btFwVer = getBtFwVersion();

  uint8_t firstChar = getRxByteAtIndex(1U);
  switch (firstChar)
  {
  case 'A':
    /* "%AUTHENTICATED%" */
    if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_AUTHENTICATED
        && getRxByteAtIndex(14U) == '%')
    {
      numberOfCharToRemove = BT_STAT_STR_LEN_AUTHENTICATED;
      /* TODO */
    }
    /* "%AUTH_FAIL%" */
    else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_AUTH_FAIL
        && getRxByteAtIndex(10U) == '%')
    {
      numberOfCharToRemove = BT_STAT_STR_LEN_AUTH_FAIL;
      /* TODO */
    }
    break;
  case 'B':
    /* "%BONDED%" */
    if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_BONDED
        && getRxByteAtIndex(7U) == '%')
    {
      numberOfCharToRemove = BT_STAT_STR_LEN_BONDED;
      /* TODO */
    }
    break;
  case 'C':
    /* "%CONNECT,001BDC06A3D5%" - RN4678 */
    if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN4678_CONNECT
        && getRxByteAtIndex(21U) == '%')
    {
      setRn4678ConnectionState(RN4678_CONNECTED_CLASSIC);
      numberOfCharToRemove = BT_STAT_STR_LEN_RN4678_CONNECT;
    }
    /* "%CONNECT" for RN42 v4.77 or "%CONNECT,001BDC06A3D5," - RN42 v6.15 */
    else if (((btFwVer == RN41_V4_77 || btFwVer == RN42_V4_77)
                 && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN42_v477_CONNECT
                 && getRxByteAtIndex(7U) == 'T')
        || (btFwVer == RN42_V6_15 && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN42_v615_CONNECT
            && getRxByteAtIndex(21U) == ','))
    {
      triggerBtRfCommStateChangeCallback(1U);

      if (btFwVer == RN41_V4_77 || btFwVer == RN42_V4_77)
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_RN42_v477_CONNECT;
      }
      else if (btFwVer == RN42_V6_15)
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_RN42_v615_CONNECT;
      }
      else
      {
        /* other RN42 FWs not supported */
      }
    }
    break;
  case 'D':
    /* "%DISCONN%" -> RN4678 */
    if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN4678_DISCONN
        && getRxByteAtIndex(8U) == '%')
    {
      setRn4678ConnectionState(RN4678_DISCONNECTED);
      numberOfCharToRemove = BT_STAT_STR_LEN_RN4678_DISCONN;
    }
    /* "%DISCONNECT" -> RN42 */
    else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN42_DISCONNECT
        && getRxByteAtIndex(10U) == 'T')
    {
      triggerBtRfCommStateChangeCallback(0);

      numberOfCharToRemove = BT_STAT_STR_LEN_RN42_DISCONNECT;
    }
    break;
  case 'E':
    if (getRxByteAtIndex(2U) == 'N')
    {
      /* "%END_INQ%" */
      if (getRxByteAtIndex(5U) == 'I' && getRxByteAtIndex(8U) == '%'
          && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_END_INQ)
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_END_INQ;
        /* TODO */
      }
      /* "%END_SCN%" */
      else if (getRxByteAtIndex(5U) == 'S' && getRxByteAtIndex(8U) == '%'
          && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_END_SCN)
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_END_SCN;
        /* TODO */
      }
    }
    else if (getRxByteAtIndex(2U) == 'R')
    {
      if (getRxByteAtIndex(5U) == 'C')
      {
        /* %ERR_CON is common to two status strings. If detected, read two more chars to determine which one it is */
        /* "%ERR_CONN%" */
        if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_ERR_CONN
            && getRxByteAtIndex(9U) == '%')
        {
          numberOfCharToRemove = BT_STAT_STR_LEN_ERR_CONN;
          /* TODO */
        }
        /* "%ERR_CONN_PARAM%" */
        else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_ERR_CONN_PARAM
            && getRxByteAtIndex(15U) == '%')
        {
          numberOfCharToRemove = BT_STAT_STR_LEN_ERR_CONN_PARAM;
          /* TODO */
        }
      }
      else if (getRxByteAtIndex(5U) == 'L')
      {
        /* "%ERR_LSEC%" */
        if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_ERR_LSEC
            && getRxByteAtIndex(9U) == '%')
        {
          numberOfCharToRemove = BT_STAT_STR_LEN_ERR_LSEC;
          /* TODO */
        }
      }
      else if (getRxByteAtIndex(5U) == 'S')
      {
        /* "%ERR_SEC%" */
        if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_ERR_SEC
            && getRxByteAtIndex(8U) == '%')
        {
          numberOfCharToRemove = BT_STAT_STR_LEN_ERR_SEC;
          /* TODO */
        }
      }
    }
    break;
  case 'F':
    /* "%FACTORY_RESET%" */
    if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_FACTORY_RESET
        && getRxByteAtIndex(14U) == '%')
    {
      numberOfCharToRemove = BT_STAT_STR_LEN_FACTORY_RESET;
      /* TODO */
    }
    break;
  case 'L':
    if (getRxByteAtIndex(2U) == 'B')
    {
      /* "%LBONDED%" */
      if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_LBONDED
          && getRxByteAtIndex(8U) == '%')
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_LBONDED;
        /* TODO */
      }
    }
    else if (getRxByteAtIndex(2U) == 'S')
    {
      if (getRxByteAtIndex(6U) == 'R')
      {
        /* "%LSECURED%" */
        if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_LSECURED
            && getRxByteAtIndex(9U) == '%')
        {
          numberOfCharToRemove = BT_STAT_STR_LEN_LSECURED;
          /* TODO */
        }
        /* "%LSECURE_FAIL%" */
        else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_LSECURE_FAIL
            && getRxByteAtIndex(13U) == '%')
        {
          numberOfCharToRemove = BT_STAT_STR_LEN_LSECURE_FAIL;
          /* TODO */
        }
      }
      else if (getRxByteAtIndex(6U) == 'A')
      {
        /* "%LSTREAM_OPEN%" */
        if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_LSTREAM_OPEN
            && getRxByteAtIndex(13U) == '%')
        {
          numberOfCharToRemove = BT_STAT_STR_LEN_LSTREAM_OPEN;
          /* TODO */
        }
      }
    }
    break;
  case 'M':
    /* "%MLDP_MODE%" */
    if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_MLDP_MODE
        && getRxByteAtIndex(10U) == '%')
    {
      numberOfCharToRemove = BT_STAT_STR_LEN_MLDP_MODE;
      /* TODO */
    }
    break;
  case 'R':
    if (getRxByteAtIndex(2U) == 'E')
    {
      /* "%REBOOT%" -> RN4678 */
      if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN4678_REBOOT
          && getRxByteAtIndex(7U) == '%')
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_RN4678_REBOOT;
      }
      else if (isBtDeviceRn41orRN42() && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN42_REBOOT
          && getRxByteAtIndex(6U) == 'T')
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_RN42_REBOOT;
      }

      if (BT_getWaitForInitialBoot() && numberOfCharToRemove)
      {
        BT_setWaitForInitialBoot(0);
        triggerBtSetGoodCommand = 1U;
      }
    }
    else if (getRxByteAtIndex(2U) == 'F')
    {
      if (getRxByteAtIndex(8U) == 'C')
      {
        /* "%RFCOMM_CLOSE%" */
        if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RFCOMM_CLOSE
            && getRxByteAtIndex(13U) == '%')
        {
          triggerBtRfCommStateChangeCallback(0);
          numberOfCharToRemove = BT_STAT_STR_LEN_RFCOMM_CLOSE;
        }
      }
      /* "%RFCOMM_OPEN%" */
      else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RFCOMM_OPEN
          && getRxByteAtIndex(12U) == '%')
      {
        triggerBtRfCommStateChangeCallback(1U);
        numberOfCharToRemove = BT_STAT_STR_LEN_RFCOMM_OPEN;
      }
    }
    break;
  case 'S':
    if (getRxByteAtIndex(3U) == 'C')
    {
      /* "%SECURED%" */
      if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SECURED
          && getRxByteAtIndex(8U) == '%')
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_SECURED;
        /* TODO */
      }
      /* "%SECURE_FAIL%" */
      else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SECURE_FAIL
          && getRxByteAtIndex(12U) == '%')
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_SECURE_FAIL;
        /* TODO */
      }
    }
    else if (getRxByteAtIndex(3U) == 'S')
    {
      /* "%SESSION_OPEN%" */
      if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SESSION_OPEN
          && getRxByteAtIndex(13U) == '%')
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_SESSION_OPEN;
        /* TODO */
      }
      /* "%SESSION_CLOSE%" */
      else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SESSION_CLOSE
          && getRxByteAtIndex(14U) == '%')
      {
        numberOfCharToRemove = BT_STAT_STR_LEN_SESSION_CLOSE;
        /* TODO */
      }
    }
    break;
  default:
    break;
  }

  clearBytesFromBtRxBuf(numberOfCharToRemove);
  if (triggerBtSetGoodCommand)
  {
    BT_setGoodCommand();
  }

  return numberOfCharToRemove > 0;
}

uint8_t isBtRxBufLike(char statStrCheck[], uint8_t numCharTolerance)
{
  uint8_t count = 0, bufIndex = 0, i = 0;
  uint8_t statStrCheckLen = strlen(statStrCheck);

  /* If there's not enough bytes in the buf even with the tolerance, return straight away */
  if (getNumBytesInBtRxBuf() < (statStrCheckLen - numCharTolerance))
  {
    return 0;
  }

  for (i = 0; i < statStrCheckLen; i++)
  {
    if (getRxByteAtIndex(bufIndex) == statStrCheck[i] || statStrCheck[i] == 'X')
    {
      count++;
      bufIndex++;
    }
  }

  if (((statStrCheckLen - count) <= numCharTolerance))
  {
    return count;
  }
  else
  {
    return 0;
  }
}

uint8_t processRnCmdResponse(void)
{
  uint8_t responseParsed = 0;

  /* Unwrap response */
  uint16_t i;
  for (i = 0; i < (indexOfFirstEol + 2); i++)
  {
    readByteFromBtRxBuf(&unwrappedResponse[i]);

    if (i >= 2 && unwrappedResponse[i - 1] == '\r' && unwrappedResponse[i] == '\n')
    {
      break;
    }
  }

  uint16_t unwrappedResponseLen = strlen(unwrappedResponse);
  uint8_t cmdExpectedAfterEol = (isBtDeviceRn4678() || isRn4678CmdDetectedOnBoot);

  /* Get BT module hardware and firmware version */
  if (commandBufPtr[0] == 'V')
  {
    enum BT_FIRMWARE_VERSION btFwVerNew = BT_FW_VER_UNKNOWN;

    /* RN41 or RN42 */
    if (unwrappedResponse[0U] == 'V')
    {
      /* RN41_VERSION_RESPONSE_V4_77 or RN42_VERSION_RESPONSE_V4_77 */
      if (unwrappedResponse[4U] == '4' && unwrappedResponse[5U] == '.')
      {
        if (unwrappedResponse[9U] == 'R' && unwrappedResponse[10U] == 'N')
        {
          btFwVerNew = RN42_V4_77;
        }
        else
        {
          btFwVerNew = RN41_V4_77;
        }
      }
      /* RN42_VERSION_RESPONSE_V6_15 */
      else if (unwrappedResponse[4U] == '6' && unwrappedResponse[5U] == '.'
          && unwrappedResponse[6U] == '1' && unwrappedResponse[7U] == '5')
      {
        btFwVerNew = RN42_V6_15;
      }
      /* V6.30 not supported */
      else if (unwrappedResponse[4U] == '6' && unwrappedResponse[5U] == '.'
          && unwrappedResponse[6U] == '3' && unwrappedResponse[7U] == '0')
      {
        btFwVerNew = RN42_V6_30;
      }
    }
    /* RN4678 */
    else if (unwrappedResponse[0U] == 'R')
    {
      /* RN4678_VERSION_RESPONSE_V1_00_5 */
      if (unwrappedResponse[10U] == '0' && unwrappedResponse[11U] == '0')
      {
        btFwVerNew = RN4678_V1_00_5;
      }
      /* RN4678_VERSION_RESPONSE_V1_11_0 */
      else if (unwrappedResponse[10U] == '1' && unwrappedResponse[11U] == '1')
      {
        btFwVerNew = RN4678_V1_11_0;
      }
      /* RN4678_VERSION_RESPONSE_V1_13_5 */
      else if (unwrappedResponse[10U] == '1' && unwrappedResponse[11U] == '3')
      {
        btFwVerNew = RN4678_V1_13_5;
      }
      /* RN4678_VERSION_RESPONSE_V1_22_0 */
      else if (unwrappedResponse[10U] == '2' && unwrappedResponse[11U] == '2')
      {
        btFwVerNew = RN4678_V1_22_0;
      }
      /* RN4678_VERSION_RESPONSE_V1_23_0 */
      else if (unwrappedResponse[10U] == '2' && unwrappedResponse[11U] == '3')
      {
        btFwVerNew = RN4678_V1_23_0;
      }
    }

    if (btFwVerNew == BT_FW_VER_UNKNOWN || btFwVerNew == RN42_V6_30)
    {
      triggerShimmerErrorState();
    }

    /* When storing the BT version, ignore from "\r" onwards */
    uint8_t btVerLen = strlen(unwrappedResponse);
    uint8_t btVerIdx;
    for (btVerIdx = 0; btVerIdx < btVerLen; btVerIdx++)
    {
      if (unwrappedResponse[btVerIdx] == '\r')
      {
        btVerLen = btVerIdx;
        break;
      }
    }
    memcpy(btVerStrResponse, unwrappedResponse, btVerLen);

    setBtFwVersion(btFwVerNew);

    responseParsed = 1;
  }
  /* Get commands */
  else if (commandBufPtr[0] == 'G')
  {
    /* Get MAC address */
    if (commandBufPtr[1] == 'B')
    {
      setMacId_cb(unwrappedResponse);
      responseParsed = 1;
    }
    else
    {
      responseParsed = parseRnGetResponse(commandBufPtr[1], &unwrappedResponse[0]);
    }
  }
  //else if(unwrappedResponse[0]=='C'
  //        && unwrappedResponse[1]=='M'
  //        && unwrappedResponse[2]=='D')
  //{
  //    setCommandModeActive(1U);
  //    responseParsed = 1;
  //    cmdExpectedAfterEol = 0;
  //}
  else if (unwrappedResponse[0] == 'A' && unwrappedResponse[1] == 'O'
      && unwrappedResponse[2] == 'K')
  {
    responseParsed = 1;
  }
  else if (unwrappedResponse[0] == 'E' && unwrappedResponse[1] == 'N'
      && unwrappedResponse[2] == 'D')
  {
    setCommandModeActive(0U);
    responseParsed = 1;
    cmdExpectedAfterEol = 0;
  }
  else if (unwrappedResponse[0] == 'E' && unwrappedResponse[1] == 'R'
      && unwrappedResponse[2] == 'R')
  {
    triggerShimmerErrorState();
  }
  else if (unwrappedResponse[0] == '?')
  {
    triggerShimmerErrorState();
  }

  else if (expectedResponsePtr[0] != '\0'
      && !memcmp(unwrappedResponse, expectedResponsePtr, strlen(expectedResponsePtr)))
  {
    responseParsed = 1;
  }

  memset(unwrappedResponse, 0, strlen(unwrappedResponse));

  if (responseParsed)
  {
    if (cmdExpectedAfterEol)
    {
      clearBytesFromBtRxBuf(RN4X_CMD_LEN);
    }

    clearBtCmdTxRxBuffsAndProceed();
  }

  return responseParsed;
}

uint8_t isFullRN4678CmdResponseReceived(void)
{
  int16_t numBytesAfterEol = numBytesInBtRxBufWhenLastProcessed - (indexOfFirstEol + 2U);

  /* For the RN4678, all responses have "CMD> " as a suffix except for "Trying\r\n", "END\r\n".
   * There is an option to turn this off in the RN4678 but we are leaving it on at the moment */
  if ((getRxByteAtIndex(0) == 'T' && getRxByteAtIndex(1U) == 'r' && getRxByteAtIndex(2U) == 'y')
      || (getRxByteAtIndex(0) == 'E' && getRxByteAtIndex(1U) == 'N'
          && getRxByteAtIndex(2U) == 'D'))
  {
    return 1U;
  }
  else
  {
    if (numBytesAfterEol != 0 && numBytesAfterEol >= RN4X_CMD_LEN)
    {
      return 1U;
    }
  }
  return 0;
}

void clearBtCmdTxRxBuffsAndProceed(void)
{
  clearBtCmdBuf();
  clearExpectedResponseBuf();
  BT_setGoodCommand();
}

uint8_t processShimmerBtCmd(void)
{
  uint8_t expectedLength = 0;
  uint8_t responseParsed = 0;

  uint8_t data = getRxByteAtIndex(0);
  switch (data)
  {
  /* 1 command byte, no argument bytes */
  case INQUIRY_COMMAND:
  case DUMMY_COMMAND:
  case GET_SAMPLING_RATE_COMMAND:
  case TOGGLE_LED_COMMAND:
  case START_STREAMING_COMMAND:
  case GET_STATUS_COMMAND:
  case GET_VBATT_COMMAND:
  case GET_TRIAL_CONFIG_COMMAND:
  case START_SDBT_COMMAND:
  case GET_CONFIG_SETUP_BYTES_COMMAND:
  case STOP_STREAMING_COMMAND:
  case STOP_SDBT_COMMAND:
  case START_LOGGING_COMMAND:
  case STOP_LOGGING_COMMAND:
  case GET_LN_ACCEL_CALIBRATION_COMMAND:
  case GET_GYRO_CALIBRATION_COMMAND:
  case GET_MAG_CALIBRATION_COMMAND:
  case GET_WR_ACCEL_CALIBRATION_COMMAND:
  case GET_GSR_RANGE_COMMAND:
  case GET_ALL_CALIBRATION_COMMAND:
  case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
  case GET_DEVICE_VERSION_COMMAND:
  case GET_FW_VERSION_COMMAND:
  case GET_CHARGE_STATUS_LED_COMMAND:
  case GET_BUFFER_SIZE_COMMAND:
  case GET_UNIQUE_SERIAL_COMMAND:
  case GET_WR_ACCEL_RANGE_COMMAND:
  case GET_MAG_GAIN_COMMAND:
  case GET_MAG_SAMPLING_RATE_COMMAND:
  case GET_WR_ACCEL_SAMPLING_RATE_COMMAND:
  case GET_WR_ACCEL_LPMODE_COMMAND:
  case GET_WR_ACCEL_HRMODE_COMMAND:
  case GET_GYRO_RANGE_COMMAND:
  case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
  case GET_BMP280_CALIBRATION_COEFFICIENTS_COMMAND:
  case GET_GYRO_SAMPLING_RATE_COMMAND:
  case GET_ALT_ACCEL_RANGE_COMMAND:
  case GET_PRESSURE_OVERSAMPLING_RATIO_COMMAND:
  case GET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
  case RESET_TO_DEFAULT_CONFIGURATION_COMMAND:
  case RESET_CALIBRATION_VALUE_COMMAND:
  case GET_MPU9150_MAG_SENS_ADJ_VALS_COMMAND:
  case GET_BT_COMMS_BAUD_RATE:
  case GET_CENTER_COMMAND:
  case GET_SHIMMERNAME_COMMAND:
  case GET_EXPID_COMMAND:
  case GET_MYID_COMMAND:
  case GET_NSHIMMER_COMMAND:
  case GET_CONFIGTIME_COMMAND:
  case GET_DIR_COMMAND:
  case GET_DERIVED_CHANNEL_BYTES:
  case GET_RWC_COMMAND:
  case UPD_SDLOG_CFG_COMMAND:
  case UPD_CALIB_DUMP_COMMAND:
  case GET_BT_VERSION_STR_COMMAND:
    readActionAndArgBytes(0);
    responseParsed = 1U;
    break;

  /* 1 command byte, 1 argument byte */
  case SET_WR_ACCEL_RANGE_COMMAND:
  case SET_WR_ACCEL_SAMPLING_RATE_COMMAND:
  case SET_MAG_GAIN_COMMAND:
  case SET_CHARGE_STATUS_LED_COMMAND:
  case SET_MAG_SAMPLING_RATE_COMMAND:
  case SET_WR_ACCEL_LPMODE_COMMAND:
  case SET_WR_ACCEL_HRMODE_COMMAND:
  case SET_GYRO_RANGE_COMMAND:
  case SET_GYRO_SAMPLING_RATE_COMMAND:
  case SET_ALT_ACCEL_RANGE_COMMAND:
  case SET_PRESSURE_OVERSAMPLING_RATIO_COMMAND:
  case SET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
  case SET_GSR_RANGE_COMMAND:
  case SET_BT_COMMS_BAUD_RATE:
  case SET_MYID_COMMAND:
  case SET_NSHIMMER_COMMAND:
  case SET_CRC_COMMAND:
  case SET_INSTREAM_RESPONSE_ACK_PREFIX_STATE:
  case SET_DATA_RATE_TEST:
  case SET_FACTORY_TEST:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 1U))
    {
      readActionAndArgBytes(1U);
      responseParsed = 1U;
    }
    break;

  /* 1 command byte, 2 argument bytes */
  case SET_SAMPLING_RATE_COMMAND:
  case GET_DAUGHTER_CARD_ID_COMMAND:
  case SET_DAUGHTER_CARD_ID_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 2U))
    {
      readActionAndArgBytes(2U);
      responseParsed = 1U;
    }
    break;

  /* 1 command byte, 3 argument bytes */
  case SET_SENSORS_COMMAND:
  case GET_EXG_REGS_COMMAND:
  case GET_DAUGHTER_CARD_MEM_COMMAND:
  case SET_TRIAL_CONFIG_COMMAND:
  case GET_INFOMEM_COMMAND:
  case GET_CALIB_DUMP_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 3U))
    {
      readActionAndArgBytes(3U);
      responseParsed = 1U;
    }
    break;

  /* 1 command byte, 4 argument bytes */
  case SET_CONFIG_SETUP_BYTES_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 4U))
    {
      readActionAndArgBytes(4U);
      responseParsed = 1U;
    }
    break;

  /* 1 command byte, 8 argument bytes */
  case SET_RWC_COMMAND:
  case SET_DERIVED_CHANNEL_BYTES:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 8U))
    {
      readActionAndArgBytes(8U);
      responseParsed = 1U;
    }
    break;

#if !USE_OLD_SD_SYNC_APPROACH
  /* 1 command byte, 9 to 11 argument bytes */
  case SET_SD_SYNC_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + SYNC_PACKET_PAYLOAD_SIZE + BT_SD_SYNC_CRC_MODE))
    {
      readActionAndArgBytes(SYNC_PACKET_PAYLOAD_SIZE + BT_SD_SYNC_CRC_MODE);
      responseParsed = 1U;
    }
    break;
#endif

  /* 1 command byte, 21 argument bytes */
  case SET_LN_ACCEL_CALIBRATION_COMMAND:
  case SET_GYRO_CALIBRATION_COMMAND:
  case SET_MAG_CALIBRATION_COMMAND:
  case SET_WR_ACCEL_CALIBRATION_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 21U))
    {
      readActionAndArgBytes(21U);
      responseParsed = 1U;
    }
    break;

    /*********************************************/
  /* special cases */
  /* 1 command byte, minimum 1 argument byte - expected length is stored in arg[0] */
  case SET_CENTER_COMMAND:
  case SET_CONFIGTIME_COMMAND:
  case SET_EXPID_COMMAND:
  case SET_SHIMMERNAME_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 1U))
    {
      expectedLength = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx + 1U)];
      if (numBytesInBtRxBufWhenLastProcessed >= (1U + 1U + expectedLength))
      {
        readActionAndArgBytes(1U + expectedLength);
        responseParsed = 1U;
      }
    }
    break;

  /* 1 command byte, minimum 2 argument bytes - expected length is stored in arg[0] */
  case SET_DAUGHTER_CARD_ID_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 2U))
    {
      expectedLength = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx + 1U)];
      if (numBytesInBtRxBufWhenLastProcessed >= (1U + 2U + expectedLength))
      {
        readActionAndArgBytes(2U + expectedLength);
        responseParsed = 1U;
      }
    }
    break;

  /* 1 command byte, minimum 3 argument bytes - expected length is stored in arg[2] */
  case SET_EXG_REGS_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 3U))
    {
      expectedLength = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx + 3U)];
      if (numBytesInBtRxBufWhenLastProcessed >= (1U + 3U + expectedLength))
      {
        readActionAndArgBytes(3U + expectedLength);
        responseParsed = 1U;
      }
    }
    break;

  /* 1 command byte, minimum 3 argument bytes - expected length is stored in arg[0] */
  case SET_INFOMEM_COMMAND:
  case SET_CALIB_DUMP_COMMAND:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 3U))
    {
      expectedLength = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx + 1U)];
      if (numBytesInBtRxBufWhenLastProcessed >= (1U + 3U + expectedLength))
      {
        readActionAndArgBytes(3U + expectedLength);
        responseParsed = 1U;
      }
    }
    break;

#if !USE_OLD_SD_SYNC_APPROACH
  /* 1 command byte, minimum 1 argument byte representing command response */
  case ACK_COMMAND_PROCESSED:
    if (numBytesInBtRxBufWhenLastProcessed >= (1U + 1U))
    {
      uint8_t cmdByte = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx + 1U)];
      if (cmdByte == SD_SYNC_RESPONSE && numBytesInBtRxBufWhenLastProcessed >= (1U + 2U))
      {
        readActionAndArgBytes(2U);
        responseParsed = 1U;
      }
    }
    break;
#endif
    /********************************************/

  default:
    break;
  }

  return responseParsed;
}

void readActionAndArgBytes(uint8_t numArgs)
{
  readByteFromBtRxBuf(gActionPtr);
  if (numArgs)
  {
    uint8_t i;
    for (i = 0; i < numArgs; i++)
    {
      readByteFromBtRxBuf(gArgsPtr + i);
    }
  }

  if (newBtCmdToProcess_cb)
  {
    newBtCmdToProcess_cb();
  }
}

uint8_t isNewLineDetectedInBtRxBuf(void)
{
  uint16_t i = 0;
  if (numBytesInBtRxBufWhenLastProcessed > 2U)
  {
    /* numBytesInRxBuf is -1 here as we're looking to i+1 for \n and it was triggering too early as new bytes were coming in */
    for (i = 0; i < (numBytesInBtRxBufWhenLastProcessed - 1); i++)
    {
      if (getRxByteAtIndex(i) == '\r' && getRxByteAtIndex(i + 1) == '\n')
      {
        return i;
      }
    }
  }
  return 0;
}

uint8_t isShimmerBtCmd(uint8_t data)
{
  switch (data)
  {
  case INQUIRY_COMMAND:
  case DUMMY_COMMAND:
  case GET_SAMPLING_RATE_COMMAND:
  case TOGGLE_LED_COMMAND:
  case START_STREAMING_COMMAND:
  case GET_STATUS_COMMAND:
  case GET_VBATT_COMMAND:
  case GET_TRIAL_CONFIG_COMMAND:
  case START_SDBT_COMMAND:
  case GET_CONFIG_SETUP_BYTES_COMMAND:
  case STOP_STREAMING_COMMAND:
  case STOP_SDBT_COMMAND:
  case START_LOGGING_COMMAND:
  case STOP_LOGGING_COMMAND:
  case GET_LN_ACCEL_CALIBRATION_COMMAND:
  case GET_GYRO_CALIBRATION_COMMAND:
  case GET_MAG_CALIBRATION_COMMAND:
  case GET_WR_ACCEL_CALIBRATION_COMMAND:
  case GET_GSR_RANGE_COMMAND:
  case GET_ALL_CALIBRATION_COMMAND:
  case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
  case GET_DEVICE_VERSION_COMMAND:
  case GET_FW_VERSION_COMMAND:
  case GET_CHARGE_STATUS_LED_COMMAND:
  case GET_BUFFER_SIZE_COMMAND:
  case GET_UNIQUE_SERIAL_COMMAND:
  case GET_WR_ACCEL_RANGE_COMMAND:
  case GET_MAG_GAIN_COMMAND:
  case GET_MAG_SAMPLING_RATE_COMMAND:
  case GET_WR_ACCEL_SAMPLING_RATE_COMMAND:
  case GET_WR_ACCEL_LPMODE_COMMAND:
  case GET_WR_ACCEL_HRMODE_COMMAND:
  case GET_GYRO_RANGE_COMMAND:
  case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
  case GET_BMP280_CALIBRATION_COEFFICIENTS_COMMAND:
  case GET_GYRO_SAMPLING_RATE_COMMAND:
  case GET_ALT_ACCEL_RANGE_COMMAND:
  case GET_PRESSURE_OVERSAMPLING_RATIO_COMMAND:
  case GET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
  case RESET_TO_DEFAULT_CONFIGURATION_COMMAND:
  case RESET_CALIBRATION_VALUE_COMMAND:
  case GET_MPU9150_MAG_SENS_ADJ_VALS_COMMAND:
  case GET_BT_COMMS_BAUD_RATE:
  case GET_CENTER_COMMAND:
  case GET_SHIMMERNAME_COMMAND:
  case GET_EXPID_COMMAND:
  case GET_MYID_COMMAND:
  case GET_NSHIMMER_COMMAND:
  case GET_CONFIGTIME_COMMAND:
  case GET_DIR_COMMAND:
  case GET_DERIVED_CHANNEL_BYTES:
  case GET_RWC_COMMAND:
  case UPD_SDLOG_CFG_COMMAND:
  case UPD_CALIB_DUMP_COMMAND:
  case GET_BT_VERSION_STR_COMMAND:
  case SET_WR_ACCEL_RANGE_COMMAND:
  case SET_WR_ACCEL_SAMPLING_RATE_COMMAND:
  case SET_MAG_GAIN_COMMAND:
  case SET_CHARGE_STATUS_LED_COMMAND:
  case SET_MAG_SAMPLING_RATE_COMMAND:
  case SET_WR_ACCEL_LPMODE_COMMAND:
  case SET_WR_ACCEL_HRMODE_COMMAND:
  case SET_GYRO_RANGE_COMMAND:
  case SET_GYRO_SAMPLING_RATE_COMMAND:
  case SET_ALT_ACCEL_RANGE_COMMAND:
  case SET_PRESSURE_OVERSAMPLING_RATIO_COMMAND:
  case SET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
  case SET_GSR_RANGE_COMMAND:
  case SET_BT_COMMS_BAUD_RATE:
  case SET_CENTER_COMMAND:
  case SET_SHIMMERNAME_COMMAND:
  case SET_EXPID_COMMAND:
  case SET_MYID_COMMAND:
  case SET_NSHIMMER_COMMAND:
  case SET_CONFIGTIME_COMMAND:
  case SET_CRC_COMMAND:
  case SET_INSTREAM_RESPONSE_ACK_PREFIX_STATE:
  case SET_DATA_RATE_TEST:
  case SET_FACTORY_TEST:
  case SET_SAMPLING_RATE_COMMAND:
  case GET_DAUGHTER_CARD_ID_COMMAND:
  case SET_DAUGHTER_CARD_ID_COMMAND:
  case SET_SENSORS_COMMAND:
  case GET_EXG_REGS_COMMAND:
  case SET_EXG_REGS_COMMAND:
  case GET_DAUGHTER_CARD_MEM_COMMAND:
  case SET_DAUGHTER_CARD_MEM_COMMAND:
  case SET_TRIAL_CONFIG_COMMAND:
  case GET_INFOMEM_COMMAND:
  case SET_INFOMEM_COMMAND:
  case GET_CALIB_DUMP_COMMAND:
  case SET_CALIB_DUMP_COMMAND:
  case SET_CONFIG_SETUP_BYTES_COMMAND:
  case SET_RWC_COMMAND:
  case SET_DERIVED_CHANNEL_BYTES:
  case SET_LN_ACCEL_CALIBRATION_COMMAND:
  case SET_GYRO_CALIBRATION_COMMAND:
  case SET_MAG_CALIBRATION_COMMAND:
  case SET_WR_ACCEL_CALIBRATION_COMMAND:
#if !USE_OLD_SD_SYNC_APPROACH
  case SET_SD_SYNC_COMMAND:
#endif
  case ACK_COMMAND_PROCESSED:
    return 1;
  default:
    return 0;
  }
}

void updateNumBytesInBtRxBufWhenLastProcessed(void)
{
  numBytesInBtRxBufWhenLastProcessed = getNumBytesInBtRxBuf();
}

uint16_t getNumBytesInBtRxBufWhenLastProcessed(void)
{
  return numBytesInBtRxBufWhenLastProcessed;
}

uint8_t areUnprocessedBytesInBtRxBuff(void)
{
  uint16_t numBytes = getNumBytesInBtRxBuf();
  return numBytes != 0 && numBytes != getNumBytesInBtRxBufWhenLastProcessed();
}
#endif

#if defined(SHIMMER3)
void btCommsProtocolInit(uint8_t (*newBtCmdToProcessCb)(void),
    void (*handleBtRfCommStateChangeCb)(uint8_t),
    void (*setMacIdCb)(uint8_t *),
    uint8_t *actionPtr,
    uint8_t *argsPtr)
#else
void btCommsProtocolInit(uint8_t (*newBtCmdToProcessCb)(void))
#endif
{
  setBtCrcMode(CRC_OFF);
  numBytesInBtRxBufWhenLastProcessed = 0;
  indexOfFirstEol = 0;
  firstProcessFailTicks = 0;
  memset(unwrappedResponse, 0, sizeof(unwrappedResponse));

  newBtCmdToProcess_cb = newBtCmdToProcessCb;
#if defined(SHIMMER3)
  handleBtRfCommStateChange_cb = handleBtRfCommStateChangeCb;
  setMacId_cb = setMacIdCb;

  gActionPtr = actionPtr;
  gArgsPtr = argsPtr;
#else
  gActionPtr = &gAction;
  gArgsPtr = &args[0];
#endif

#if !BT_DMA_USED_FOR_RX
  commandBufPtr = getTxCmdBufPtr();
#endif
#if defined(SHIMMER3)
  expectedResponsePtr = BT_getExpResp();
#endif
#if BT_DMA_USED_FOR_RX
#if defined(SHIMMER3)
  btRxExp = BT_getExpResp();
#endif

  waitingForArgs = 0;
  waitingForArgsLength = 0;
  argsSize = 0;

#if defined(SHIMMER3)
  memset(btStatusStr, 0, sizeof(btStatusStr));

  memset(btRxBuffFullResponse, 0x00,
      sizeof(btRxBuffFullResponse) / sizeof(btRxBuffFullResponse[0]));
  setBtRxFullResponsePtr(btRxBuffFullResponse);
#endif

  //memset(btRxBuffPtr, 0x00, sizeof(btRxBuffPtr) / sizeof(btRxBuffPtr[0]));
  //DMA2_init((uint16_t*) &UCA1RXBUF, (uint16_t*) btRxBuff, sizeof(btRxBuff));
  //DMA2_transferDoneFunction(&Dma2ConversionDone);
  //DMA2SZ = 1U;
  //DMA2_enable();

#else
  gBtRxFifoPtr = getRxFifoPtr();
  isRn4678CmdDetectedOnBoot = 0;
#endif

  memset(btVerStrResponse, 0x00, sizeof(btVerStrResponse) / sizeof(btVerStrResponse[0]));

  resetBtResponseBools();
}

#if defined(SHIMMER3)
void triggerBtRfCommStateChangeCallback(bool state)
{
  if (handleBtRfCommStateChange_cb)
  {
    handleBtRfCommStateChange_cb(state);
  }
}

void triggerShimmerErrorState(void)
{
  while (1)
  {
    Board_ledOff(LED_ALL);
    _delay_cycles(24000000);
    Board_ledOn(LED_YELLOW);
    _delay_cycles(12000000);
    Board_ledOn(LED_RED);
    _delay_cycles(12000000);
    Board_ledOn(LED_BLUE);
    _delay_cycles(12000000);
    Board_ledOn(LED_GREEN1);
    _delay_cycles(12000000);
  }
}
#endif

void resetBtResponseBools(void)
{
  sendAck = 0;
  inquiryResponse = 0;
  samplingRateResponse = 0;
  lnAccelCalibrationResponse = 0;
  wrAccelRangeResponse = 0;
  magGainResponse = 0;
  magSamplingRateResponse = 0;
  dockedResponse = 0;
  trialConfigResponse = 0;
  centerResponse = 0;
  shimmerNameResponse = 0;
  expIDResponse = 0;
  configTimeResponse = 0;
  dirResponse = 0;
  nshimmerResponse = 0;
  myIDResponse = 0;
  wrAccelSamplingRateResponse = 0;
  wrAccelLpModeResponse = 0;
  wrAccelHrModeResponse = 0;
  gyroRangeResponse = 0;
  bmp180CalibrationCoefficientsResponse = 0;
  bmp280CalibrationCoefficientsResponse = 0;
  setEepromIsPresent(0);
  gyroSamplingRateResponse = 0;
  altAccelRangeResponse = 0;
  bmpOversamplingRatioResponse = 0;
  internalExpPowerEnableResponse = 0;
  configSetupBytesResponse = 0;
  gyroCalibrationResponse = 0;
  magCalibrationResponse = 0;
  wrAccelCalibrationResponse = 0;
  allCalibrationResponse = 0;
  deviceVersionResponse = 0;
  fwVersionResponse = 0;
  bufferSizeResponse = 0;
  uniqueSerialResponse = 0;
  mpu9150MagSensAdjValsResponse = 0;
  exgRegsResponse = 0;
  dcIdResponse = 0;
  dcMemResponse = 0;
  infomemResponse = 0;
  calibRamResponse = 0;
  btVerResponse = 0;
  rwcResponse = 0;
  btVbattResponse = 0;
  blinkLedResponse = 0;
  gsrRangeResponse = 0;
  btCommsBaudRateResponse = 0;
  derivedChannelResponse = 0;
  btDataRateTestResponse = 0;
  bmpGenericCalibrationCoefficientsResponse = 0;

  useAckPrefixForInstreamResponses = 1U;

#if defined(SHIMMER4_SDK)
  i2cvBattBtRsp = 0;
#endif
}

uint8_t getBtVerStrLen(void)
{
  return strlen(btVerStrResponse);
}

char *getBtVerStrPtr(void)
{
  return &btVerStrResponse[0];
}

void updateBtVer(void)
{
#if defined(SHIMMER3R)
  BT_generateCyw20820FirmwareVersionStr(&btVerStrResponse[0]);
#endif
}

uint8_t isWaitingForArgs(void)
{
  return waitingForArgs;
}

void BtUart_processCmd(void)
{
  uint64_t temp64;
  gConfigBytes *storedConfig = S4Ram_getStoredConfig();

  uint32_t config_time;
  uint8_t my_config_time[4];
  uint8_t name_len;

  uint8_t update_sdconfig = 0, update_calib_dump_file = 0;
  uint8_t fullSyncResp[SYNC_PACKET_MAX_SIZE] = { 0 };

  switch (gAction)
  {
  case INQUIRY_COMMAND:
    inquiryResponse = 1;
    break;
  case DUMMY_COMMAND:
    break;
  case GET_SAMPLING_RATE_COMMAND:
    samplingRateResponse = 1;
    break;
  case TOGGLE_LED_COMMAND:
    shimmerStatus.toggleLedRedCmd ^= 1;
    break;
  case START_STREAMING_COMMAND:
    shimmerStatus.btstreamCmd = BT_STREAM_CMD_STATE_START;
    S4_Task_set(TASK_STARTSENSING);
    break;
  case START_SDBT_COMMAND:
    shimmerStatus.btstreamCmd = BT_STREAM_CMD_STATE_START;
    shimmerStatus.sdlogCmd = 1;
    S4_Task_set(TASK_STARTSENSING);
    break;
  case START_LOGGING_COMMAND:
    shimmerStatus.sdlogCmd = 1;
    S4_Task_set(TASK_STARTSENSING);
    break;
  case SET_CRC_COMMAND:
    setBtCrcMode(args[0]);
    break;
  case SET_INSTREAM_RESPONSE_ACK_PREFIX_STATE:
    useAckPrefixForInstreamResponses = args[0];
    break;
  case STOP_STREAMING_COMMAND:
    shimmerStatus.btstreamCmd = BT_STREAM_CMD_STATE_STOP;
    S4_Task_set(TASK_STOPSENSING);
    break;
  case STOP_SDBT_COMMAND:
    shimmerStatus.btstreamCmd = BT_STREAM_CMD_STATE_STOP;
    shimmerStatus.sdlogCmd = 2;
    S4_Task_set(TASK_STOPSENSING);
    break;
  case STOP_LOGGING_COMMAND:
    shimmerStatus.sdlogCmd = 2;
    S4_Task_set(TASK_STOPSENSING);
    break;
  case SET_SENSORS_COMMAND:
    S4Ram_storedConfigSet(&args[0], NV_SENSORS0, 3);
    //TODO move to general config correction function
    if (storedConfig->chEnGsr)
    {
      //they are sharing Shimmer3 adc1, so ban intch1 when gsr is on
      storedConfig->chEnIntADC3 = 0;
    }
    S4Ram_sdHeadTextSet(&storedConfig->rawBytes[NV_SENSORS0], NV_SENSORS0, 3);
    InfoMem_write(NV_SENSORS0, &storedConfig->rawBytes[NV_SENSORS0], 3);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case GET_WR_ACCEL_RANGE_COMMAND:
    wrAccelRangeResponse = 1;
    break;
  case GET_MAG_GAIN_COMMAND:
    magGainResponse = 1;
    break;
  case GET_MAG_SAMPLING_RATE_COMMAND:
    magSamplingRateResponse = 1;
    break;
  case GET_STATUS_COMMAND:
    dockedResponse = 1;
    break;
#if defined(SHIMMER4_SDK)
  case GET_I2C_BATT_STATUS_COMMAND:
    i2cvBattBtRsp = 1;
    break;
  case SET_I2C_BATT_STATUS_FREQ_COMMAND:
    temp16 = args[0] + ((uint16_t) args[1] << 8);
    I2C_readBattSetFreq(temp16);
    break;
#endif
  case GET_VBATT_COMMAND:
    btVbattResponse = 1;
    break;
  case GET_TRIAL_CONFIG_COMMAND:
    trialConfigResponse = 1;
    break;
  case SET_TRIAL_CONFIG_COMMAND:
    storedConfig->rawBytes[NV_SD_TRIAL_CONFIG0] = args[0];
    storedConfig->rawBytes[NV_SD_TRIAL_CONFIG1] = args[1];
    storedConfig->rawBytes[NV_SD_BT_INTERVAL] = args[2];

    //TODO move to common setting correction function
#if !IS_SUPPORTED_TCXO
    storedConfig->tcxo = 0; /* Disable TCXO */
#endif

    if (storedConfig->singleTouchStart)
    {
      storedConfig->userButtonEnable = 1;
      storedConfig->syncEnable = 1;
    }

    if (storedConfig->btInterval < SYNC_INT_C)
    {
      storedConfig->btInterval = SYNC_INT_C;
    }

    InfoMem_write(NV_SAMPLING_RATE, &storedConfig->rawBytes[NV_SD_TRIAL_CONFIG0], 3);

    S4Ram_sdHeadTextSetByte(SDH_TRIAL_CONFIG0, storedConfig->rawBytes[NV_SD_TRIAL_CONFIG0]);
    S4Ram_sdHeadTextSetByte(SDH_TRIAL_CONFIG1, storedConfig->rawBytes[NV_SD_TRIAL_CONFIG1]);
    S4Ram_sdHeadTextSetByte(SDH_BROADCAST_INTERVAL, storedConfig->rawBytes[NV_SD_BT_INTERVAL]);
    //S4Ram_sdHeadTextSet(&storedConfig->rawBytes, NV_SD_TRIAL_CONFIG0, 3);

    update_sdconfig = 1;
    break;
  case GET_CENTER_COMMAND:
    centerResponse = 1;
    break;
  case SET_CENTER_COMMAND:
    break;
  case GET_SHIMMERNAME_COMMAND:
    shimmerNameResponse = 1;
    break;
  case SET_SHIMMERNAME_COMMAND:
    name_len = args[0] < (MAX_CHARS - 1) ? args[0] : (MAX_CHARS - 1);
    memset(&storedConfig->rawBytes[NV_SD_SHIMMER_NAME], 0, MAX_CHARS - 1);
    memcpy(&storedConfig->rawBytes[NV_SD_SHIMMER_NAME], &args[1], name_len);
    InfoMem_write(NV_SD_SHIMMER_NAME,
        &storedConfig->rawBytes[NV_SD_SHIMMER_NAME], MAX_CHARS - 1);
    SD_setShimmerName();
    update_sdconfig = 1;
    break;
  case GET_EXPID_COMMAND:
    expIDResponse = 1;
    break;
  case SET_EXPID_COMMAND:
    name_len = args[0] < (MAX_CHARS - 1) ? args[0] : (MAX_CHARS - 1);
    memset(&storedConfig->rawBytes[NV_SD_EXP_ID_NAME], 0, MAX_CHARS - 1);
    memcpy(&storedConfig->rawBytes[NV_SD_EXP_ID_NAME], &args[1], name_len);
    InfoMem_write(NV_SD_EXP_ID_NAME, &storedConfig->rawBytes[NV_SD_EXP_ID_NAME],
        MAX_CHARS - 1);
    SD_setExpIdName();
    update_sdconfig = 1;
    break;
  case GET_CONFIGTIME_COMMAND:
    configTimeResponse = 1;
    break;
  case GET_DIR_COMMAND:
    dirResponse = 1;
    break;
  case SET_CONFIGTIME_COMMAND:
    name_len = args[0] < (MAX_CHARS - 1) ? args[0] : (MAX_CHARS - 1);
    uint8_t *configTimeTextPtr = getConfigTimeTextPtr();
    memcpy(configTimeTextPtr, &args[1], name_len);
    configTimeTextPtr[name_len] = 0;
    SetName();
    config_time = atol((char *) configTimeTextPtr);
    my_config_time[3] = *((uint8_t *) &config_time);
    my_config_time[2] = *(((uint8_t *) &config_time) + 1);
    my_config_time[1] = *(((uint8_t *) &config_time) + 2);
    my_config_time[0] = *(((uint8_t *) &config_time) + 3);
    S4Ram_sdHeadTextSet(&my_config_time[0], SDH_CONFIG_TIME_0, 4);
    S4Ram_storedConfigSet(&my_config_time[0], NV_SD_CONFIG_TIME, 4);
    InfoMem_write(NV_SD_CONFIG_TIME, &storedConfig->rawBytes[NV_SD_CONFIG_TIME], 4);
    update_sdconfig = 1;
    break;
  case GET_NSHIMMER_COMMAND:
    nshimmerResponse = 1;
    break;
  case SET_NSHIMMER_COMMAND:
    storedConfig->numberOfShimmers = args[0];
    S4Ram_sdHeadTextSetByte(SDH_NSHIMMER, args[0]);
    InfoMem_write(NV_SD_NSHIMMER, &storedConfig->rawBytes[NV_SD_NSHIMMER], 1);
    update_sdconfig = 1;
    break;
  case GET_MYID_COMMAND:
    myIDResponse = 1;
    break;
  case SET_MYID_COMMAND:
    storedConfig->myTrialID = args[0];
    S4Ram_sdHeadTextSetByte(SDH_MYTRIAL_ID, args[0]);
    InfoMem_write(NV_SD_MYTRIAL_ID, &storedConfig->rawBytes[NV_SD_MYTRIAL_ID], 1);
    update_sdconfig = 1;
    break;
  case SET_WR_ACCEL_RANGE_COMMAND:
    storedConfig->wrAccelRange = args[0] < 4 ? (args[0] & 0x03) : 0;
    InfoMem_write(NV_CONFIG_SETUP_BYTE0, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE0, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case GET_WR_ACCEL_SAMPLING_RATE_COMMAND:
    wrAccelSamplingRateResponse = 1;
    break;
  case GET_WR_ACCEL_LPMODE_COMMAND:
    wrAccelLpModeResponse = 1;
    break;
  case GET_WR_ACCEL_HRMODE_COMMAND:
    wrAccelHrModeResponse = 1;
    break;
  case GET_GYRO_RANGE_COMMAND:
    gyroRangeResponse = 1;
    break;
  case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
    bmp180CalibrationCoefficientsResponse = 1;
    break;
  case GET_BMP280_CALIBRATION_COEFFICIENTS_COMMAND:
    bmp280CalibrationCoefficientsResponse = 1;
    break;
  case GET_PRESSURE_CALIBRATION_COEFFICIENTS_COMMAND:
    bmpGenericCalibrationCoefficientsResponse = 1;
    break;
  case GET_GYRO_SAMPLING_RATE_COMMAND:
    gyroSamplingRateResponse = 1;
    break;
  case GET_ALT_ACCEL_RANGE_COMMAND:
    altAccelRangeResponse = 1;
    break;
  case GET_PRESSURE_OVERSAMPLING_RATIO_COMMAND:
    bmpOversamplingRatioResponse = 1;
    break;
  case GET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
    internalExpPowerEnableResponse = 1;
    break;
  case GET_MPU9150_MAG_SENS_ADJ_VALS_COMMAND:
    mpu9150MagSensAdjValsResponse = 1;
    break;
  case GET_EXG_REGS_COMMAND:
    if (args[0] < 2 && args[1] < 10 && args[2] < 11)
    {
      exgChip = args[0];
      exgStartAddr = args[1];
      exgLength = args[2];
    }
    else
    {
      exgLength = 0;
    }
    exgRegsResponse = 1;
    break;
  case SET_WR_ACCEL_SAMPLING_RATE_COMMAND:
#if defined(SHIMMER3)
    storedConfig->wrAccelRate
        = (args[0] <= LSM303DLHC_ACCEL_1_344kHz) ? args[0] : LSM303DLHC_ACCEL_100HZ;
#elif defined(SHIMMER3R)
    storedConfig->wrAccelRate
        = (args[0] <= LIS2DW12_XL_ODR_1k6Hz) ? args[0] : LIS2DW12_XL_ODR_100Hz;
#endif
    InfoMem_write(NV_CONFIG_SETUP_BYTE0, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE0, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_MAG_GAIN_COMMAND:
#if defined(SHIMMER3)
    storedConfig->magRange = (args[0] <= LSM303DLHC_MAG_8_1G) ? args[0] : LSM303DLHC_MAG_1_3G;
#elif defined(SHIMMER3R)
    storedConfig->magRange = (args[0] <= LIS3MDL_16_GAUSS) ? args[0] : LIS3MDL_4_GAUSS;
#endif
    InfoMem_write(NV_CONFIG_SETUP_BYTE2, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE2], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE2, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE2]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_MAG_SAMPLING_RATE_COMMAND:
#if defined(SHIMMER3)
    storedConfig->magRateLsb = (args[0] <= LSM303DLHC_MAG_220HZ) ? args[0] : LSM303DLHC_MAG_75HZ;
#elif defined(SHIMMER3R)
    set_config_byte_mag_rate(storedConfig, args[0]);
#endif
    InfoMem_write(NV_CONFIG_SETUP_BYTE2, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE2], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE2, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE2]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_WR_ACCEL_LPMODE_COMMAND:
    set_config_byte_wr_accel_lp_mode(storedConfig, args[0]);
    InfoMem_write(NV_CONFIG_SETUP_BYTE0, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE0, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_WR_ACCEL_HRMODE_COMMAND:
    storedConfig->wrAccelHrMode = (args[0] == 1) ? 1 : 0;
    InfoMem_write(NV_CONFIG_SETUP_BYTE0, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE0, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_GYRO_RANGE_COMMAND:
    set_config_byte_gyro_range(storedConfig, args[0]);
    InfoMem_write(NV_CONFIG_SETUP_BYTE2, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE2], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE2, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE2]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_GYRO_SAMPLING_RATE_COMMAND:
    set_config_byte_gyro_rate(storedConfig, args[0]);
    InfoMem_write(NV_CONFIG_SETUP_BYTE1, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE1], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE1, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE1]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_ALT_ACCEL_RANGE_COMMAND:
#if defined(SHIMMER3)
    storedConfig->altAccelRange = (args[0] < 4) ? (args[0] & 0x03) : ACCEL_2G;
#elif defined(SHIMMER3R)
    storedConfig->altAccelRange = (args[0] < LSM6DSV_16g) ? (args[0] & 0x03) : LSM6DSV_2g;
#endif
    InfoMem_write(NV_CONFIG_SETUP_BYTE3, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE3], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE3, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE3]);

    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_PRESSURE_OVERSAMPLING_RATIO_COMMAND:
    set_config_byte_pressure_oversampling_ratio(storedConfig, args[0]);
    InfoMem_write(NV_CONFIG_SETUP_BYTE3, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE3], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE3, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE3]);
    update_sdconfig = 1;
    break;
  case SET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
    storedConfig->expansionBoardPower = (args[0] == 1) ? 1 : 0;
    InfoMem_write(NV_CONFIG_SETUP_BYTE3, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE3], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE3, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE3]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case GET_CONFIG_SETUP_BYTES_COMMAND:
    configSetupBytesResponse = 1;
    break;
  case SET_CONFIG_SETUP_BYTES_COMMAND:
    S4Ram_storedConfigSet(&args[0], NV_CONFIG_SETUP_BYTE0, 4);
    InfoMem_write(NV_CONFIG_SETUP_BYTE0, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0], 4);
    S4Ram_sdHeadTextSet(&storedConfig->rawBytes[0], NV_CONFIG_SETUP_BYTE0, 4);
    S4Ram_config2SdHead();
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_SAMPLING_RATE_COMMAND:
    S4Ram_storedConfigSet(&args[0], NV_SAMPLING_RATE, 2);
    InfoMem_write(NV_SAMPLING_RATE, &storedConfig->rawBytes[NV_SAMPLING_RATE], 2);
    S4Ram_sdHeadTextSet(&args[0], SDH_SAMPLE_RATE_0, 2);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      //restart sampling timer to use new sampling rate
      setStopSensing();
      setStartSensing();
    }
    break;
  case GET_CALIB_DUMP_COMMAND:
    //usage:
    //0x98, offset, offset, length
    calibRamLength = args[0];
    calibRamOffset = args[1] + (args[2] << 8);
    calibRamResponse = 1;
    break;
  case SET_CALIB_DUMP_COMMAND:
    //usage:
    //0x98, offset, offset, length, data[0:127]
    //max length of this command = 132
    calibRamLength = args[0];
    calibRamOffset = args[1] + (args[2] << 8);
    if (ShimmerCalib_ramWrite(&args[3], calibRamLength, calibRamOffset) == 1)
    {
      ShimmerCalibSyncFromDumpRamAll();
      update_calib_dump_file = 1;
    }
    break;
  case UPD_CALIB_DUMP_COMMAND:
    ShimmerCalibSyncFromDumpRamAll();
    update_calib_dump_file = 1;
    break;
    //case UPD_FLASH_COMMAND:
    //   InfoMem_update();
    //   //ShimmerCalibSyncFromDumpRamAll();
    //   //update_calib_dump_file = 1;
    //   break;
  case SET_LN_ACCEL_CALIBRATION_COMMAND:
    memcpy(&storedConfig->lnAccelCalib.rawBytes[0], &args[0], 21);
    InfoMem_write(NV_LN_ACCEL_CALIBRATION, &storedConfig->lnAccelCalib.rawBytes[0], 21);
    S4Ram_sdHeadTextSet(
        &storedConfig->lnAccelCalib.rawBytes[0], SDH_LN_ACCEL_CALIBRATION, 21);

#if defined(SHIMMER3)
    CalibSaveFromInfoMemToCalibDump(SC_SENSOR_ANALOG_ACCEL);
#elif defined(SHIMMER3R)
    CalibSaveFromInfoMemToCalibDump(SC_SENSOR_LSM6DSV_ACCEL);
#endif

    update_calib_dump_file = 1;
    //calib_update = 1;
    //calib_sensor = S_ACCEL_A;
    break;
  case GET_LN_ACCEL_CALIBRATION_COMMAND:
    lnAccelCalibrationResponse = 1;
    break;
  case SET_GYRO_CALIBRATION_COMMAND:
    memcpy(&storedConfig->gyroCalib.rawBytes[0], &args[0], 21);
    InfoMem_write(NV_GYRO_CALIBRATION, &storedConfig->gyroCalib.rawBytes[0], 21);
    S4Ram_sdHeadTextSet(&storedConfig->gyroCalib.rawBytes[0], SDH_GYRO_CALIBRATION, 21);

#if defined(SHIMMER3)
    CalibSaveFromInfoMemToCalibDump(SC_SENSOR_MPU9X50_ICM20948_GYRO);
#elif defined(SHIMMER3R)
    CalibSaveFromInfoMemToCalibDump(SC_SENSOR_LSM6DSV_GYRO);
#endif

    update_calib_dump_file = 1;
    //calib_update = 1;
    //calib_sensor = S_GYRO;
    //calib_range = storedConfig[NV_CONFIG_SETUP_BYTE2] & 0x03;
    break;
  case GET_GYRO_CALIBRATION_COMMAND:
    gyroCalibrationResponse = 1;
    break;
  case SET_MAG_CALIBRATION_COMMAND:
    memcpy(&storedConfig->magCalib.rawBytes[0], &args[0], 21);
    InfoMem_write(NV_MAG_CALIBRATION, &storedConfig->magCalib.rawBytes[0], 21);
    S4Ram_sdHeadTextSet(&storedConfig->magCalib.rawBytes[0], SDH_MAG_CALIBRATION, 21);

#if defined(SHIMMER3)
    CalibSaveFromInfoMemToCalibDump(SC_SENSOR_LSM303_MAG);
#elif defined(SHIMMER3R)
    CalibSaveFromInfoMemToCalibDump(SC_SENSOR_LIS3MDL_MAG);
#endif

    update_calib_dump_file = 1;
    //calib_update = 1;
    //calib_sensor = S_MAG;
    //calib_range = (storedConfig[NV_CONFIG_SETUP_BYTE2] >> 5) & 0x07;
    break;
  case GET_MAG_CALIBRATION_COMMAND:
    magCalibrationResponse = 1;
    break;
  case SET_WR_ACCEL_CALIBRATION_COMMAND:
    memcpy(&storedConfig->wrAccelCalib.rawBytes[0], &args[0], 21);
    InfoMem_write(NV_WR_ACCEL_CALIBRATION, &storedConfig->wrAccelCalib.rawBytes[0], 21);
    S4Ram_sdHeadTextSet(
        &storedConfig->wrAccelCalib.rawBytes[0], SDH_WR_ACCEL_CALIBRATION, 21);

#if defined(SHIMMER3)
    CalibSaveFromInfoMemToCalibDump(SC_SENSOR_LSM303_ACCEL);
#elif defined(SHIMMER3R)
    CalibSaveFromInfoMemToCalibDump(SC_SENSOR_LIS2DW12_ACCEL);
#endif

    update_calib_dump_file = 1;
    //calib_update = 1;
    //calib_sensor = S_ACCEL_D;
    //calib_range = (storedConfig[NV_CONFIG_SETUP_BYTE0] >> 2) & 0x03;
    break;
  case SET_GSR_RANGE_COMMAND:
    storedConfig->gsrRange = (args[0] <= 4) ? (args[0] & 0x07) : GSR_AUTORANGE;

    InfoMem_write(NV_CONFIG_SETUP_BYTE3, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE3], 1);
    S4Ram_sdHeadTextSetByte(
        SDH_CONFIG_SETUP_BYTE3, storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE3]);
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case SET_EXG_REGS_COMMAND:
    if (args[0] < 2 && args[1] < 10 && args[2] < 11)
    {
      if (args[0])
      {
        S4Ram_storedConfigSet(&args[3], NV_EXG_ADS1292R_2_CONFIG1 + args[1], args[2]);
        InfoMem_write(NV_EXG_ADS1292R_2_CONFIG1 + args[1],
            &storedConfig->rawBytes[NV_EXG_ADS1292R_2_CONFIG1 + args[1]], args[2]);
        S4Ram_sdHeadTextSet(&storedConfig->rawBytes[NV_EXG_ADS1292R_2_CONFIG1],
            SDH_EXG_ADS1292R_2_CONFIG1, args[2]);
      }
      else
      {
        S4Ram_storedConfigSet(&args[3], NV_EXG_ADS1292R_1_CONFIG1 + args[1], args[2]);

        if ((getDaughtCardId()->exp_brd_id == EXP_BRD_EXG_UNIFIED)
            && (getDaughtCardId()->exp_brd_rev >= 4))
        {
          /* Check if unit is SR47-4 or greater.
           * If so, amend configuration byte 2 of ADS chip 1 to have bit 3 set
           * to 1. This ensures clock lines on ADS chip are correct
           */
          storedConfig->exgADS1292rRegsCh1.config2 |= 8;
        }

        InfoMem_write(NV_EXG_ADS1292R_1_CONFIG1 + args[1],
            &storedConfig->rawBytes[NV_EXG_ADS1292R_1_CONFIG1 + args[1]], args[2]);
        S4Ram_sdHeadTextSet(&storedConfig->rawBytes[NV_EXG_ADS1292R_1_CONFIG1],
            SDH_EXG_ADS1292R_1_CONFIG1, args[2]);
      }
      update_sdconfig = 1;
    }
    break;
  case GET_BT_VERSION_STR_COMMAND:
    btVerResponse = 1;
    break;
  case SET_DATA_RATE_TEST:
    /* Stop test before ACK is sent */
    if (args[0] == 0)
    {
      setBtDataRateTestState(0);
      clearBtTxBuf(1);
    }
    btDataRateTestResponse = 1;
    break;
  case SET_FACTORY_TEST:
    if (args[0] < FACTORY_TEST_COUNT)
    {
      setup_factory_test(PRINT_TO_BT_UART, (factory_test_t) args[0]);
#if defined(SHIMMER3)
      TaskSet(TASK_FACTORY_TEST);
#else
      S4_Task_set(TASK_FACTORY_TEST);
#endif
    }
    break;

  case RESET_TO_DEFAULT_CONFIGURATION_COMMAND:
    S4Ram_SetDefaultInfomem();
    S4Ram_config2SdHead();
    update_sdconfig = 1;
    if (shimmerStatus.isSensing)
    {
      setStopSensing();
      setStartSensing();
    }
    break;
  case RESET_CALIBRATION_VALUE_COMMAND:
    //memset(&storedConfig[NV_A_ACCEL_CALIBRATION], 0xFF,
    //NV_NUM_CALIBRATION_BYTES); InfoMem_write((void*)NV_A_ACCEL_CALIBRATION,
    //&storedConfig[NV_A_ACCEL_CALIBRATION], NV_NUM_CALIBRATION_BYTES);
    //memcpy(&sdHeadText[SDH_LSM303DLHC_ACCEL_CALIBRATION], &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION],
    //21); memcpy(&sdHeadText[SDH_MPU9250_GYRO_CALIBRATION], &storedConfig[NV_MPU9250_GYRO_CALIBRATION],
    //21); memcpy(&sdHeadText[SDH_LSM303DLHC_MAG_CALIBRATION], &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION],
    //21); memcpy(&sdHeadText[SDH_A_ACCEL_CALIBRATION], &storedConfig[NV_A_ACCEL_CALIBRATION],
    //21); calib_update = 1;
    ShimmerCalib_init();
    ShimmerCalibSyncFromDumpRamAll();
    update_calib_dump_file = 1;
    break;

  case GET_WR_ACCEL_CALIBRATION_COMMAND:
    wrAccelCalibrationResponse = 1;
    break;
  case GET_GSR_RANGE_COMMAND:
    gsrRangeResponse = 1;
    break;
  case GET_ALL_CALIBRATION_COMMAND:
    allCalibrationResponse = 1;
    break;

  case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
  case GET_DEVICE_VERSION_COMMAND:
    deviceVersionResponse = 1;
    break;
  case GET_FW_VERSION_COMMAND:
    fwVersionResponse = 1;
    break;
  case GET_CHARGE_STATUS_LED_COMMAND:
    blinkLedResponse = 1;
    break;
  case GET_BUFFER_SIZE_COMMAND:
    bufferSizeResponse = 1;
    break;
  case GET_UNIQUE_SERIAL_COMMAND:
    uniqueSerialResponse = 1;
    break;
  case GET_DAUGHTER_CARD_ID_COMMAND:
    dcMemLength = args[0];
    dcMemOffset = args[1];
    if ((dcMemLength <= 16) && (dcMemOffset <= 15) && (dcMemLength + dcMemOffset <= 16))
      dcIdResponse = 1;
    break;
  case SET_DAUGHTER_CARD_ID_COMMAND:
    dcMemLength = args[0];
    dcMemOffset = args[1];
    if ((dcMemLength <= 16) && (dcMemOffset <= 15) && (dcMemLength + dcMemOffset <= 16))
    {
      eepromWrite(dcMemOffset, dcMemLength, &args[2]);
    }
    break;
  case GET_DAUGHTER_CARD_MEM_COMMAND:
    dcMemLength = args[0];
    dcMemOffset = args[1] + (args[2] << 8);
    if ((dcMemLength <= 128) && (dcMemOffset <= 2031) && (dcMemLength + dcMemOffset <= 2032))
      dcMemResponse = 1;
    break;
  case SET_DAUGHTER_CARD_MEM_COMMAND:
    dcMemLength = args[0];
    dcMemOffset = args[1] + (args[2] << 8);
    if ((dcMemLength <= 128) && (dcMemOffset <= 2031) && (dcMemLength + dcMemOffset <= 2032))
    {
      eepromWrite(dcMemOffset + 16U, (uint16_t) dcMemLength, &args[3]);
    }
    break;
  case GET_BT_COMMS_BAUD_RATE:
    btCommsBaudRateResponse = 1;
    break;
  case SET_BT_COMMS_BAUD_RATE:
    //TODO changing BAUD rate is not going to be supported
    //if(btArgs[0] != storedConfig->btCommsBaudRate) {
    //  if(btArgs[0]<11) {
    //    changeBtBaudRate = btArgs[0];
    //  } else {
    //    changeBtBaudRate = 9;
    //  }
    //}
    break;
  case GET_DERIVED_CHANNEL_BYTES:
    derivedChannelResponse = 1;
    break;
  case SET_DERIVED_CHANNEL_BYTES:
    memcpy(&storedConfig->rawBytes[NV_DERIVED_CHANNELS_0], &args[0], 3);
    memcpy(&storedConfig->rawBytes[NV_DERIVED_CHANNELS_3], &args[3], 5);
    InfoMem_write(NV_DERIVED_CHANNELS_0, &storedConfig->rawBytes[NV_DERIVED_CHANNELS_0], 3);
    InfoMem_write(NV_DERIVED_CHANNELS_3, &storedConfig->rawBytes[NV_DERIVED_CHANNELS_3], 5);
    S4Ram_sdHeadTextSet(&storedConfig->rawBytes[NV_DERIVED_CHANNELS_0],
        SDH_DERIVED_CHANNELS_0, 3);
    S4Ram_sdHeadTextSet(&storedConfig->rawBytes[NV_DERIVED_CHANNELS_3],
        SDH_DERIVED_CHANNELS_3, 5);
    update_sdconfig = 1;
    break;
  case GET_INFOMEM_COMMAND:
    infomemLength = args[0];
    infomemOffset = args[1] + (args[2] << 8);
    if ((infomemLength <= 128) && (infomemOffset <= (NV_NUM_RWMEM_BYTES - 1))
        && (infomemLength + infomemOffset <= NV_NUM_RWMEM_BYTES))
      infomemResponse = 1;
    break;
  case SET_INFOMEM_COMMAND:
    uint8_t temp_btMacHex[6];

    infomemLength = args[0];
    infomemOffset = args[1] + (args[2] << 8);
    if ((infomemLength <= 128) && (infomemOffset <= (NV_NUM_RWMEM_BYTES - 1))
        && (infomemLength + infomemOffset <= NV_NUM_RWMEM_BYTES))
    {
      if (infomemOffset == (INFOMEM_SEG_C_ADDR_MSP430 - INFOMEM_OFFSET_MSP430))
      {
        /* Read MAC address so it is not forgotten */
        InfoMem_readRam(&temp_btMacHex[0], NV_MAC_ADDRESS, 6);
        /* Re-write MAC address to Infomem */
        memcpy(&args[3 + NV_MAC_ADDRESS - 128], &temp_btMacHex[0], 6);
      }
      if (infomemOffset == (INFOMEM_SEG_D_ADDR_MSP430 - INFOMEM_OFFSET_MSP430))
      {
        /* Check if unit is SR47-4 or greater.
         * If so, amend configuration byte 2 of ADS chip 1 to have bit 3 set
         * to 1. This ensures clock lines on ADS chip are correct
         */
        if ((getDaughtCardId()->exp_brd_id == EXP_BRD_EXG_UNIFIED)
            && (getDaughtCardId()->exp_brd_rev >= 4))
        {
          args[3 + NV_EXG_ADS1292R_1_CONFIG2] |= 8;
        }
      }

#if !IS_SUPPORTED_TCXO
      //Disable TXCO
      if (infomemOffset <= NV_SD_TRIAL_CONFIG1 && NV_SD_TRIAL_CONFIG1 <= infomemOffset + infomemLength)
      {
        uint8_t tcxoInfomemOffset = NV_SD_TRIAL_CONFIG1 - infomemOffset;
        args[3 + tcxoInfomemOffset] &= ~SDH_TCXO;
      }
#endif

      S4Ram_storedConfigSet(&args[3], infomemOffset, infomemLength);
      InfoMem_write(infomemOffset, &args[3], infomemLength);
      InfoMem_readRam(&storedConfig->rawBytes[infomemOffset], infomemOffset, infomemLength);

      if (infomemOffset == (INFOMEM_SEG_D_ADDR_MSP430 - INFOMEM_OFFSET_MSP430))
      {
        CalibSaveFromInfoMemToCalibDump(0xFF);
      }

      S4Ram_config2SdHead();
      SD_infomem2Names();
      update_sdconfig = 1;
      if (((infomemOffset >= NV_LN_ACCEL_CALIBRATION) && (infomemOffset <= NV_CALIBRATION_END))
          || (((infomemLength + infomemOffset) >= NV_LN_ACCEL_CALIBRATION)
              && ((infomemLength + infomemOffset) <= NV_CALIBRATION_END))
          || ((infomemOffset <= NV_LN_ACCEL_CALIBRATION)
              && ((infomemLength + infomemOffset) >= NV_CALIBRATION_END)))
      {
        ShimmerCalibUpdateFromInfoAll();
        update_calib_dump_file = 1;
      }
    }
    else
    {
      return;
    }
    break;
  case GET_RWC_COMMAND:
    rwcResponse = 1;
    break;
  case SET_RWC_COMMAND:
    memcpy((uint8_t *) (&temp64), args, 8); //64bits = 8bytes
    RTC_init(temp64);
    storedConfig->rtcSetByBt = 1;
    InfoMem_write(NV_SD_TRIAL_CONFIG0, &storedConfig->rawBytes[NV_SD_TRIAL_CONFIG0], 1);

    S4Ram_sdHeadTextSetByte(SDH_TRIAL_CONFIG0, storedConfig->rawBytes[NV_SD_TRIAL_CONFIG0]);

    //TODO decide if the following is needed
    //uint64_t * rwcTimeDiffPtr = getRwcTimeDiffPtr();
    //S4Ram_sdHeadTextSetByte(SDH_RTC_DIFF_7, *((uint8_t*) rwcTimeDiffPtr));
    //S4Ram_sdHeadTextSetByte(SDH_RTC_DIFF_6, *(((uint8_t*) rwcTimeDiffPtr) + 1));
    //S4Ram_sdHeadTextSetByte(SDH_RTC_DIFF_5, *(((uint8_t*) rwcTimeDiffPtr) + 2));
    //S4Ram_sdHeadTextSetByte(SDH_RTC_DIFF_4, *(((uint8_t*) rwcTimeDiffPtr) + 3));
    //S4Ram_sdHeadTextSetByte(SDH_RTC_DIFF_3, *(((uint8_t*) rwcTimeDiffPtr) + 4));
    //S4Ram_sdHeadTextSetByte(SDH_RTC_DIFF_2, *(((uint8_t*) rwcTimeDiffPtr) + 5));
    //S4Ram_sdHeadTextSetByte(SDH_RTC_DIFF_1, *(((uint8_t*) rwcTimeDiffPtr) + 6));
    //S4Ram_sdHeadTextSetByte(SDH_RTC_DIFF_0, *(((uint8_t*) rwcTimeDiffPtr) + 7));

    setupNextRtcMinuteAlarm(); //configure RTC alarm after time set from BT.
    break;
#if USE_OLD_SD_SYNC_APPROACH
  case ACK_COMMAND_PROCESSED:
#else
  case SET_SD_SYNC_COMMAND:
#endif
    /* Reassemble full packet so that original RcNodeR10() will work without modificiation */
    fullSyncResp[0] = gAction;
    memcpy(&fullSyncResp[1], &args[0], SYNC_PACKET_MAX_SIZE - SYNC_PACKET_SIZE_CMD);
    setSyncResp(&fullSyncResp[0], SYNC_PACKET_MAX_SIZE);
    S4_Task_set(TASK_RCNODER10);
#if !USE_OLD_SD_SYNC_APPROACH
  case ACK_COMMAND_PROCESSED:
    /* Slave response received by Master */
    if (args[0] == SD_SYNC_RESPONSE)
    {
      /* SD Sync Center - get's into this case when the center is waiting for a 0x01 or 0xFF from a node */
      setSyncResp(&args[1], 1U);
      S4_Task_set(TASK_RCCENTERR1);
    }
    break;
#endif
  default:;
  }
  /* Send ACK back for all commands except when FW has received an ACK */
  if (gAction != ACK_COMMAND_PROCESSED
      /* ACK is sent back as part of SD_SYNC_RESPONSE so no need to send it here */
      && gAction != SET_SD_SYNC_COMMAND)
  {
    sendAck = 1;
    S4_Task_set(TASK_BTRESPONSE);
  }

  if (update_sdconfig)
  {
    SetSdCfgFlag(1);
  }
  if (update_calib_dump_file && CheckSdInslot() && !shimmerStatus.badFile)
  {
    if (!shimmerStatus.isDocked)
    {
      ShimmerCalib_ram2File();
    }
    else
    {
      SetRamCalibFlag(1);
    }
  }
}

void BtUart_sendRsp(void)
{
  sc_t sc1;
  uint16_t packet_length = 0;
  //STATTypeDef * stat = GetStatus();
  uint8_t resPacket[RESPONSE_PACKET_SIZE];
  packet_length = 0;

  gConfigBytes *storedConfig = S4Ram_getStoredConfig();

  if (shimmerStatus.isBtConnected)
  {
    if (sendAck)
    {
      *(resPacket + packet_length++) = ACK_COMMAND_PROCESSED;
      sendAck = 0;
    }
    if (inquiryResponse)
    {
      /* Channel order/packet structure need to be assembled before sending the inquiry response so that the information is correct. */
      S4Sens_configureChannels();

      *(resPacket + packet_length++) = INQUIRY_RESPONSE;
      *(uint16_t *) (resPacket + packet_length) = storedConfig->samplingRateTicks; //ADC sampling rate
      packet_length += 2;
      memcpy(resPacket + packet_length, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0], 4);
      packet_length += 4;
      *(resPacket + packet_length++) = sensing.nbrAdcChans + sensing.nbrDigiChans; //number of data channels
      *(resPacket + packet_length++) = storedConfig->bufferSize; //buffer size
      memcpy((resPacket + packet_length), sensing.cc,
          (sensing.nbrAdcChans + sensing.nbrDigiChans));
      packet_length += sensing.nbrAdcChans + sensing.nbrDigiChans;
      inquiryResponse = 0;
    }
    else if (samplingRateResponse)
    {
      *(resPacket + packet_length++) = SAMPLING_RATE_RESPONSE;
      *(uint16_t *) (resPacket + packet_length) = storedConfig->samplingRateTicks; //ADC sampling rate
      packet_length += 2;
      samplingRateResponse = 0;
    }
    else if (wrAccelRangeResponse)
    {
      *(resPacket + packet_length++) = WR_ACCEL_RANGE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->wrAccelRange;
      wrAccelRangeResponse = 0;
    }
    else if (magGainResponse)
    {
      *(resPacket + packet_length++) = MAG_GAIN_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->magRange;
      magGainResponse = 0;
    }
    else if (magSamplingRateResponse)
    {
      *(resPacket + packet_length++) = MAG_SAMPLING_RATE_RESPONSE;
      *(resPacket + packet_length++) = get_config_byte_mag_rate();
      magSamplingRateResponse = 0;
    }
    else if (dockedResponse)
    {
      *(resPacket + packet_length++) = INSTREAM_CMD_RESPONSE;
      *(resPacket + packet_length++) = STATUS_RESPONSE;
      *(resPacket + packet_length++) = (shimmerStatus.toggleLedRedCmd << 7)
          + ((shimmerStatus.badFile & 0x01) << 6)
          + ((shimmerStatus.isSdInserted & 0x01) << 5)
          + ((shimmerStatus.isStreaming & 0x01) << 4)
          + ((shimmerStatus.isLogging & 0x01) << 3) + (isRwcTimeSet() << 2)
          + ((shimmerStatus.isSensing & 0x01) << 1) + (shimmerStatus.isDocked & 0x01);
      dockedResponse = 0;
#if defined(SHIMMER4_SDK)
    }
    else if (i2cvBattBtRsp)
    {
      *(resPacket + packet_length++) = INSTREAM_CMD_RESPONSE;
      *(resPacket + packet_length++) = RSP_I2C_BATT_STATUS_COMMAND;
      memcpy((resPacket + packet_length), (uint8_t *) shimmerStatus.battDigital, STC3100_DATA_LEN);
      packet_length += STC3100_DATA_LEN;
      i2cvBattBtRsp = 0;
#endif
    }
    else if (btVbattResponse)
    {
      manageReadBatt(1);
      *(resPacket + packet_length++) = INSTREAM_CMD_RESPONSE;
      *(resPacket + packet_length++) = VBATT_RESPONSE;
      memcpy((uint8_t *) (resPacket + packet_length), (uint8_t *) shimmerStatus.battVal, 3);
      packet_length += 3;
      btVbattResponse = 0;
    }
    else if (trialConfigResponse)
    {
      *(resPacket + packet_length++) = TRIAL_CONFIG_RESPONSE;
      //2 trial config bytes + 1 interval byte
      S4Ram_storedConfigGet(&resPacket[packet_length], NV_SD_TRIAL_CONFIG0, 3);
      packet_length += 3;
      trialConfigResponse = 0;
    }
    else if (centerResponse)
    {
      centerResponse = 0;
    }
    else if (shimmerNameResponse)
    {
      SD_setShimmerName();
      uint8_t shimmer_name_len = strlen((char *) storedConfig->shimmerName);
      shimmer_name_len = shimmer_name_len > (MAX_CHARS - 1) ? (MAX_CHARS - 1) : shimmer_name_len;
      *(resPacket + packet_length++) = SHIMMERNAME_RESPONSE;
      *(resPacket + packet_length++) = shimmer_name_len;
      memcpy((resPacket + packet_length), &storedConfig->shimmerName[0], shimmer_name_len);
      packet_length += shimmer_name_len;
      shimmerNameResponse = 0;
    }
    else if (expIDResponse)
    {
      SD_setExpIdName();
      uint8_t exp_id_name_len = strlen((char *) storedConfig->expIdName);
      exp_id_name_len = exp_id_name_len > (MAX_CHARS - 1) ? (MAX_CHARS - 1) : exp_id_name_len;
      *(resPacket + packet_length++) = EXPID_RESPONSE;
      *(resPacket + packet_length++) = exp_id_name_len;
      memcpy((resPacket + packet_length), &storedConfig->expIdName[0], exp_id_name_len);
      packet_length += exp_id_name_len;
      expIDResponse = 0;
    }
    else if (configTimeResponse)
    {
      SD_setCfgTime();
      uint8_t *configTimeTextPtr = getConfigTimeTextPtr();
      uint8_t cfgtime_name_len = strlen((char *) configTimeTextPtr);
      *(resPacket + packet_length++) = CONFIGTIME_RESPONSE;
      *(resPacket + packet_length++) = cfgtime_name_len;
      memcpy((resPacket + packet_length), configTimeTextPtr, cfgtime_name_len);
      packet_length += cfgtime_name_len;
      configTimeResponse = 0;
    }
    else if (dirResponse)
    {
      uint8_t *fileNamePtr = getFileNamePtr();
      uint8_t dir_len = strlen((char *) fileNamePtr) - 3;
      *(resPacket + packet_length++) = INSTREAM_CMD_RESPONSE;
      *(resPacket + packet_length++) = DIR_RESPONSE;
      *(resPacket + packet_length++) = dir_len;
      memcpy((resPacket + packet_length), fileNamePtr, dir_len);
      packet_length += dir_len;
      dirResponse = 0;
    }
    else if (nshimmerResponse)
    {
      *(resPacket + packet_length++) = NSHIMMER_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->numberOfShimmers;
      nshimmerResponse = 0;
    }
    else if (myIDResponse)
    {
      *(resPacket + packet_length++) = MYID_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->myTrialID;
      myIDResponse = 0;
    }
    else if (wrAccelSamplingRateResponse)
    {
      *(resPacket + packet_length++) = WR_ACCEL_SAMPLING_RATE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->wrAccelRate;
      wrAccelSamplingRateResponse = 0;
    }
    else if (wrAccelLpModeResponse)
    {
      *(resPacket + packet_length++) = WR_ACCEL_LPMODE_RESPONSE;
      *(resPacket + packet_length++) = get_config_byte_wr_accel_lp_mode();
      wrAccelLpModeResponse = 0;
    }
    else if (wrAccelHrModeResponse)
    {
      *(resPacket + packet_length++) = WR_ACCEL_HRMODE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->wrAccelHrMode;
      wrAccelHrModeResponse = 0;
    }
    else if (gyroRangeResponse)
    {
      *(resPacket + packet_length++) = GYRO_RANGE_RESPONSE;
      *(resPacket + packet_length++) = get_config_byte_gyro_range();
      gyroRangeResponse = 0;
    }
    else if (bmp180CalibrationCoefficientsResponse)
    {
      *(resPacket + packet_length++) = BMP180_CALIBRATION_COEFFICIENTS_RESPONSE;
      if (!isBmp180InUse())
      {
        //Dummy bytes sent if incorrect calibration bytes requested.
        memset(resPacket + packet_length, 0x01, BMP180_CALIB_DATA_SIZE);
      }
      else
      {
        memcpy(resPacket + packet_length, get_bmp_calib_data_bytes(), BMP180_CALIB_DATA_SIZE);
      }
      packet_length += BMP180_CALIB_DATA_SIZE;

      bmp180CalibrationCoefficientsResponse = 0;
    }
    else if (bmp280CalibrationCoefficientsResponse)
    {
      *(resPacket + packet_length++) = BMP280_CALIBRATION_COEFFICIENTS_RESPONSE;
      if (!isBmp280InUse())
      {
        //Dummy bytes sent if incorrect calibration bytes requested.
        memset(resPacket + packet_length, 0x01, BMP280_CALIB_DATA_SIZE);
      }
      else
      {
        memcpy(resPacket + packet_length, get_bmp_calib_data_bytes(), BMP280_CALIB_DATA_SIZE);
      }
      packet_length += BMP280_CALIB_DATA_SIZE;

      bmp280CalibrationCoefficientsResponse = 0;
    }
    else if (bmpGenericCalibrationCoefficientsResponse)
    {
      uint8_t bmpCalibByteLen = get_bmp_calib_data_bytes_len();
      *(resPacket + packet_length++) = PRESSURE_CALIBRATION_COEFFICIENTS_RESPONSE;
      *(resPacket + packet_length++) = 1U + bmpCalibByteLen;
      if (isBmp180InUse())
      {
        *(resPacket + packet_length++) = PRESSURE_SENSOR_BMP180;
      }
      else if (isBmp280InUse())
      {
        *(resPacket + packet_length++) = PRESSURE_SENSOR_BMP280;
      }
      else
      {
        *(resPacket + packet_length++) = PRESSURE_SENSOR_BMP390;
      }
      memcpy(resPacket + packet_length, get_bmp_calib_data_bytes(), bmpCalibByteLen);
      packet_length += bmpCalibByteLen;
      bmpGenericCalibrationCoefficientsResponse = 0;
    }
    else if (gyroSamplingRateResponse)
    {
      *(resPacket + packet_length++) = GYRO_SAMPLING_RATE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->gyroRate;
      gyroSamplingRateResponse = 0;
    }
    else if (altAccelRangeResponse)
    {
      *(resPacket + packet_length++) = ALT_ACCEL_RANGE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->altAccelRange;
      altAccelRangeResponse = 0;
    }
    else if (bmpOversamplingRatioResponse)
    {
      *(resPacket + packet_length++) = PRESSURE_OVERSAMPLING_RATIO_RESPONSE;
      *(resPacket + packet_length++) = get_config_byte_pressure_oversampling_ratio();
      bmpOversamplingRatioResponse = 0;
    }
    else if (internalExpPowerEnableResponse)
    {
      *(resPacket + packet_length++) = INTERNAL_EXP_POWER_ENABLE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->expansionBoardPower;
      internalExpPowerEnableResponse = 0;
    }
    else if (configSetupBytesResponse)
    {
      *(resPacket + packet_length++) = CONFIG_SETUP_BYTES_RESPONSE;
      memcpy(resPacket + packet_length, &storedConfig->rawBytes[NV_CONFIG_SETUP_BYTE0], 4);
      packet_length += 4;
      configSetupBytesResponse = 0;
    }
    else if (calibRamResponse)
    {
      *(resPacket + packet_length++) = RSP_CALIB_DUMP_COMMAND;
      *(resPacket + packet_length++) = calibRamLength;
      *(resPacket + packet_length++) = calibRamOffset & 0xff;
      *(resPacket + packet_length++) = (calibRamOffset >> 8) & 0xff;
      ShimmerCalib_ramRead(resPacket + packet_length, calibRamLength, calibRamOffset);
      packet_length += calibRamLength;
      calibRamResponse = 0;
    }
    else if (lnAccelCalibrationResponse)
    {
      *(resPacket + packet_length++) = LN_ACCEL_CALIBRATION_RESPONSE;
      //memcpy((resPacket + packet_length),
      //&storedConfig->lnAccelCalib.rawBytes[0], 21); packet_length += 21;

#if defined(SHIMMER3)
      sc1.id = SC_SENSOR_ANALOG_ACCEL;
      sc1.range = SC_SENSOR_RANGE_ANALOG_ACCEL;
#elif defined(SHIMMER3R)
      sc1.id = SC_SENSOR_LSM6DSV_ACCEL;
      sc1.range = storedConfig->altAccelRange;
#endif
      sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
      ShimmerCalib_singleSensorRead(&sc1);
      memcpy((resPacket + packet_length), sc1.data.raw, sc1.data_len);
      packet_length += sc1.data_len;

      lnAccelCalibrationResponse = 0;
    }
    else if (gyroCalibrationResponse)
    {
      *(resPacket + packet_length++) = GYRO_CALIBRATION_RESPONSE;
      //memcpy((resPacket + packet_length),
      //&storedConfig->gyroCalib.rawBytes[0], 21); packet_length += 21;

#if defined(SHIMMER3)
      sc1.id = SC_SENSOR_MPU9X50_ICM20948_GYRO;
#elif defined(SHIMMER3R)
      sc1.id = SC_SENSOR_LSM6DSV_GYRO;
#endif
      sc1.range = get_config_byte_gyro_range();
      sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
      ShimmerCalib_singleSensorRead(&sc1);
      memcpy((resPacket + packet_length), sc1.data.raw, sc1.data_len);
      packet_length += sc1.data_len;

      gyroCalibrationResponse = 0;
    }
    else if (magCalibrationResponse)
    {
      *(resPacket + packet_length++) = MAG_CALIBRATION_RESPONSE;
      //memcpy((resPacket + packet_length), &storedConfig->magCalib.rawBytes[0],
      //21); packet_length += 21;

#if defined(SHIMMER3)
      sc1.id = SC_SENSOR_LSM303_MAG;
#elif defined(SHIMMER3R)
      sc1.id = SC_SENSOR_LIS3MDL_MAG;
#endif
      sc1.range = storedConfig->magRange;
      sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
      ShimmerCalib_singleSensorRead(&sc1);
      memcpy((resPacket + packet_length), sc1.data.raw, sc1.data_len);
      packet_length += sc1.data_len;

      magCalibrationResponse = 0;
    }
    else if (wrAccelCalibrationResponse)
    {
      *(resPacket + packet_length++) = WR_ACCEL_CALIBRATION_RESPONSE;
      //memcpy((resPacket + packet_length),
      //&storedConfig->wrAccelCalib.rawBytes[0], 21); packet_length += 21;

#if defined(SHIMMER3)
      sc1.id = SC_SENSOR_LSM303_ACCEL;
#elif defined(SHIMMER3R)
      sc1.id = SC_SENSOR_LIS2DW12_ACCEL;
#endif
      sc1.range = storedConfig->wrAccelRange;
      sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
      ShimmerCalib_singleSensorRead(&sc1);
      memcpy((resPacket + packet_length), sc1.data.raw, sc1.data_len);
      packet_length += sc1.data_len;

      wrAccelCalibrationResponse = 0;
    }
    else if (gsrRangeResponse)
    {
      *(resPacket + packet_length++) = GSR_RANGE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->gsrRange;
      gsrRangeResponse = 0;
    }
    else if (allCalibrationResponse)
    {
      *(resPacket + packet_length++) = ALL_CALIBRATION_RESPONSE;
      //S4Ram_storedConfigGet(&resPacket[packet_length], NV_A_ACCEL_CALIBRATION,
      //    NV_NUM_CALIBRATION_BYTES);
      //packet_length += NV_NUM_CALIBRATION_BYTES;

      uint8_t i;
      for (i = 0; i < 4; i++)
      {
        if (i == 0)
        {
#if defined(SHIMMER3)
          sc1.id = SC_SENSOR_ANALOG_ACCEL;
          sc1.range = SC_SENSOR_RANGE_ANALOG_ACCEL;
#elif defined(SHIMMER3R)
          sc1.id = SC_SENSOR_LSM6DSV_ACCEL;
          sc1.range = storedConfig->altAccelRange;
#endif
          sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
        }
        else if (i == 1)
        {
#if defined(SHIMMER3)
          sc1.id = SC_SENSOR_MPU9X50_ICM20948_GYRO;
#elif defined(SHIMMER3R)
          sc1.id = SC_SENSOR_LSM6DSV_GYRO;
#endif
          sc1.range = get_config_byte_gyro_range();
          sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
        }
        else if (i == 2)
        {
#if defined(SHIMMER3)
          sc1.id = SC_SENSOR_LSM303_MAG;
#elif defined(SHIMMER3R)
          sc1.id = SC_SENSOR_LIS3MDL_MAG;
#endif
          sc1.range = storedConfig->magRange;
          sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
        }
        else if (i == 3)
        {
#if defined(SHIMMER3)
          sc1.id = SC_SENSOR_LSM303_ACCEL;
#elif defined(SHIMMER3R)
          sc1.id = SC_SENSOR_LIS2DW12_ACCEL;
#endif
          sc1.range = storedConfig->wrAccelRange;
          sc1.data_len = SC_DATA_LEN_STD_IMU_CALIB;
        }
        ShimmerCalib_singleSensorRead(&sc1);
        memcpy((resPacket + packet_length), sc1.data.raw, sc1.data_len);
        packet_length += sc1.data_len;
      }

      allCalibrationResponse = 0;
    }
    else if (deviceVersionResponse)
    {
      *(resPacket + packet_length++) = DEVICE_VERSION_RESPONSE;
      *(resPacket + packet_length++) = DEVICE_VER;
      deviceVersionResponse = 0;
    }
    else if (mpu9150MagSensAdjValsResponse)
    {
#if defined(SHIMMER3) || defined(SHIMMER4_SDK)
      //Mag sensitivity adj feature is not present in ICM-20948
      if (isGyroInUseMpu9x50())
      {
        MPU9150_init();
        MPU9150_wake(1);
        MPU9150_wake(0);
        *(resPacket + packet_length++) = MPU9150_MAG_SENS_ADJ_VALS_RESPONSE;
        MPU9150_getMagSensitivityAdj(resPacket + packet_length);
        packet_length += 3;
      }
      else
      {
#endif
        *(resPacket + packet_length++) = ACK_COMMAND_PROCESSED;
#if defined(SHIMMER3) || defined(SHIMMER4_SDK)
      }
#endif
      mpu9150MagSensAdjValsResponse = 0;
    }
    else if (fwVersionResponse)
    {
      *(resPacket + packet_length++) = FW_VERSION_RESPONSE;
      *(resPacket + packet_length++) = FW_IDENTIFIER & 0xFF;
      *(resPacket + packet_length++) = (FW_IDENTIFIER & 0xFF00) >> 8;
      *(resPacket + packet_length++) = FW_VER_MAJOR & 0xFF;
      *(resPacket + packet_length++) = (FW_VER_MAJOR & 0xFF00) >> 8;
      *(resPacket + packet_length++) = FW_VER_MINOR;
      *(resPacket + packet_length++) = FW_VER_REL;
      fwVersionResponse = 0;
    }
    else if (blinkLedResponse)
    {
      *(resPacket + packet_length++) = CHARGE_STATUS_LED_RESPONSE;
      *(resPacket + packet_length++) = shimmerStatus.battStat;
      blinkLedResponse = 0;
    }
    else if (bufferSizeResponse)
    {
      *(resPacket + packet_length++) = BUFFER_SIZE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->bufferSize;
      bufferSizeResponse = 0;
    }
    else if (uniqueSerialResponse)
    {
      *(resPacket + packet_length++) = UNIQUE_SERIAL_RESPONSE;
      uint32_t uid[3];
      uid[0] = HAL_GetUIDw0();
      uid[1] = HAL_GetUIDw1();
      uid[2] = HAL_GetUIDw2();
      memcpy((resPacket + packet_length), (uint8_t *) &uid[0], 12);
      packet_length += 12;
      uniqueSerialResponse = 0;
    }
    else if (exgRegsResponse)
    {
      *(resPacket + packet_length++) = EXG_REGS_RESPONSE;
      *(resPacket + packet_length++) = exgLength;
      if (exgLength)
      {
        if (exgChip)
          memcpy((resPacket + packet_length),
              &storedConfig->exgADS1292rRegsCh2.rawBytes[0], exgLength);
        else
          memcpy((resPacket + packet_length),
              &storedConfig->exgADS1292rRegsCh1.rawBytes[0], exgLength);
        packet_length += exgLength;
      }
      exgRegsResponse = 0;
    }
    else if (dcIdResponse)
    {
      *(resPacket + packet_length++) = DAUGHTER_CARD_ID_RESPONSE;
      *(resPacket + packet_length++) = dcMemLength;
      memcpy(resPacket + packet_length, getDaughtCardId() + dcMemOffset, dcMemLength);
      packet_length += dcMemLength;
      dcIdResponse = 0;
    }
    else if (dcMemResponse)
    {
      *(resPacket + packet_length++) = DAUGHTER_CARD_MEM_RESPONSE;
      *(resPacket + packet_length++) = dcMemLength;
      if (!shimmerStatus.isSensing)
      {
        eepromRead(dcMemOffset + 16U, dcMemLength, resPacket + packet_length);
      }
      else
      {
        memset(resPacket + packet_length, 0xff, dcMemLength);
      }
      packet_length += dcMemLength;
      dcMemResponse = 0;
    }
    else if (btCommsBaudRateResponse)
    {
      *(resPacket + packet_length++) = BT_COMMS_BAUD_RATE_RESPONSE;
      *(resPacket + packet_length++) = storedConfig->btCommsBaudRate;
      btCommsBaudRateResponse = 0;
    }
    else if (infomemResponse)
    {
      *(resPacket + packet_length++) = INFOMEM_RESPONSE;
      *(resPacket + packet_length++) = infomemLength;
      S4Ram_storedConfigGet(&resPacket[packet_length], infomemOffset, infomemLength);
      packet_length += infomemLength;
      infomemResponse = 0;
    }
    else if (derivedChannelResponse)
    {
      *(resPacket + packet_length++) = DERIVED_CHANNEL_BYTES_RESPONSE;
      S4Ram_storedConfigGet(&resPacket[packet_length], NV_DERIVED_CHANNELS_0, 3);
      packet_length += 3;
      S4Ram_storedConfigGet(&resPacket[packet_length], NV_DERIVED_CHANNELS_3, 5);
      packet_length += 5;
      derivedChannelResponse = 0;
    }
    else if (rwcResponse)
    {
      uint64_t temp_rtcCurrentTime = RTC_get64();
      *(resPacket + packet_length++) = RWC_RESPONSE;
      memcpy(resPacket + packet_length, (uint8_t *) (&temp_rtcCurrentTime), 8);
      packet_length += 8;
      rwcResponse = 0;
    }
    else if (btVerResponse)
    {
      uint8_t btVerStrLen = getBtVerStrLen();

      *(resPacket + packet_length++) = BT_VERSION_STR_RESPONSE;
      *(resPacket + packet_length++) = btVerStrLen;
      memcpy((resPacket + packet_length), getBtVerStrPtr(), btVerStrLen);
      packet_length += btVerStrLen;

      btVerResponse = 0;
    }
    else if (btDataRateTestResponse)
    {
      /* Start test after ACK is sent - this will be handled by the
       * interrupt after ACK byte is transmitted */
      if (args[0] != 0)
      {
        setBtDataRateTestState(1);
      }
      btDataRateTestResponse = 0;
    }

    uint8_t crcMode = getBtCrcMode();
    if (crcMode != CRC_OFF)
    {
      calculateCrcAndInsert(crcMode, resPacket, packet_length);
      packet_length += crcMode;
    }
    BT_write(resPacket, packet_length);
  }
}

void setDmaWaitingForResponse(uint16_t count)
{
  btRxWaitByteCount = count;
}

uint16_t getBtRxShimmerCommsWaitByteCount(void)
{
  return btRxWaitByteCount;
}

void setBtCrcMode(COMMS_CRC_MODE btCrcModeNew)
{
  btCrcMode = btCrcModeNew;
  //TODO turn on/off peripheral when needed to save power
  //if (btCrcMode == CRC_OFF)
  //{
  //  HAL_CRC_DeInit(hcrc);
  //}
  //else
  //{
  //  MX_CRC_Init();
  //}
}

COMMS_CRC_MODE getBtCrcMode(void)
{
  return btCrcMode;
}

uint8_t BT_getMacAddressAscii(char *macAscii)
{
#if defined(SHIMMER3R)
  //MAC is stored as 6 byte array in CYW20820 library
  uint8_t *macAddrPtr = BT_getCyw20820MacAddressPtr();
  (void) sprintf(macAscii, "%02X%02X%02X%02X%02X%02X", macAddrPtr[5],
      macAddrPtr[4], macAddrPtr[3], macAddrPtr[2], macAddrPtr[1], macAddrPtr[0]);
  return 1;
#elif defined(SHIMMER4_SDK)
  if (BT_getRn42MacAddressPtr(macAddrPtr))
  {
    memcpy(macAscii, macAddrPtr, 12);
    return 0;
  }
  else
  {
    return 1;
  }
#endif
}

uint8_t BT_getMacAddressHex(uint8_t *macHex)
{
#if defined(SHIMMER3R)
  uint8_t *ptr = BT_getCyw20820MacAddressPtr();
  macHex[0] = *(ptr + 5);
  macHex[1] = *(ptr + 4);
  macHex[2] = *(ptr + 3);
  macHex[3] = *(ptr + 2);
  macHex[4] = *(ptr + 1);
  macHex[5] = *(ptr + 0);
  return 1;
#elif defined(SHIMMER4_SDK)
  uint8_t i, pchar[3];
  if (BT_getRn42MacAddressPtr(macAddrPtr))
  {
    //memcpy(mac, rn42Mac, 12);
    pchar[2] = 0;
    for (i = 0; i < 6; i++)
    {
      pchar[0] = macAddrPtr[i * 2];
      pchar[1] = macAddrPtr[i * 2 + 1];
      macHex[i] = strtoul((char *) pchar, 0, 16);
    }
    return 0;
  }
  else
  {
    return 1;
  }
#endif
}

void BtsdSelfcmd(void)
{
  if (shimmerStatus.isBtConnected)
  {
    uint8_t i = 0;
    uint8_t selfcmd[6]; /* max is 6 bytes */

    if (useAckPrefixForInstreamResponses)
    {
      selfcmd[i++] = ACK_COMMAND_PROCESSED;
    }
    selfcmd[i++] = INSTREAM_CMD_RESPONSE;
    selfcmd[i++] = STATUS_RESPONSE;
    selfcmd[i++] = (shimmerStatus.toggleLedRedCmd << 7) | (shimmerStatus.badFile << 6)
        | (shimmerStatus.isSdInserted << 5) | (shimmerStatus.isStreaming << 4)
        | (shimmerStatus.isLogging << 3) | (isRwcTimeSet() << 2)
        | (shimmerStatus.isSensing << 1) | shimmerStatus.isDocked;

    uint8_t crcMode = getBtCrcMode();
    if (crcMode != CRC_OFF)
    {
      calculateCrcAndInsert(crcMode, &selfcmd[0], i);
      i += crcMode; //Ordinal of enum is how many bytes are used
    }

    BT_write(selfcmd, i);
  }
}

void HandleBtRfCommStateChange(uint8_t isConnected)
{
  if (isConnected)
  { //BT is connected
#if BT_DMA_USED_FOR_RX
    resetBtRxVariablesOnConnect();
#endif

    if (!S4Ram_getStoredConfig()->syncEnable)
    {
      if (shimmerStatus.syncEnabled)
      {
        shimmerStatus.btstreamReady = 0;
      }
      else
      {
        shimmerStatus.btstreamReady = 1;

#if defined(SHIMMER3) & BT_DMA_USED_FOR_RX
        setDmaWaitingForResponseIfStatusStrDisabled();
#endif
      }
    }
    else
    {
      if (S4Ram_getStoredConfig()->masterEnable)
      {
        //center sends sync packet and is waiting for response
        if (isBtSdSyncRunning())
        {
#if defined(SHIMMER3) & BT_DMA_USED_FOR_RX
          /* Only need to charge up the DMA if status strings aren't enabled. Otherwise this is handled within the setup/DMA code. */
          setDmaWaitingForResponseIfStatusStrDisabled();
#endif
          SyncCenterT10();
        }
      }
      else
      {
        resetSyncRcNodeR10Cnt();
        //node is waiting for 1 byte ROUTINE_COMMUNICATION(0xE0)
#if defined(SHIMMER3) & BT_DMA_USED_FOR_RX
        /* Only need to charge up the DMA if status strings aren't enabled. Otherwise this is handled within the setup/DMA code. */
        setDmaWaitingForResponseIfStatusStrDisabled();
#endif
      }
    }
  }
  else
  { //BT is disconnected
    if (!S4Ram_getStoredConfig()->syncEnable)
    {
      shimmerStatus.btstreamReady = 0;
      shimmerStatus.btstreamCmd = BT_STREAM_CMD_STATE_STOP;

      setBtDataRateTestState(0);

      clearBtTxBuf(0);
    }
    setBtCrcMode(CRC_OFF);
    /* Revert to default state if changed */
    useAckPrefixForInstreamResponses = 1U;
  }
  if (!S4Ram_getStoredConfig()->syncEnable)
  {
    S4_Task_set(TASK_SDLOG_CFG_UPDATE);
  }
}
