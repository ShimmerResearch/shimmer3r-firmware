/*
 * shimmer_bt_comms.c
 *
 *  Created on: 22 Jun 2022
 *      Author: MarkNolan
 */

#include "shimmer_bt_comms.h"
#include <stdio.h>
#include <string.h>

//#include "msp430.h"

//#include "RN4X.h"
//#include "../5xx_HAL/hal_board.h"
//#include "../5xx_HAL/hal_RTC.h"
//#include "hal_CRC.h"
//#include "../5xx_HAL/hal_CRC.h"
//#include "../../shimmer_btsd.h"
//#include "shimmer_btsd.h"
#include "../S4_App/s4.h"
#include "../S4_App/s4_taskList.h"
#include "../S4_App/s4_sensing.h"
#if BT_DMA_USED_FOR_RX
//#include "../5xx_HAL/hal_DMA.h"
#endif
#include "sd_sync.h"

uint8_t unwrappedResponse[256] = {0};
#if !BT_DMA_USED_FOR_RX
char *commandBufPtr;
#endif
#if defined(SHIMMER3)
uint8_t *expectedResponsePtr;
#endif
#if BT_DMA_USED_FOR_RX
uint8_t btArgs[MAX_COMMAND_ARG_SIZE], btWaitingForArgs, btWaitingForArgsLength, btArgsSize, btAction;
volatile uint8_t btStatusStrIndex;

#if defined(SHIMMER3)
volatile char btStatusStr[BT_STAT_STR_LEN_LARGEST+1U]; /* +1 to always have a null char */
volatile char btRxBuffFullResponse[BT_VER_RESPONSE_LARGEST+1U]; /* +1 to always have a null char */
#endif
//uint8_t btRxBuff[MAX_COMMAND_ARG_SIZE], *btRxExp;
uint8_t *btRxBuffPtr;
#else
RingFifoRx_t *gBtRxFifoPtr;
/* isRn4678CmdDetectedOnBoot is a workaround to know if it's RN42 or RN4678 before version is parsed */
uint8_t isRn4678CmdDetectedOnBoot = 0;
#endif

#if defined(SHIMMER3)
uint8_t *gActionPtr;
uint8_t *gArgsPtr;
#endif

#if defined(SHIMMER3)
volatile char btVerStrResponse[BT_VER_RESPONSE_LARGEST+1U]; /* +1 to always have a null char */
#else
//TODO decide on size
volatile char btVerStrResponse[100U];
#endif

uint8_t (*newBtCmdToProcess_cb)(void);
#if defined(SHIMMER3)
void (*handleBtRfCommStateChange_cb)(uint8_t);
#endif
void (*setMacId_cb)(uint8_t *);

uint16_t numBytesInBtRxBufWhenLastProcessed = 0;
uint16_t indexOfFirstEol;
uint32_t firstProcessFailTicks = 0;

uint8_t sendAck, inquiryBtRsp, samplingRateBtRsp, toggleLedRed, //enableBtstream, enableSdlog,
        lsm303dlhcAccelRangeResponse, lsm303dlhcMagGainResponse, lsm303dlhcMagSamplingRateResponse, dockStatusBtRsp,
        vbattBtRsp, trialConfigResponse, centerResponse, shimmerNameResponse, expIDResponse, configTimeResponse,
        dirResponse, nshimmerResponse, myIDResponse, lsm303dlhcAccelSamplingRateResponse, i2cvBattBtRsp,
        lsm303dlhcAccelHRModeResponse, mpu9250GyroRangeResponse, bmp180CalibCoeffBtRsp, mpu9250SamplingRateResponse,
        mpu9250AccelRangeResponse, bmp180OversamplingRatioResponse, internalExpPowerEnableResponse,
        exgRegsResponse, configSetupBytesResponse, fwVersionBtRsp, blinkLedBtRsp, infomemBtRsp, dcIdBtRsp, dcMemBtRsp,
        mpu9250MagSensAdjValsResponse, lsm303dlhcAccelLPModeResponse, deviceVersionBtRsp, rwcResponse,
        calibRamResponse, btDataRateResponse, btVerResponse, bmp280CalibrationCoefficientsResponse;//btIsConnected,
uint8_t btInfomemLength, btDcMemLength, btCalibRamLength;
uint16_t btInfomemOffset, btDcMemOffset, btCalibRamOffset;

uint16_t btRxWaitByteCount = 0;

volatile COMMS_CRC_MODE btCrcMode;

#if BT_DMA_USED_FOR_RX
/* Return of 1 brings MSP out of low-power mode */
//uint8_t Dma2ConversionDone(void)
uint8_t Dma2ConversionDone(uint8_t *rxBuff)
{
  btRxBuffPtr = rxBuff;
#if defined(SHIMMER3)
    uint8_t bt_waitForStartCmd, bt_waitForMacAddress, bt_waitForVersion, bt_waitForInitialBoot, bt_waitForReturnNewLine;
#endif
    uint8_t expectedlen = 0U;

#if defined(SHIMMER3)
    DMA2AndCtsDisable();
    bt_waitForStartCmd = BT_getWaitForStartCmd();
    bt_waitForMacAddress = BT_getWaitForMacAddress();
    bt_waitForVersion = BT_getWaitForVersion();
    bt_waitForInitialBoot = BT_getWaitForInitialBoot();
    bt_waitForReturnNewLine = BT_getWaitForReturnNewLine();
#endif

#if defined(SHIMMER3)
    if (!*btRxExp
            && (areBtStatusStringsEnabled()
                    || (isBtConnected() || bt_waitForStartCmd || bt_waitForMacAddress || bt_waitForVersion || bt_waitForInitialBoot || bt_waitForReturnNewLine)))
    {
        if (bt_waitForStartCmd)
        {
            /* RN42 responds with "CMD\r\n" and RN4678 with "CMD> " */
            uint8_t len = strlen((char*) btRxBuffPtr);
            if (len == 5U
                    && btRxBuffPtr[0] == 'C'
                    && btRxBuffPtr[1] == 'M'
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
            memcpy(btRxBuffFullResponse + btOffset, btRxBuffPtr, strlen((char*) btRxBuffPtr));
            memset(btRxBuffPtr, 0, strlen((char*) btRxBuffPtr));

            uint8_t responseLen = strlen(btRxBuffFullResponse);

            if(btRxBuffFullResponse[responseLen-1U]=='\r')
            {
                /* Wait for "\n" */
                setDmaWaitingForResponse(1U);
                return 0;
            }
            // Wait for RN4678_CMD
            else if(isBtDeviceRn4678()
                    && responseLen > RN4X_CMD_LEN
                    && btRxBuffFullResponse[responseLen-5U]=='C'
                    && btRxBuffFullResponse[responseLen-4U]=='M'
                    && btRxBuffFullResponse[responseLen-3U]=='D'
                    && btRxBuffFullResponse[responseLen-2U]=='>'
                    && btRxBuffFullResponse[responseLen-1U]==' ')
            {
                /* Return/New line received */
                BT_setWaitForReturnNewLine(0);
                BT_setGoodCommand();
            }
            else if(btRxBuffFullResponse[responseLen-2U]=='\r'
                    && btRxBuffFullResponse[responseLen-1U]=='\n')
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
            memcpy(btRxBuffFullResponse + btOffset, btRxBuffPtr, strlen((char*) btRxBuffPtr));
            memset(btRxBuffPtr, 0, strlen((char*) btRxBuffPtr));

            uint8_t btVerLen = strlen(btRxBuffFullResponse);
            enum BT_FIRMWARE_VERSION btFwVerNew = BT_FW_VER_UNKNOWN;

            /* RN41 or RN42 */
            if (btRxBuffFullResponse[0U]=='V')
            {
                /* RN41_VERSION_RESPONSE_V4_77 or RN42_VERSION_RESPONSE_V4_77 */
                if (btRxBuffFullResponse[4U]=='4' && btRxBuffFullResponse[5U]=='.')
                {
                    if (btRxBuffFullResponse[9U]=='R' && btRxBuffFullResponse[10U]=='N')
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
                else if (btRxBuffFullResponse[4U]=='6' && btRxBuffFullResponse[5U]=='.' && btRxBuffFullResponse[6U]=='1' && btRxBuffFullResponse[7U]=='5')
                {
                    btVerRemainingChars = RN42_VERSION_RESPONSE_LEN_V6_15;
                    btFwVerNew = RN42_V6_15;
                }
                /* V6.30 not supported */
                else if (btRxBuffFullResponse[4U]=='6' && btRxBuffFullResponse[5U]=='.' && btRxBuffFullResponse[6U]=='3' && btRxBuffFullResponse[7U]=='0')
                {
                    triggerShimmerErrorState();
                }
            }
            /* RN4678 */
            else if (btRxBuffFullResponse[0U]=='R')
            {
                /* RN4678_VERSION_RESPONSE_V1_00_5 */
                if (btRxBuffFullResponse[10U]=='0' && btRxBuffFullResponse[11U]=='0')
                {
                    btVerRemainingChars = RN4678_VERSION_LEN_V1_00_5;
                    btFwVerNew = RN4678_V1_00_5;
                }
                /* RN4678_VERSION_RESPONSE_V1_11_0 */
                else if (btRxBuffFullResponse[10U]=='1' && btRxBuffFullResponse[11U]=='1')
                {
                    btVerRemainingChars = RN4678_VERSION_LEN_V1_11_0;
                    btFwVerNew = RN4678_V1_11_0;
                }
                /* RN4678_VERSION_RESPONSE_V1_13_5 */
                else if (btRxBuffFullResponse[10U]=='1' && btRxBuffFullResponse[11U]=='3')
                {
                    btVerRemainingChars = RN4678_VERSION_LEN_V1_13_5;
                    btFwVerNew = RN4678_V1_13_5;
                }
                /* RN4678_VERSION_RESPONSE_V1_22_0 */
                else if (btRxBuffFullResponse[10U]=='2' && btRxBuffFullResponse[11U]=='2')
                {
                    btVerRemainingChars = RN4678_VERSION_LEN_V1_22_0;
                    btFwVerNew = RN4678_V1_22_0;
                }
                /* RN4678_VERSION_RESPONSE_V1_23_0 */
                else if (btRxBuffFullResponse[10U]=='2' && btRxBuffFullResponse[11U]=='3')
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

            if(btVerRemainingChars)
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

                memset(btRxBuffFullResponse, 0, strlen((char*) btRxBuffFullResponse));
                BT_setWaitForVersion(0);
                BT_setGoodCommand();
            }
        }
#if USE_OLD_SD_SYNC_APPROACH
        else if (getRcommVar())
        {
            /* SD Sync Center - get's into this case when the center is waiting for a 0x01 or 0xFF from a node */
            // 1 byte of RC command
            setRcommResp(btRxBuffPtr, 1U);
            setRcommVar(0);
            setDmaWaitingForResponse(1U);
            return TaskSet(TASK_RCCENTERR1);
        }
#endif
        else
        {
#endif
            if (btWaitingForArgs)
            {
                if ((!btWaitingForArgsLength)
#if defined(SHIMMER3)
                        && ((*(gActionPtr) == SET_EXG_REGS_COMMAND)
#else
                        && ((btAction == SET_EXG_REGS_COMMAND)
#endif
                                && (btWaitingForArgs == 3)))
                {
#if defined(SHIMMER3)
                    gArgsPtr[0] = btRxBuffPtr[0];
                    gArgsPtr[1] = btRxBuffPtr[1];
                    gArgsPtr[2] = btRxBuffPtr[2];
                    btWaitingForArgsLength = gArgsPtr[2];
#else
                    btArgs[0] = btRxBuffPtr[0];
                    btArgs[1] = btRxBuffPtr[1];
                    btArgs[2] = btRxBuffPtr[2];
                    btWaitingForArgsLength = btArgs[2];
#endif
                    setDmaWaitingForResponse(btWaitingForArgsLength);
                    return 0;
                }
                else if ((!btWaitingForArgsLength)
#if defined(SHIMMER3)
                        && (((*(gActionPtr) == SET_INFOMEM_COMMAND)
#else
                        && (((btAction == SET_INFOMEM_COMMAND)
#endif
                                && (btWaitingForArgs == 3))
#if defined(SHIMMER3)
                                || ((*(gActionPtr) == SET_CALIB_DUMP_COMMAND)
#else
                                || ((btAction == SET_CALIB_DUMP_COMMAND)
#endif
                                        && (btWaitingForArgs == 3))))
                {
#if defined(SHIMMER3)
                    gArgsPtr[0] = btRxBuffPtr[0];
                    gArgsPtr[1] = btRxBuffPtr[1];
                    gArgsPtr[2] = btRxBuffPtr[2];
                    btWaitingForArgsLength = gArgsPtr[2];
#else
                    btArgs[0] = btRxBuffPtr[0];
                    btArgs[1] = btRxBuffPtr[1];
                    btArgs[2] = btRxBuffPtr[2];
                    btWaitingForArgsLength = btArgs[0];
#endif
                    setDmaWaitingForResponse(btWaitingForArgsLength);
                    return 0;
                }
                else if ((!btWaitingForArgsLength) && (
                //                ((*(gActionPtr) == SET_DAUGHTER_CARD_ID_COMMAND) && (btWaitingForArgs == 1)) ||
#if defined(SHIMMER3)
                        ((*(gActionPtr) == SET_DAUGHTER_CARD_MEM_COMMAND)
                            && (btWaitingForArgs == 1))
                            || ((*(gActionPtr) == SET_CENTER_COMMAND)
                                    && (btWaitingForArgs == 1))
                            || ((*(gActionPtr) == SET_CONFIGTIME_COMMAND)
                                    && (btWaitingForArgs == 1))
                            || ((*(gActionPtr) == SET_EXPID_COMMAND)
                                    && (btWaitingForArgs == 1))
                            || ((*(gActionPtr) == SET_SHIMMERNAME_COMMAND)
                                    && (btWaitingForArgs == 1))))
#else
                        ((btAction == SET_DAUGHTER_CARD_MEM_COMMAND)
                            && (btWaitingForArgs == 1))
                            || ((btAction == SET_CENTER_COMMAND)
                                    && (btWaitingForArgs == 1))
                            || ((btAction == SET_CONFIGTIME_COMMAND)
                                    && (btWaitingForArgs == 1))
                            || ((btAction == SET_EXPID_COMMAND)
                                    && (btWaitingForArgs == 1))
                            || ((btAction == SET_SHIMMERNAME_COMMAND)
                                    && (btWaitingForArgs == 1))))
#endif
                {
#if defined(SHIMMER3)
                    gArgsPtr[0] = btRxBuffPtr[0];
                    if (gArgsPtr[0])
#else
                    btArgs[0] = btRxBuffPtr[0];
                    if (btArgs[0])
#endif
                    {
#if defined(SHIMMER3)
                      btWaitingForArgsLength = gArgsPtr[0];
#else
                      btWaitingForArgsLength = btArgs[0];
#endif
                        setDmaWaitingForResponse(btWaitingForArgsLength);
                        return 0;
                    }
                }
#if defined(SHIMMER3)
                else if (*(gActionPtr) == RN4678_STATUS_STRING_SEPARATOR)
                {
                    uint8_t numberOfCharRemaining = 0U;
                    uint8_t bringUcOutOfSleep = 0U;

                    memcpy(btStatusStr+btStatusStrIndex, btRxBuffPtr, btWaitingForArgs);
                    memset(btRxBuffPtr, 0, btWaitingForArgs);
                    btStatusStrIndex += btWaitingForArgs;

                    enum BT_FIRMWARE_VERSION btFwVer = getBtFwVersion();

                    uint8_t firstChar = btStatusStr[1U];
                    switch (firstChar)
                    {
                    case 'A':
                        /* "%AUTHENTICATED%" */
                        if (btStatusStr[14U]=='%')
                        {
                            /* TODO */
                        }
                        /* "%AUTHEN" - Read outstanding bytes */
                        else if (btStatusStr[6U]=='N')
                        {
                            numberOfCharRemaining = BT_STAT_STR_LEN_AUTHENTICATED - BT_STAT_STR_LEN_SMALLEST;
                        }
                        /* "%AUTH_FAIL%" */
                        else if (btStatusStr[10U]=='%')
                        {
                            /* TODO */
                        }
                        /* "%AUTH_FA" - Read outstanding bytes */
                        else if (btStatusStr[6U]=='F')
                        {
                            numberOfCharRemaining = BT_STAT_STR_LEN_AUTH_FAIL - BT_STAT_STR_LEN_SMALLEST;
                        }
                        break;
                    case 'B':
                        /* "%BONDED%" */
                        if (btStatusStr[7U]=='%')
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
                        if (btStatusStr[5U]=='E')
                        {
                            /* "%CONNECT,001BDC06A3D5%" - RN4678 */
                            if (btStatusStr[21U]=='%')
                            {
                                setRn4678ConnectionState(RN4678_CONNECTED_CLASSIC);
                            }
                            /* "%CONNECT" for RN42 v4.77 or "%CONNECT,001BDC06A3D5," - RN42 v6.15 */
                            else if ((btFwVer == RN41_V4_77 && btStatusStr[7U]=='T')
                                    || (btFwVer == RN42_V4_77 && btStatusStr[7U]=='T')
                                    || (btFwVer == RN42_V6_15 && btStatusStr[21U]==','))
                            {
                                triggerBtRfCommStateChangeCallback(TRUE);
                                bringUcOutOfSleep = 1U;
                            }
                            else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
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
                        else if (btStatusStr[5U]=='_')
                        {
                            /* "%CONN_PARAM,000C,0000,03C0%" - RN4678 */
                            if (btStatusStr[26U]=='%')
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
                        if (btStatusStr[8U]=='%')
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
                        else if (btStatusStr[10U]=='T')
                        {
                            triggerBtRfCommStateChangeCallback(FALSE);
                            bringUcOutOfSleep = 1U;
                        }
                        /* "%DISCON" - Read outstanding bytes */
                        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
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
                        if (btStatusStr[2U]=='N')
                        {
                            if (btStatusStr[5U]=='I')
                            {
                                /* "%END_INQ%" */
                                if (btStatusStr[8U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%END_IN" - Read outstanding bytes */
                                else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_END_INQ - BT_STAT_STR_LEN_SMALLEST;
                                }
                            }
                            else if (btStatusStr[5U]=='S')
                            {
                                /* "%END_SCN%" */
                                if (btStatusStr[8U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%END_SC" - Read outstanding bytes */
                                else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_END_SCN - BT_STAT_STR_LEN_SMALLEST;
                                }
                            }
                        }
                        else if (btStatusStr[2U]=='R')
                        {
                            if (btStatusStr[5U]=='C')
                            {
                                /* %ERR_CON is common to two status strings. If detected, read two more chars to determine which one it is */
                                /* "%ERR_CONN%" */
                                if (btStatusStr[9U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%ERR_CONN_PARAM%" */
                                else if (btStatusStr[15U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%ERR_CONN_" - Read outstanding bytes */
                                else if (btStatusStr[9U]=='_')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_ERR_CONN_PARAM - BT_STAT_STR_LEN_ERR_CONN;
                                }
                                /* "%ERR_CON" - Read outstanding bytes */
                                else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_ERR_CONN - BT_STAT_STR_LEN_SMALLEST;
                                }
                            }
                            else if (btStatusStr[5U]=='L')
                            {
                                /* "%ERR_LSEC%" */
                                if (btStatusStr[9U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%ERR_LSE" - Read outstanding bytes */
                                else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_ERR_LSEC - BT_STAT_STR_LEN_SMALLEST;
                                }
                            }
                            else if (btStatusStr[5U]=='S')
                            {
                                /* "%ERR_SEC%" */
                                if (btStatusStr[8U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%ERR_SE" - Read outstanding bytes */
                                else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_ERR_SEC - BT_STAT_STR_LEN_SMALLEST;
                                }
                            }
                        }
                        break;
                    case 'F':
                        /* "%FACTORY_RESET%" */
                        if (btStatusStr[14U]=='%')
                        {
                            /* TODO */
                        }
                        /* "%FACTOR" - Read outstanding bytes */
                        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                        {
                            numberOfCharRemaining = BT_STAT_STR_LEN_FACTORY_RESET - BT_STAT_STR_LEN_SMALLEST;
                        }
                        break;
                    case 'L':
                        if (btStatusStr[2U]=='C')
                        {
                            /* "%LCONNECT,001BDC06A3D5,1%" - RN4678 BLE mode */
                            if (btStatusStr[24U]=='%')
                            {
                                setRn4678ConnectionState(RN4678_CONNECTED_BLE);

                                /* RN4678 seems to assume charactertic is advice once BLE connected */
                                triggerBtRfCommStateChangeCallback(TRUE);

                                bringUcOutOfSleep = 1U;
                            }
                            else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                            {
                                numberOfCharRemaining = BT_STAT_STR_LEN_RN4678_LCONNECT - BT_STAT_STR_LEN_SMALLEST;
                            }
                            break;
                        }
                        else if (btStatusStr[2U]=='B')
                        {
                            /* "%LBONDED%" */
                            if (btStatusStr[8U]=='%')
                            {
                                /* TODO */
                            }
                            /* "%LBONDE" - Read outstanding bytes */
                            else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                            {
                                numberOfCharRemaining = BT_STAT_STR_LEN_LBONDED - BT_STAT_STR_LEN_SMALLEST;
                            }
                        }
                        else if (btStatusStr[2U]=='S')
                        {
                            if (btStatusStr[6U]=='R')
                            {
                                /* "%LSECURED%" */
                                if (btStatusStr[9U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%LSECURE_FAIL%" */
                                else if (btStatusStr[13U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%LSECURE_F" */
                                else if (btStatusStr[9U]=='F')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_LSECURE_FAIL - BT_STAT_STR_LEN_LSECURED;
                                }
                                /* "%LSECUR" - Read outstanding bytes */
                                else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_LSECURED - BT_STAT_STR_LEN_SMALLEST;
                                }
                            }
                            else if (btStatusStr[6U]=='A')
                            {
                                /* "%LSTREAM_OPEN%" */
                                if (btStatusStr[13U]=='%')
                                {
                                    /* TODO */
                                }
                                /* "%LSTREA" - Read outstanding bytes */
                                else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_LSTREAM_OPEN - BT_STAT_STR_LEN_SMALLEST;
                                }
                            }
                        }
                        break;
                    case 'M':
                        /* "%MLDP_MODE%" */
                        if (btStatusStr[10U]=='%')
                        {
                            /* TODO */
                        }
                        /* "%MLDP_M" - Read outstanding bytes */
                        else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                        {
                            numberOfCharRemaining = BT_STAT_STR_LEN_MLDP_MODE - BT_STAT_STR_LEN_SMALLEST;
                        }
                        break;
                    case 'R':
                        if (btStatusStr[2U]=='E')
                        {
                            /* "%REBOOT%" -> RN4678 */
                            if (btStatusStr[7U]=='%')
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
                        else if (btStatusStr[2U]=='F')
                        {
                            if (btStatusStr[8U]=='C')
                            {
                                /* "%RFCOMM_CLOSE%" */
                                if (btStatusStr[13U]=='%')
                                {
                                    triggerBtRfCommStateChangeCallback(FALSE);
                                    bringUcOutOfSleep = 1U;
                                }
                                /* "%RFCOMM_CLOSE" - Read outstanding bytes */
                                else if (btStatusStr[13U]=='\0')
                                {
                                    numberOfCharRemaining = BT_STAT_STR_LEN_RFCOMM_CLOSE - BT_STAT_STR_LEN_RFCOMM_OPEN;
                                }
                            }
                            /* "%RFCOMM_OPEN%" */
                            else if (btStatusStr[12U]=='%')
                            {
                                triggerBtRfCommStateChangeCallback(TRUE);
                                bringUcOutOfSleep = 1U;
                            }
                            /* "%RFCOMM" - Read outstanding bytes */
                            else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                            {
                                numberOfCharRemaining = BT_STAT_STR_LEN_RFCOMM_OPEN - BT_STAT_STR_LEN_SMALLEST;
                            }
                        }
                        break;
                    case 'S':
                        if (btStatusStr[3U]=='C')
                        {
                            /* "%SECURED%" */
                            if (btStatusStr[8U]=='%')
                            {
                                /* TODO */
                            }
                            /* "%SECURE_FAIL%" */
                            else if (btStatusStr[12U]=='%')
                            {
                                /* TODO */
                            }
                            /* "%SECURE_F" - Read outstanding bytes */
                            else if (btStatusStr[7U]=='_')
                            {
                                numberOfCharRemaining = BT_STAT_STR_LEN_SECURE_FAIL - BT_STAT_STR_LEN_SECURED;
                            }
                            /* "%SECURE" - Read outstanding bytes */
                            else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
                            {
                                numberOfCharRemaining = BT_STAT_STR_LEN_SECURED - BT_STAT_STR_LEN_SMALLEST;
                            }
                        }
                        else if (btStatusStr[3U]=='S')
                        {
                            /* "%SESSION_OPEN%" */
                            if (btStatusStr[13U]=='%')
                            {
                                /* TODO */
                            }
                            /* "%SESSION_CLOSE%" */
                            else if (btStatusStr[14U]=='%')
                            {
                                /* TODO */
                            }
                            /* "%SESSION_CLOSE" - Read outstanding bytes */
                            else if (btStatusStr[13U]=='E')
                            {
                                numberOfCharRemaining = BT_STAT_STR_LEN_SESSION_CLOSE - BT_STAT_STR_LEN_SESSION_OPEN;
                            }
                            /* "%SESSIO" - Read outstanding bytes */
                            else if (btStatusStr[BT_STAT_STR_LEN_SMALLEST]=='\0')
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
                        btWaitingForArgs = numberOfCharRemaining;
                    }
                    else
                    {
                        btWaitingForArgs = 0;
                        numberOfCharRemaining = 1U;
                    }
                    setDmaWaitingForResponse(numberOfCharRemaining);
                    return bringUcOutOfSleep;
                }
#endif
#if defined(SHIMMER3)
                else if (*(gActionPtr) == ACK_COMMAND_PROCESSED)
#else
                else if (btAction == ACK_COMMAND_PROCESSED)
#endif
                {
#if USE_OLD_SD_SYNC_APPROACH
                    /* Store local time as early as possible after sync bytes have been received */
                    saveLocalTime();
#else
                    /* If waiting for command byte */
                    if(!btWaitingForArgsLength)
                    {
                        /* Save command byte */
#if defined(SHIMMER3)
                        gArgsPtr[0] = btRxBuffPtr[0];
#else
                        btArgs[0] = btRxBuffPtr[0];
#endif

                        if (btRxBuffPtr[0] == SD_SYNC_RESPONSE)
                        {
                            /* Wait for flag to be received */
                            btWaitingForArgsLength = 1U;
                            setDmaWaitingForResponse(btWaitingForArgsLength);
                            return 0;
                        }
                    }
#endif
                }

                if (btWaitingForArgsLength)
                {
#if defined(SHIMMER3)
                    memcpy(gArgsPtr + btWaitingForArgs, btRxBuffPtr, btWaitingForArgsLength);
#else
                    memcpy(&btArgs[btWaitingForArgs], btRxBuffPtr, btWaitingForArgsLength);
#endif
                }
                else
                {
#if defined(SHIMMER3)
                    memcpy(gArgsPtr, btRxBuffPtr, btWaitingForArgs);
#else
                    memcpy(&btArgs[0], btRxBuffPtr, btWaitingForArgs);
#endif
                }

                btWaitingForArgsLength = 0;
                btWaitingForArgs = 0;
                btArgsSize = 0;
                setDmaWaitingForResponse(1U);
                if(newBtCmdToProcess_cb)
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
                case GET_A_ACCEL_CALIBRATION_COMMAND:
                case GET_MPU9150_GYRO_CALIBRATION_COMMAND:
                case GET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
                case GET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
                case GET_GSR_RANGE_COMMAND:
                case GET_ALL_CALIBRATION_COMMAND:
                case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
                case GET_DEVICE_VERSION_COMMAND:
                case GET_FW_VERSION_COMMAND:
                case GET_CHARGE_STATUS_LED_COMMAND:
                case GET_BUFFER_SIZE_COMMAND:
                case GET_UNIQUE_SERIAL_COMMAND:
                case GET_LSM303DLHC_ACCEL_RANGE_COMMAND:
                case GET_LSM303DLHC_MAG_GAIN_COMMAND:
                case GET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
                case GET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
                case GET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
                case GET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
                case GET_MPU9150_GYRO_RANGE_COMMAND:
                case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
                case GET_BMP280_CALIBRATION_COEFFICIENTS_COMMAND:
                case GET_MPU9150_SAMPLING_RATE_COMMAND:
                case GET_MPU9150_ACCEL_RANGE_COMMAND:
                case GET_BMPX80_PRES_OVERSAMPLING_RATIO_COMMAND:
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
//                case UPD_CALIB_DUMP_COMMAND:
                case UPD_FLASH_COMMAND:
                case GET_BT_VERSION_STR_COMMAND:
#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    if(newBtCmdToProcess_cb)
                    {
                        newBtCmdToProcess_cb();
                    }
                    setDmaWaitingForResponse(1U);
                    /* Wake-up MCU so that the get command can be processed */
                    wakeupMcu = 1U;
                    break;
                case SET_LSM303DLHC_ACCEL_RANGE_COMMAND:
                case SET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
                case SET_LSM303DLHC_MAG_GAIN_COMMAND:
                case SET_CHARGE_STATUS_LED_COMMAND:
                case SET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
                case SET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
                case SET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
                case SET_MPU9150_GYRO_RANGE_COMMAND:
                case SET_MPU9150_SAMPLING_RATE_COMMAND:
                case SET_MPU9150_ACCEL_RANGE_COMMAND:
                case SET_BMPX80_PRES_OVERSAMPLING_RATIO_COMMAND:
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
#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    btWaitingForArgs = 1U;
                    break;
                case SET_SAMPLING_RATE_COMMAND:
                case GET_DAUGHTER_CARD_ID_COMMAND:
                    //                case SET_DAUGHTER_CARD_ID_COMMAND:
#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    btWaitingForArgs = 2U;
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
#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    btWaitingForArgs = 3U;
                    break;
                case SET_CONFIG_SETUP_BYTES_COMMAND:
#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    btWaitingForArgs = 4U;
                    break;
                case SET_RWC_COMMAND:
                case SET_DERIVED_CHANNEL_BYTES:
#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    btWaitingForArgs = 8U;
                    break;
                case SET_A_ACCEL_CALIBRATION_COMMAND:
                case SET_MPU9150_GYRO_CALIBRATION_COMMAND:
                case SET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
                case SET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    btWaitingForArgs = 21U;
                    break;
#endif
#if defined(SHIMMER3)
                case RN4678_STATUS_STRING_SEPARATOR:
                    memset(btStatusStr, 0, sizeof(btStatusStr));
                    btStatusStr[0U] = RN4678_STATUS_STRING_SEPARATOR;
                    btStatusStrIndex = 1U;
                    *(gActionPtr) = data;
                    /* Minus 1 because we've already received 1 x RN4678_STATUS_STRING_SEPARATOR */
                    btWaitingForArgs = BT_STAT_STR_LEN_SMALLEST - 1U;
                    break;
#endif
#if USE_OLD_SD_SYNC_APPROACH
                case ACK_COMMAND_PROCESSED:
                    /* SD Sync Node - gets here when a node receives a sync packet from a center */
                    *(gActionPtr) = data;
                    btWaitingForArgs = SYNC_PACKET_PAYLOAD_SIZE;
                    break;
#else
                case ACK_COMMAND_PROCESSED:
                    /* Wait for command byte */
#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    btWaitingForArgs = 1U;
                    break;
                case SET_SD_SYNC_COMMAND:
                    /* Store local time as early as possible after sync bytes have been received */
                    saveLocalTime();

#if defined(SHIMMER3)
                    *(gActionPtr) = data;
#else
                    btAction = data;
#endif
                    btWaitingForArgs = SYNC_PACKET_PAYLOAD_SIZE + BT_SD_SYNC_CRC_MODE;
                    break;
#endif
                default:
                    setDmaWaitingForResponse(1U);
                    break;
                }

                if (btWaitingForArgs)
                {
                    setDmaWaitingForResponse(btWaitingForArgs);
                }

                return wakeupMcu;
            }
#if defined(SHIMMER3)
        }
    }
    else
    {
        uint8_t len = strlen((char*) btRxExp);
        if (!memcmp(btRxBuffPtr, btRxExp, len))
        {
            memset(btRxBuffPtr, 0, len);
            BT_setGoodCommand();
        }
        else
        {
            _NOP(); // bad command trap: reaching here = serious BT problem
        }
    }

    setDmaWaitingForResponseIfStatusStrEnabled();
#endif
    return 0;
}

void resetBtRxVariablesOnConnect(void)
{
#if defined(SHIMMER3)
    /* Reset to unsupported command */
    *(gActionPtr) = ACK_COMMAND_PROCESSED-1U;
#else
    btAction = ACK_COMMAND_PROCESSED-1U;
#endif
    btWaitingForArgs = 0;
    btWaitingForArgsLength = 0;
}

void resetBtRxBuff(void)
{
    memset(btRxBuffPtr, 0, sizeof(btRxBuffPtr));
}

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
            if(wasStartBtCmdModeSentAndReponseReceived())
            {
                processStartRnCmdResponse();
                responseParsed = 1U;
            }
            else if(isRnCommandModeActive() && (indexOfFirstEol = isNewLineDetectedInBtRxBuf())>0)
            {
                /* isRn4678CmdDetectedOnBoot and isBtDeviceUnknown() are needed
                 * here because the first command that is sent is to get the
                 * version number and so until the response is parsed, BtDevice
                 * has not been set yet */
                if(((isBtDeviceRn4678() || isRn4678CmdDetectedOnBoot) && isFullRN4678CmdResponseReceived())
                        || isBtDeviceRn41orRN42()
                        || isBtDeviceUnknown())
                {
                    responseParsed = processRnCmdResponse();
                }
            }
            /* Status responses begin with '%' */
            else if(firstByteInRxBuf==RN4678_STATUS_STRING_SEPARATOR
                    && numBytesInBtRxBufWhenLastProcessed>=BT_STAT_STR_LEN_SMALLEST)
            {
                responseParsed = processStatusString();
            }
            /* Shimmer command */
            else if(!areBtSetupCommandsRunning() && isShimmerBtCmd(firstByteInRxBuf))
            {
                responseParsed = processShimmerBtCmd();
            }

            /* Update reference time so that timeout can be detected */
            /* Don't enable timeout until BT is being initialised */
            if(isBtStarting() || responseParsed || getNumBytesInBtRxBuf()==0)
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
        // Still unable to parse puffer, clear the first byte
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
//    clearBytesFromBtRxBuf(RN4X_CMD_LEN);

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
    return (commandBufPtr[0]=='$'
            && commandBufPtr[1U]=='$'
            && commandBufPtr[2U]=='$'
            && numBytesInBtRxBufWhenLastProcessed>=RN4X_CMD_LEN
            && getRxByteAtIndex(0)=='C'
            && getRxByteAtIndex(1U)=='M'
            && getRxByteAtIndex(2U)=='D');
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
    for(;numBytes>0;numBytes--)
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
                && getRxByteAtIndex(14U)=='%')
        {
            numberOfCharToRemove = BT_STAT_STR_LEN_AUTHENTICATED;
            /* TODO */
        }
        /* "%AUTH_FAIL%" */
        else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_AUTH_FAIL
                && getRxByteAtIndex(10U)=='%')
        {
            numberOfCharToRemove = BT_STAT_STR_LEN_AUTH_FAIL;
            /* TODO */
        }
        break;
    case 'B':
        /* "%BONDED%" */
        if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_BONDED
                && getRxByteAtIndex(7U)=='%')
        {
            numberOfCharToRemove = BT_STAT_STR_LEN_BONDED;
            /* TODO */
        }
        break;
    case 'C':
        /* "%CONNECT,001BDC06A3D5%" - RN4678 */
        if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN4678_CONNECT
                && getRxByteAtIndex(21U)=='%')
        {
            setRn4678ConnectionState(RN4678_CONNECTED_CLASSIC);
            numberOfCharToRemove = BT_STAT_STR_LEN_RN4678_CONNECT;
        }
        /* "%CONNECT" for RN42 v4.77 or "%CONNECT,001BDC06A3D5," - RN42 v6.15 */
        else if (( (btFwVer == RN41_V4_77 || btFwVer == RN42_V4_77)
                && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN42_v477_CONNECT
                && getRxByteAtIndex(7U)=='T')
            || (btFwVer == RN42_V6_15
                    && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN42_v615_CONNECT
                    && getRxByteAtIndex(21U)==','))
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
                && getRxByteAtIndex(8U)=='%')
        {
            setRn4678ConnectionState(RN4678_DISCONNECTED);
            numberOfCharToRemove = BT_STAT_STR_LEN_RN4678_DISCONN;
        }
        /* "%DISCONNECT" -> RN42 */
        else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN42_DISCONNECT
                && getRxByteAtIndex(10U)=='T')
        {
            triggerBtRfCommStateChangeCallback(0);

            numberOfCharToRemove = BT_STAT_STR_LEN_RN42_DISCONNECT;
        }
        break;
    case 'E':
        if (getRxByteAtIndex(2U)=='N')
        {
            /* "%END_INQ%" */
            if (getRxByteAtIndex(5U)=='I'
                    && getRxByteAtIndex(8U)=='%'
                    && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_END_INQ)
            {
                numberOfCharToRemove = BT_STAT_STR_LEN_END_INQ;
                /* TODO */
            }
            /* "%END_SCN%" */
            else if (getRxByteAtIndex(5U)=='S'
                    && getRxByteAtIndex(8U)=='%'
                    && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_END_SCN)
            {
                numberOfCharToRemove = BT_STAT_STR_LEN_END_SCN;
                /* TODO */
            }
        }
        else if (getRxByteAtIndex(2U)=='R')
        {
            if (getRxByteAtIndex(5U)=='C')
            {
                /* %ERR_CON is common to two status strings. If detected, read two more chars to determine which one it is */
                /* "%ERR_CONN%" */
                if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_ERR_CONN
                        && getRxByteAtIndex(9U)=='%')
                {
                    numberOfCharToRemove = BT_STAT_STR_LEN_ERR_CONN;
                    /* TODO */
                }
                /* "%ERR_CONN_PARAM%" */
                else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_ERR_CONN_PARAM
                        && getRxByteAtIndex(15U)=='%')
                {
                    numberOfCharToRemove = BT_STAT_STR_LEN_ERR_CONN_PARAM;
                    /* TODO */
                }
            }
            else if (getRxByteAtIndex(5U)=='L')
            {
                /* "%ERR_LSEC%" */
                if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_ERR_LSEC
                        && getRxByteAtIndex(9U)=='%')
                {
                    numberOfCharToRemove = BT_STAT_STR_LEN_ERR_LSEC;
                    /* TODO */
                }
            }
            else if (getRxByteAtIndex(5U)=='S')
            {
                /* "%ERR_SEC%" */
                if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_ERR_SEC
                        && getRxByteAtIndex(8U)=='%')
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
                && getRxByteAtIndex(14U)=='%')
        {
            numberOfCharToRemove = BT_STAT_STR_LEN_FACTORY_RESET;
            /* TODO */
        }
        break;
    case 'L':
        if (getRxByteAtIndex(2U)=='B')
        {
            /* "%LBONDED%" */
            if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_LBONDED
                    && getRxByteAtIndex(8U)=='%')
            {
                numberOfCharToRemove = BT_STAT_STR_LEN_LBONDED;
                /* TODO */
            }
        }
        else if (getRxByteAtIndex(2U)=='S')
        {
            if (getRxByteAtIndex(6U)=='R')
            {
                /* "%LSECURED%" */
                if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_LSECURED
                        && getRxByteAtIndex(9U)=='%')
                {
                    numberOfCharToRemove = BT_STAT_STR_LEN_LSECURED;
                    /* TODO */
                }
                /* "%LSECURE_FAIL%" */
                else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_LSECURE_FAIL
                        && getRxByteAtIndex(13U)=='%')
                {
                    numberOfCharToRemove = BT_STAT_STR_LEN_LSECURE_FAIL;
                    /* TODO */
                }
            }
            else if (getRxByteAtIndex(6U)=='A')
            {
                /* "%LSTREAM_OPEN%" */
                if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_LSTREAM_OPEN
                        && getRxByteAtIndex(13U)=='%')
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
                && getRxByteAtIndex(10U)=='%')
        {
            numberOfCharToRemove = BT_STAT_STR_LEN_MLDP_MODE;
            /* TODO */
        }
        break;
    case 'R':
        if (getRxByteAtIndex(2U)=='E')
        {
            /* "%REBOOT%" -> RN4678 */
            if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN4678_REBOOT
                    && getRxByteAtIndex(7U)=='%')
            {
                numberOfCharToRemove = BT_STAT_STR_LEN_RN4678_REBOOT;
            }
            else if (isBtDeviceRn41orRN42()
                    && numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RN42_REBOOT
                    && getRxByteAtIndex(6U)=='T')
            {
                numberOfCharToRemove = BT_STAT_STR_LEN_RN42_REBOOT;
            }

            if(BT_getWaitForInitialBoot() && numberOfCharToRemove)
            {
                BT_setWaitForInitialBoot(0);
                triggerBtSetGoodCommand = 1U;
            }
        }
        else if (getRxByteAtIndex(2U)=='F')
        {
            if (getRxByteAtIndex(8U)=='C')
            {
                /* "%RFCOMM_CLOSE%" */
                if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RFCOMM_CLOSE
                        && getRxByteAtIndex(13U)=='%')
                {
                    triggerBtRfCommStateChangeCallback(0);
                    numberOfCharToRemove = BT_STAT_STR_LEN_RFCOMM_CLOSE;
                }
            }
            /* "%RFCOMM_OPEN%" */
            else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_RFCOMM_OPEN
                    && getRxByteAtIndex(12U)=='%')
            {
                triggerBtRfCommStateChangeCallback(1U);
                numberOfCharToRemove = BT_STAT_STR_LEN_RFCOMM_OPEN;
            }
        }
        break;
    case 'S':
        if (getRxByteAtIndex(3U)=='C')
        {
            /* "%SECURED%" */
            if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SECURED
                    && getRxByteAtIndex(8U)=='%')
            {
                numberOfCharToRemove = BT_STAT_STR_LEN_SECURED;
                /* TODO */
            }
            /* "%SECURE_FAIL%" */
            else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SECURE_FAIL
                    && getRxByteAtIndex(12U)=='%')
            {
                numberOfCharToRemove = BT_STAT_STR_LEN_SECURE_FAIL;
                /* TODO */
            }
        }
        else if (getRxByteAtIndex(3U)=='S')
        {
            /* "%SESSION_OPEN%" */
            if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SESSION_OPEN
                    && getRxByteAtIndex(13U)=='%')
            {
                numberOfCharToRemove = BT_STAT_STR_LEN_SESSION_OPEN;
                /* TODO */
            }
            /* "%SESSION_CLOSE%" */
            else if (numBytesInBtRxBufWhenLastProcessed >= BT_STAT_STR_LEN_SESSION_CLOSE
                    && getRxByteAtIndex(14U)=='%')
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
    if(triggerBtSetGoodCommand)
    {
        BT_setGoodCommand();
    }

    return numberOfCharToRemove>0;
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
        if (getRxByteAtIndex(bufIndex) == statStrCheck[i]
                   || statStrCheck[i]=='X')
        {
            count++;
            bufIndex++;
        }
    }

    if(((statStrCheckLen - count) <= numCharTolerance))
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
    for (i = 0; i < (indexOfFirstEol+2); i++)
    {
        readByteFromBtRxBuf(&unwrappedResponse[i]);

        if(i>=2
                && unwrappedResponse[i-1]=='\r'
                && unwrappedResponse[i]=='\n')
        {
            break;
        }
    }

    uint16_t unwrappedResponseLen = strlen(unwrappedResponse);
    uint8_t cmdExpectedAfterEol = (isBtDeviceRn4678() || isRn4678CmdDetectedOnBoot);

    /* Get BT module hardware and firmware version */
    if(commandBufPtr[0]=='V')
    {
        enum BT_FIRMWARE_VERSION btFwVerNew = BT_FW_VER_UNKNOWN;

        /* RN41 or RN42 */
        if (unwrappedResponse[0U]=='V')
        {
            /* RN41_VERSION_RESPONSE_V4_77 or RN42_VERSION_RESPONSE_V4_77 */
            if (unwrappedResponse[4U]=='4' && unwrappedResponse[5U]=='.')
            {
                if (unwrappedResponse[9U]=='R' && unwrappedResponse[10U]=='N')
                {
                    btFwVerNew = RN42_V4_77;
                }
                else
                {
                    btFwVerNew = RN41_V4_77;
                }
            }
            /* RN42_VERSION_RESPONSE_V6_15 */
            else if (unwrappedResponse[4U]=='6' && unwrappedResponse[5U]=='.' && unwrappedResponse[6U]=='1' && unwrappedResponse[7U]=='5')
            {
                btFwVerNew = RN42_V6_15;
            }
            /* V6.30 not supported */
            else if (unwrappedResponse[4U]=='6' && unwrappedResponse[5U]=='.' && unwrappedResponse[6U]=='3' && unwrappedResponse[7U]=='0')
            {
                btFwVerNew = RN42_V6_30;
            }
        }
        /* RN4678 */
        else if (unwrappedResponse[0U]=='R')
        {
            /* RN4678_VERSION_RESPONSE_V1_00_5 */
            if (unwrappedResponse[10U]=='0' && unwrappedResponse[11U]=='0')
            {
                btFwVerNew = RN4678_V1_00_5;
            }
            /* RN4678_VERSION_RESPONSE_V1_11_0 */
            else if (unwrappedResponse[10U]=='1' && unwrappedResponse[11U]=='1')
            {
                btFwVerNew = RN4678_V1_11_0;
            }
            /* RN4678_VERSION_RESPONSE_V1_13_5 */
            else if (unwrappedResponse[10U]=='1' && unwrappedResponse[11U]=='3')
            {
                btFwVerNew = RN4678_V1_13_5;
            }
            /* RN4678_VERSION_RESPONSE_V1_22_0 */
            else if (unwrappedResponse[10U]=='2' && unwrappedResponse[11U]=='2')
            {
                btFwVerNew = RN4678_V1_22_0;
            }
            /* RN4678_VERSION_RESPONSE_V1_23_0 */
            else if (unwrappedResponse[10U]=='2' && unwrappedResponse[11U]=='3')
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
    else if(commandBufPtr[0]=='G')
    {
        /* Get MAC address */
        if(commandBufPtr[1]=='B')
        {
            setMacId_cb(unwrappedResponse);
            responseParsed = 1;
        }
        else
        {
            responseParsed = parseRnGetResponse(commandBufPtr[1], &unwrappedResponse[0]);
        }
    }
//    else if(unwrappedResponse[0]=='C'
//            && unwrappedResponse[1]=='M'
//            && unwrappedResponse[2]=='D')
//    {
//        setCommandModeActive(1U);
//        responseParsed = 1;
//        cmdExpectedAfterEol = 0;
//    }
    else if(unwrappedResponse[0]=='A'
            && unwrappedResponse[1]=='O'
            && unwrappedResponse[2]=='K')
    {
        responseParsed = 1;
    }
    else if(unwrappedResponse[0]=='E'
            && unwrappedResponse[1]=='N'
            && unwrappedResponse[2]=='D')
    {
        setCommandModeActive(0U);
        responseParsed = 1;
        cmdExpectedAfterEol = 0;
    }
    else if(unwrappedResponse[0]=='E'
            && unwrappedResponse[1]=='R'
            && unwrappedResponse[2]=='R')
    {
        triggerShimmerErrorState();
    }
    else if(unwrappedResponse[0]=='?')
    {
        triggerShimmerErrorState();
    }

    else if(expectedResponsePtr[0]!='\0'
            && !memcmp(unwrappedResponse, expectedResponsePtr, strlen(expectedResponsePtr)))
    {
        responseParsed = 1;
    }

    memset(unwrappedResponse, 0, strlen(unwrappedResponse));

    if(responseParsed)
    {
        if(cmdExpectedAfterEol)
        {
            clearBytesFromBtRxBuf(RN4X_CMD_LEN);
        }

        clearBtCmdTxRxBuffsAndProceed();
    }

    return responseParsed;
}

uint8_t isFullRN4678CmdResponseReceived(void)
{
    int16_t numBytesAfterEol = numBytesInBtRxBufWhenLastProcessed-(indexOfFirstEol+2U);

    /* For the RN4678, all responses have "CMD> " as a suffix except for "Trying\r\n", "END\r\n".
     * There is an option to turn this off in the RN4678 but we are leaving it on at the moment */
    if((getRxByteAtIndex(0)=='T'
                    && getRxByteAtIndex(1U)=='r'
                    && getRxByteAtIndex(2U)=='y')
            || (getRxByteAtIndex(0)=='E'
                    && getRxByteAtIndex(1U)=='N'
                    && getRxByteAtIndex(2U)=='D'))
    {
        return 1U;
    }
    else
    {
        if(numBytesAfterEol!=0 && numBytesAfterEol>=RN4X_CMD_LEN)
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
    case GET_A_ACCEL_CALIBRATION_COMMAND:
    case GET_MPU9150_GYRO_CALIBRATION_COMMAND:
    case GET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
    case GET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
    case GET_GSR_RANGE_COMMAND:
    case GET_ALL_CALIBRATION_COMMAND:
    case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
    case GET_DEVICE_VERSION_COMMAND:
    case GET_FW_VERSION_COMMAND:
    case GET_CHARGE_STATUS_LED_COMMAND:
    case GET_BUFFER_SIZE_COMMAND:
    case GET_UNIQUE_SERIAL_COMMAND:
    case GET_LSM303DLHC_ACCEL_RANGE_COMMAND:
    case GET_LSM303DLHC_MAG_GAIN_COMMAND:
    case GET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
    case GET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
    case GET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
    case GET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
    case GET_MPU9150_GYRO_RANGE_COMMAND:
    case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
    case GET_BMP280_CALIBRATION_COEFFICIENTS_COMMAND:
    case GET_MPU9150_SAMPLING_RATE_COMMAND:
    case GET_MPU9150_ACCEL_RANGE_COMMAND:
    case GET_BMPX80_PRES_OVERSAMPLING_RATIO_COMMAND:
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
    case SET_LSM303DLHC_ACCEL_RANGE_COMMAND:
    case SET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
    case SET_LSM303DLHC_MAG_GAIN_COMMAND:
    case SET_CHARGE_STATUS_LED_COMMAND:
    case SET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
    case SET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
    case SET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
    case SET_MPU9150_GYRO_RANGE_COMMAND:
    case SET_MPU9150_SAMPLING_RATE_COMMAND:
    case SET_MPU9150_ACCEL_RANGE_COMMAND:
    case SET_BMPX80_PRES_OVERSAMPLING_RATIO_COMMAND:
    case SET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
    case SET_GSR_RANGE_COMMAND:
    case SET_BT_COMMS_BAUD_RATE:
    case SET_MYID_COMMAND:
    case SET_NSHIMMER_COMMAND:
    case SET_CRC_COMMAND:
    case SET_INSTREAM_RESPONSE_ACK_PREFIX_STATE
    case SET_DATA_RATE_TEST:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+1U))
        {
            readActionAndArgBytes(1U);
            responseParsed = 1U;
        }
        break;

    /* 1 command byte, 2 argument bytes */
    case SET_SAMPLING_RATE_COMMAND:
    case GET_DAUGHTER_CARD_ID_COMMAND:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+2U))
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
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+3U))
        {
            readActionAndArgBytes(3U);
            responseParsed = 1U;
        }
        break;

    /* 1 command byte, 4 argument bytes */
    case SET_CONFIG_SETUP_BYTES_COMMAND:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+4U))
        {
            readActionAndArgBytes(4U);
            responseParsed = 1U;
        }
        break;

    /* 1 command byte, 8 argument bytes */
    case SET_RWC_COMMAND:
    case SET_DERIVED_CHANNEL_BYTES:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+8U))
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
    case SET_A_ACCEL_CALIBRATION_COMMAND:
    case SET_MPU9150_GYRO_CALIBRATION_COMMAND:
    case SET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
    case SET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+21U))
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
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+1U))
        {
            expectedLength = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx+1U)];
            if(numBytesInBtRxBufWhenLastProcessed>=(1U+1U+expectedLength))
            {
                readActionAndArgBytes(1U+expectedLength);
                responseParsed = 1U;
            }
        }
        break;

    /* 1 command byte, minimum 2 argument bytes - expected length is stored in arg[0] */
    case SET_DAUGHTER_CARD_ID_COMMAND:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+2U))
        {
            expectedLength = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx+1U)];
            if(numBytesInBtRxBufWhenLastProcessed>=(1U+2U+expectedLength))
            {
                readActionAndArgBytes(2U+expectedLength);
                responseParsed = 1U;
            }
        }
        break;

    /* 1 command byte, minimum 3 argument bytes - expected length is stored in arg[2] */
    case SET_EXG_REGS_COMMAND:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+3U))
        {
            expectedLength = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx+3U)];
            if(numBytesInBtRxBufWhenLastProcessed>=(1U+3U+expectedLength))
            {
                readActionAndArgBytes(3U+expectedLength);
                responseParsed = 1U;
            }
        }
        break;

    /* 1 command byte, minimum 3 argument bytes - expected length is stored in arg[0] */
    case SET_INFOMEM_COMMAND:
    case SET_CALIB_DUMP_COMMAND:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+3U))
        {
            expectedLength = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx+1U)];
            if(numBytesInBtRxBufWhenLastProcessed>=(1U+3U+expectedLength))
            {
                readActionAndArgBytes(3U+expectedLength);
                responseParsed = 1U;
            }
        }
        break;

#if !USE_OLD_SD_SYNC_APPROACH
    /* 1 command byte, minimum 1 argument byte representing command response */
    case ACK_COMMAND_PROCESSED:
        if(numBytesInBtRxBufWhenLastProcessed>=(1U+1U))
        {
            uint8_t cmdByte = gBtRxFifoPtr->data[BT_RX_BUF_MASK & (gBtRxFifoPtr->rdIdx+1U)];
            if(cmdByte==SD_SYNC_RESPONSE
                    && numBytesInBtRxBufWhenLastProcessed>=(1U+2U))
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
    if(numArgs)
    {
        uint8_t i;
        for(i=0;i<numArgs;i++)
        {
            readByteFromBtRxBuf(gArgsPtr+i);
        }
    }

    if(newBtCmdToProcess_cb)
    {
        newBtCmdToProcess_cb();
    }
}

uint8_t isNewLineDetectedInBtRxBuf(void)
{
    uint16_t i = 0;
    if(numBytesInBtRxBufWhenLastProcessed>2U){
        /* numBytesInRxBuf is -1 here as we're looking to i+1 for \n and it was triggering too early as new bytes were coming in */
        for(i=0;i<(numBytesInBtRxBufWhenLastProcessed-1);i++)
        {
            if(getRxByteAtIndex(i)=='\r'
                && getRxByteAtIndex(i+1)=='\n')
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
    case GET_A_ACCEL_CALIBRATION_COMMAND:
    case GET_MPU9150_GYRO_CALIBRATION_COMMAND:
    case GET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
    case GET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
    case GET_GSR_RANGE_COMMAND:
    case GET_ALL_CALIBRATION_COMMAND:
    case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
    case GET_DEVICE_VERSION_COMMAND:
    case GET_FW_VERSION_COMMAND:
    case GET_CHARGE_STATUS_LED_COMMAND:
    case GET_BUFFER_SIZE_COMMAND:
    case GET_UNIQUE_SERIAL_COMMAND:
    case GET_LSM303DLHC_ACCEL_RANGE_COMMAND:
    case GET_LSM303DLHC_MAG_GAIN_COMMAND:
    case GET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
    case GET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
    case GET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
    case GET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
    case GET_MPU9150_GYRO_RANGE_COMMAND:
    case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
    case GET_BMP280_CALIBRATION_COEFFICIENTS_COMMAND:
    case GET_MPU9150_SAMPLING_RATE_COMMAND:
    case GET_MPU9150_ACCEL_RANGE_COMMAND:
    case GET_BMPX80_PRES_OVERSAMPLING_RATIO_COMMAND:
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
    case SET_LSM303DLHC_ACCEL_RANGE_COMMAND:
    case SET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
    case SET_LSM303DLHC_MAG_GAIN_COMMAND:
    case SET_CHARGE_STATUS_LED_COMMAND:
    case SET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
    case SET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
    case SET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
    case SET_MPU9150_GYRO_RANGE_COMMAND:
    case SET_MPU9150_SAMPLING_RATE_COMMAND:
    case SET_MPU9150_ACCEL_RANGE_COMMAND:
    case SET_BMPX80_PRES_OVERSAMPLING_RATIO_COMMAND:
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
    case SET_SAMPLING_RATE_COMMAND:
    case GET_DAUGHTER_CARD_ID_COMMAND:
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
    case SET_A_ACCEL_CALIBRATION_COMMAND:
    case SET_MPU9150_GYRO_CALIBRATION_COMMAND:
    case SET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
    case SET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
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
                         uint8_t * actionPtr,
                         uint8_t * argsPtr)
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

    btWaitingForArgs = 0;
    btWaitingForArgsLength = 0;
    btArgsSize = 0;

#if defined(SHIMMER3)
    memset(btStatusStr, 0, sizeof(btStatusStr));

    memset(btRxBuffFullResponse, 0x00, sizeof(btRxBuffFullResponse) / sizeof(btRxBuffFullResponse[0]));
    setBtRxFullResponsePtr(btRxBuffFullResponse);
#endif

//    memset(btRxBuffPtr, 0x00, sizeof(btRxBuffPtr) / sizeof(btRxBuffPtr[0]));
//    DMA2_init((uint16_t*) &UCA1RXBUF, (uint16_t*) btRxBuff, sizeof(btRxBuff));
//    DMA2_transferDoneFunction(&Dma2ConversionDone);
//    DMA2SZ = 1U;
//    DMA2_enable();

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
  deviceVersionBtRsp = 0;
  rwcResponse = 0;
  dcIdBtRsp = 0;
  dcMemBtRsp = 0;
  infomemBtRsp = 0;

  sendAck = 0;

  i2cvBattBtRsp = 0;
  inquiryBtRsp = 0;
  samplingRateBtRsp = 0;
  toggleLedRed = 0;
  lsm303dlhcAccelRangeResponse = 0;
  lsm303dlhcMagGainResponse = 0;
  lsm303dlhcMagSamplingRateResponse = 0;
  dockStatusBtRsp = 0;
  vbattBtRsp = 0;
  trialConfigResponse = 0;
  centerResponse = 0;
  shimmerNameResponse = 0;
  expIDResponse = 0;
  configTimeResponse = 0;
  dirResponse = 0;
  nshimmerResponse = 0;
  myIDResponse = 0;
  lsm303dlhcAccelSamplingRateResponse = 0;
  lsm303dlhcAccelLPModeResponse = 0;
  lsm303dlhcAccelHRModeResponse = 0;
  mpu9250GyroRangeResponse = 0;
  bmp180CalibCoeffBtRsp = 0;
  bmp280CalibrationCoefficientsResponse = 0;
  mpu9250SamplingRateResponse = 0;
  mpu9250AccelRangeResponse = 0;
  bmp180OversamplingRatioResponse = 0;
  internalExpPowerEnableResponse = 0;
  mpu9250MagSensAdjValsResponse = 0;
  exgRegsResponse = 0;
  configSetupBytesResponse = 0;
  fwVersionBtRsp = 0;
  blinkLedBtRsp = 0;
}

uint8_t getBtVerStrLen(void)
{
    return strlen(btVerStrResponse);
}

char * getBtVerStrPtr(void)
{
    return &btVerStrResponse[0];
}

void updateBtVer(void)
{
#if defined(SHIMMER3R)
  BT_getCyw20820FirmwareVersionStr(&btVerStrResponse[0]);
#endif
}

uint8_t isWaitingForArgs(void)
{
  return btWaitingForArgs;
}

void BtUart_processCmd(void) {
   uint64_t temp64;
   uint16_t temp16;

   //HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_6);//green

   //uint32_t config_time;
   //uint8_t my_config_time[4];
   //uint8_t name_len;
   //uint8_t update_sdconfig = 0, calib_update = 0, calib_sensor = 0, calib_range = 0;
   switch (btAction) {
   case INQUIRY_COMMAND:
      inquiryBtRsp = 1;
      break;
   case DUMMY_COMMAND:
      break;
   case GET_SAMPLING_RATE_COMMAND:
      samplingRateBtRsp = 1;
      break;
   case TOGGLE_LED_COMMAND:
      toggleLedRed ^= 1;
      break;
   case START_STREAMING_COMMAND:
      stat.btstreamCmd = 1;
      S4_Task_set(TASK_STARTSENSING);
      break;
   case START_LOGGING_COMMAND:
      stat.sdlogCmd = 1;
      S4_Task_set(TASK_STARTSENSING);
      break;
   case START_SDBT_COMMAND:
      stat.btstreamCmd = 1;
      stat.sdlogCmd = 1;
      S4_Task_set(TASK_STARTSENSING);
      break;
   case SET_CRC_COMMAND:
      setBtCrcMode(btArgs[0]);
      break;
   case STOP_STREAMING_COMMAND:
      stat.btstreamCmd = 2;
      S4_Task_set(TASK_STOPSENSING);
      break;
   case STOP_LOGGING_COMMAND:
      stat.sdlogCmd = 2;
      S4_Task_set(TASK_STOPSENSING);
      break;
   case STOP_SDBT_COMMAND:
      stat.btstreamCmd = 2;
      stat.sdlogCmd = 2;
      S4_Task_set(TASK_STOPSENSING);
      break;
   case SET_SENSORS_COMMAND:
      S4Ram_storedConfigSet(btArgs, NV_SENSORS0, 3);
      S4Ram_sdHeadTextSet(btArgs, NV_SENSORS0, 3);
      //InfoMem_update();
      break;
   case GET_LSM303DLHC_ACCEL_RANGE_COMMAND:
      lsm303dlhcAccelRangeResponse = 1;
      break;
   case GET_LSM303DLHC_MAG_GAIN_COMMAND:
      lsm303dlhcMagGainResponse = 1;
      break;
   case GET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
      lsm303dlhcMagSamplingRateResponse = 1;
      break;
   case GET_STATUS_COMMAND:
      dockStatusBtRsp = 1;
      break;
#if defined(SHIMMER4_SDK)
   case GET_I2C_BATT_STATUS_COMMAND:
      i2cvBattBtRsp = 1;
      break;
   case SET_I2C_BATT_STATUS_FREQ_COMMAND:
      temp16 = btArgs[0] + ((uint16_t)btArgs[1]<<8);
      I2C_readBattSetFreq(temp16);
      break;
#endif
   case GET_VBATT_COMMAND:
      vbattBtRsp = 1;
      break;
   case GET_TRIAL_CONFIG_COMMAND:
      trialConfigResponse = 1;
      break;
   case SET_TRIAL_CONFIG_COMMAND:
      S4Ram_storedConfigSetByte(NV_SD_TRIAL_CONFIG0, btArgs[0]);
      S4Ram_storedConfigSetByte(NV_SD_TRIAL_CONFIG1, 0);
      S4Ram_storedConfigSetByte(NV_SD_BT_INTERVAL, btArgs[2]);
      S4Ram_sdHeadTextSetByte(SDH_TRIAL_CONFIG0, btArgs[0]);
      S4Ram_sdHeadTextSetByte(SDH_TRIAL_CONFIG1, 0);
      S4Ram_sdHeadTextSetByte(SDH_BROADCAST_INTERVAL, btArgs[2]);
      InfoMem_update();
      break;
   /*case GET_CENTER_COMMAND:
    centerResponse = 1;
    break;
   case SET_CENTER_COMMAND:
    break;
   case GET_SHIMMERNAME_COMMAND:
    shimmerNameResponse = 1;
    break;
   case SET_SHIMMERNAME_COMMAND:
    name_len = btArgs[0]<(MAX_CHARS-1)?args[0]:(MAX_CHARS-1);
    memset((uint8_t*)(storedConfig+NV_SD_SHIMMER_NAME), 0, MAX_CHARS-1);
    memcpy((uint8_t*)(storedConfig+NV_SD_SHIMMER_NAME), &args[1], name_len);
    InfoMem_write((uint8_t*)NV_SD_SHIMMER_NAME, storedConfig+NV_SD_SHIMMER_NAME, MAX_CHARS-1);
    SetShimmerName();
    update_sdconfig = 1;
    break;
   case GET_EXPID_COMMAND:
    expIDResponse = 1;
    break;
   case SET_EXPID_COMMAND:
    name_len = args[0]<(MAX_CHARS-1)?args[0]:(MAX_CHARS-1);
    memset((uint8_t*)(storedConfig+NV_SD_EXP_ID_NAME), 0, MAX_CHARS-1);
    memcpy((uint8_t*)(storedConfig+NV_SD_EXP_ID_NAME), &args[1], name_len);
    InfoMem_write((uint8_t*)NV_SD_EXP_ID_NAME, storedConfig+NV_SD_EXP_ID_NAME, MAX_CHARS-1);
    SetExpIdName();
    update_sdconfig = 1;
    break;
   case GET_CONFIGTIME_COMMAND:
    configTimeResponse = 1;
    break;
   case GET_DIR_COMMAND:
    dirResponse = 1;
    break;
   case SET_CONFIGTIME_COMMAND:
    name_len = args[0]<(MAX_CHARS-1)?args[0]:(MAX_CHARS-1);
    memcpy(configTimeText, &args[1], name_len);
    configTimeText[args[0]]='\0';
    SetName();
    config_time = atol((char*)configTimeText);
    my_config_time[3] = *((uint8_t*)&config_time);
    my_config_time[2] = *(((uint8_t*)&config_time)+1);
    my_config_time[1] = *(((uint8_t*)&config_time)+2);
    my_config_time[0] = *(((uint8_t*)&config_time)+3);
    memcpy(&sdHeadText[SDH_CONFIG_TIME_0], my_config_time, 4);
    memcpy((uint8_t*)(storedConfig+NV_SD_CONFIG_TIME), my_config_time, 4);
    InfoMem_write((uint8_t*)NV_SD_CONFIG_TIME, storedConfig+NV_SD_CONFIG_TIME, 4);
    update_sdconfig = 1;
    break;
   case GET_NSHIMMER_COMMAND:
    nshimmerResponse = 1;
    break;
   case SET_NSHIMMER_COMMAND:
    storedConfig[NV_SD_NSHIMMER] = args[0];
    sdHeadText[SDH_NSHIMMER] = args[0];
    InfoMem_write((uint8_t*)NV_SD_NSHIMMER, storedConfig+NV_SD_NSHIMMER, 1);
    update_sdconfig = 1;
    break;
   case GET_MYID_COMMAND:
    myIDResponse = 1;
    break;
   case SET_MYID_COMMAND:
    storedConfig[NV_SD_MYTRIAL_ID] = args[0];
    sdHeadText[SDH_MYTRIAL_ID] = args[0];
    InfoMem_write((uint8_t*)NV_SD_MYTRIAL_ID, storedConfig+NV_SD_MYTRIAL_ID, 1);
    update_sdconfig = 1;
    break;
   case SET_LSM303DLHC_ACCEL_RANGE_COMMAND:
    if(args[0] < 4)
    storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0xF3) + ((args[0]&0x03)<<2);
    else
    storedConfig[NV_CONFIG_SETUP_BYTE0] &= 0xF3;
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE0, &storedConfig[NV_CONFIG_SETUP_BYTE0], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
    update_sdconfig = 1;
    if(isSensing) {
    stopSensing = 1;
    startSensing = 1;
    }
    break;
   case GET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
    lsm303dlhcAccelSamplingRateResponse = 1;
    break;
   case GET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
    lsm303dlhcAccelLPModeResponse = 1;
    break;
   case GET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
    lsm303dlhcAccelHRModeResponse = 1;
    break;
   case GET_MPU9250_GYRO_RANGE_COMMAND:
    mpu9250GyroRangeResponse = 1;
    break;*/
   case GET_BMP180_CALIBRATION_COEFFICIENTS_COMMAND:
      bmp180CalibCoeffBtRsp = 1;
      break;
  case GET_BMP280_CALIBRATION_COEFFICIENTS_COMMAND:
      bmp280CalibrationCoefficientsResponse = 1;
      break;
/*
  case GET_MPU9250_SAMPLING_RATE_COMMAND:
    mpu9250SamplingRateResponse = 1;
    break;
  case GET_MPU9250_ACCEL_RANGE_COMMAND:
    mpu9250AccelRangeResponse = 1;
    break;
  case GET_BMP180_PRES_OVERSAMPLING_RATIO_COMMAND:
    bmp180OversamplingRatioResponse = 1;
    break;
  case GET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
    internalExpPowerEnableResponse = 1;
    break;
  case GET_MPU9250_MAG_SENS_ADJ_VALS_COMMAND:
    mpu9250MagSensAdjValsResponse = 1;
    break;
  case GET_EXG_REGS_COMMAND:
    if(args[0]<2 && args[1]<10 && args[2]<11) {
      exgChip = args[0];
      exgStartAddr = args[1];
      exgLength = args[2];
    } else
      exgLength = 0;
    exgRegsResponse = 1;
    break;
*/
   /*   case SET_LSM303DLHC_ACCEL_SAMPLING_RATE_COMMAND:
    if(args[0] < 10)
      storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0x0F) + ((args[0]&0x0F)<<4);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0x0F) + (LSM303DLHC_ACCEL_100HZ<<4);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE0, &storedConfig[NV_CONFIG_SETUP_BYTE0], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_LSM303DLHC_MAG_GAIN_COMMAND:
    if(args[0]>0 && args[0]<8)
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0x1F) + ((args[0]&0x07)<<5);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0x1F) + (LSM303DLHC_MAG_1_3G<<5);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE2, &storedConfig[NV_CONFIG_SETUP_BYTE2], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE2] = storedConfig[NV_CONFIG_SETUP_BYTE2];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_LSM303DLHC_MAG_SAMPLING_RATE_COMMAND:
    if(args[0] < 8)
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0xE3) + ((args[0]&0x07)<<2);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0xE3) + (LSM303DLHC_MAG_75HZ<<2);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE2, &storedConfig[NV_CONFIG_SETUP_BYTE2], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE2] = storedConfig[NV_CONFIG_SETUP_BYTE2];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_LSM303DLHC_ACCEL_LPMODE_COMMAND:
    if(args[0] == 1){
      storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0xFD) + 0x02;
    }else{
      storedConfig[NV_CONFIG_SETUP_BYTE0] &= 0xFD;
    }
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE0, &storedConfig[NV_CONFIG_SETUP_BYTE0], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_LSM303DLHC_ACCEL_HRMODE_COMMAND:
    if(args[0] == 1){
      storedConfig[NV_CONFIG_SETUP_BYTE0] = (storedConfig[NV_CONFIG_SETUP_BYTE0]&0xFE) + 0x01;
    }else{
      storedConfig[NV_CONFIG_SETUP_BYTE0] &= 0xFE;
    }
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE0, &storedConfig[NV_CONFIG_SETUP_BYTE0], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE0] = storedConfig[NV_CONFIG_SETUP_BYTE0];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_MPU9250_GYRO_RANGE_COMMAND:
    if(args[0] < 4)
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0xFC) + (args[0]&0x03);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE2] = (storedConfig[NV_CONFIG_SETUP_BYTE2]&0xFC) + MPU9250_GYRO_500DPS;
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE2, &storedConfig[NV_CONFIG_SETUP_BYTE2], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE2] = storedConfig[NV_CONFIG_SETUP_BYTE2];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_MPU9250_SAMPLING_RATE_COMMAND:
    storedConfig[NV_CONFIG_SETUP_BYTE1] = args[0];
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE1, &storedConfig[NV_CONFIG_SETUP_BYTE1], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE1] = storedConfig[NV_CONFIG_SETUP_BYTE1];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_MPU9250_ACCEL_RANGE_COMMAND:
    if(args[0] < 4)
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0x3F) + ((args[0]&0x03)<<6);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0x3F) + (ACCEL_2G<<6);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE3, &storedConfig[NV_CONFIG_SETUP_BYTE3], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
    case SET_BMP180_PRES_OVERSAMPLING_RATIO_COMMAND:
    if(args[0] < 4)
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0xCF) + ((args[0]&0x03)<<4);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0xCF) + (BMP180_OSS_1<<4);
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE3, &storedConfig[NV_CONFIG_SETUP_BYTE3], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
    update_sdconfig = 1;
    break;
    case SET_INTERNAL_EXP_POWER_ENABLE_COMMAND:
    if(args[0] == 1)
      storedConfig[NV_CONFIG_SETUP_BYTE3] = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0xFE) + (args[0]&0x01);
    else
      storedConfig[NV_CONFIG_SETUP_BYTE3] &= 0xFE;
    InfoMem_write((void*)NV_CONFIG_SETUP_BYTE3, &storedConfig[NV_CONFIG_SETUP_BYTE3], 1);
    sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
    update_sdconfig = 1;
    if(isSensing) {
      stopSensing = 1;
      startSensing = 1;
    }
    break;
   */
   case GET_CONFIG_SETUP_BYTES_COMMAND:
      configSetupBytesResponse = 1;
      break;
   case SET_CONFIG_SETUP_BYTES_COMMAND:
      S4Ram_storedConfigSet(btArgs, NV_CONFIG_SETUP_BYTE0, 4);
      //InfoMem_update();
      break;
   case SET_SAMPLING_RATE_COMMAND:
      S4Ram_storedConfigSet(btArgs, NV_SAMPLING_RATE, 2);
      S4Ram_sdHeadTextSet(btArgs, SDH_SAMPLE_RATE_0, 2);
      //InfoMem_update();
      break;
   case GET_CALIB_DUMP_COMMAND:
      // usage:
      // 0x98, offset, offset, length
      btCalibRamLength = btArgs[0];
      btCalibRamOffset = btArgs[1] + (btArgs[2]<<8);
      calibRamResponse = 1;
      break;
   case SET_CALIB_DUMP_COMMAND:
      // usage:
      // 0x98, offset, offset, length, data[0:127]
      // max length of this command = 132
      btCalibRamLength = btArgs[0];
      btCalibRamOffset = btArgs[1] + (btArgs[2]<<8);
      if(ShimmerCalib_ramWrite(&btArgs[3], btCalibRamLength, btCalibRamOffset) == 1){
         //InfoMem_update();
//         ShimmerCalibSyncFromDumpRamAll();
//         update_calib_dump_file = 1;
      }
      break;
   case UPD_FLASH_COMMAND:
      InfoMem_update();
      //ShimmerCalibSyncFromDumpRamAll();
      //update_calib_dump_file = 1;
      break;
   /*   case SET_A_ACCEL_CALIBRATION_COMMAND:
    memcpy(&storedConfig[NV_A_ACCEL_CALIBRATION], args, 21);
    InfoMem_write((void*)NV_A_ACCEL_CALIBRATION, &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
    memcpy(&sdHeadText[SDH_A_ACCEL_CALIBRATION], &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
    calib_update = 1;
    calib_sensor = S_ACCEL_A;
    break;
    case GET_A_ACCEL_CALIBRATION_COMMAND:
    aAccelCalibrationResponse = 1;
    break;
    case SET_MPU9250_GYRO_CALIBRATION_COMMAND:
    memcpy(&storedConfig[NV_MPU9250_GYRO_CALIBRATION], args, 21);
    InfoMem_write((void*)NV_MPU9250_GYRO_CALIBRATION, &storedConfig[NV_MPU9250_GYRO_CALIBRATION], 21);
    memcpy(&sdHeadText[SDH_MPU9250_GYRO_CALIBRATION], &storedConfig[NV_MPU9250_GYRO_CALIBRATION], 21);
    calib_update = 1;
    calib_sensor = S_GYRO;
    calib_range = storedConfig[NV_CONFIG_SETUP_BYTE2] & 0x03;
    break;
    case GET_MPU9250_GYRO_CALIBRATION_COMMAND:
    gyroCalibrationResponse = 1;
    break;
    case SET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
    memcpy(&storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], args, 21);
    InfoMem_write((void*)NV_LSM303DLHC_MAG_CALIBRATION, &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
    memcpy(&sdHeadText[SDH_LSM303DLHC_MAG_CALIBRATION], &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
    calib_update = 1;
    calib_sensor = S_MAG;
    calib_range = (storedConfig[NV_CONFIG_SETUP_BYTE2]>>5) & 0x07;
    break;
    case GET_LSM303DLHC_MAG_CALIBRATION_COMMAND:
    magCalibrationResponse = 1;
    break;
    case SET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
    memcpy(&storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], args, 21);
    InfoMem_write((void*)NV_LSM303DLHC_ACCEL_CALIBRATION, &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
    memcpy(&sdHeadText[SDH_LSM303DLHC_ACCEL_CALIBRATION], &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
    calib_update = 1;
    calib_sensor = S_ACCEL_D;
    calib_range = (storedConfig[NV_CONFIG_SETUP_BYTE0]>>2)&0x03;
    break;*/
   case SET_GSR_RANGE_COMMAND:
      if (btArgs[0] <= 4){
         S4Ram_getStoredConfig()->gsrRange = btArgs[0] & 0x07;
      }else{
         S4Ram_getStoredConfig()->gsrRange = GSR_AUTORANGE;
      }
      //InfoMem_write((void*)NV_CONFIG_SETUP_BYTE3, &storedConfig[NV_CONFIG_SETUP_BYTE3], 1);
      //InfoMem_update();
      //sdHeadText[SDH_CONFIG_SETUP_BYTE3] = storedConfig[NV_CONFIG_SETUP_BYTE3];
//    update_sdconfig = 1;
//    if(isSensing) {
//      stopSensing = 1;
//      startSensing = 1;
//    }
      break;
   case SET_EXG_REGS_COMMAND:
      if (btArgs[0] < 2 && btArgs[1] < 10 && btArgs[2] < 11) {
         if (btArgs[0]) {
            S4Ram_storedConfigSet(&btArgs[3], NV_EXG_ADS1292R_2_CONFIG1 + btArgs[1], btArgs[2]);
            //InfoMem_update();
            //memcpy(sdHeadText + SDH_EXG_ADS1292R_2_CONFIG1, storedConfig + NV_EXG_ADS1292R_2_CONFIG1, btArgs[2]);
         } else {
            S4Ram_storedConfigSet(&btArgs[3], NV_EXG_ADS1292R_1_CONFIG1 + btArgs[1], btArgs[2]);
            //InfoMem_update();
            //memcpy(sdHeadText + SDH_EXG_ADS1292R_1_CONFIG1, storedConfig + NV_EXG_ADS1292R_1_CONFIG1, btArgs[2]);
         }
         //update_sdconfig = 1;
      }
      break;
   case GET_BT_VERSION_STR_COMMAND:
       btVerResponse = 1;
       break;
   case SET_DATA_RATE_TEST:
       /* Stop test before ACK is sent */
       if (btArgs[0] == 0)
       {
           setBtDataRateTestState(0);
           clearBtTxBuf(1);
       }
       btDataRateResponse = 1;
       break;

   /*     case RESET_TO_DEFAULT_CONFIGURATION_COMMAND:
     SetDefaultConfiguration();
     Config2SdHead();
     update_sdconfig = 1;
     configureAdcChannels = 1;
     if(isSensing) {
       stopSensing = 1;
       startSensing = 1;
     }
     break;
     case RESET_CALIBRATION_VALUE_COMMAND:
     memset(&storedConfig[NV_A_ACCEL_CALIBRATION], 0xFF, NV_NUM_CALIBRATION_BYTES);
     InfoMem_write((void*)NV_A_ACCEL_CALIBRATION, &storedConfig[NV_A_ACCEL_CALIBRATION], NV_NUM_CALIBRATION_BYTES);
     memcpy(&sdHeadText[SDH_LSM303DLHC_ACCEL_CALIBRATION], &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
     memcpy(&sdHeadText[SDH_MPU9250_GYRO_CALIBRATION], &storedConfig[NV_MPU9250_GYRO_CALIBRATION], 21);
     memcpy(&sdHeadText[SDH_LSM303DLHC_MAG_CALIBRATION], &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
     memcpy(&sdHeadText[SDH_A_ACCEL_CALIBRATION], &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
     calib_update = 1;
     break;
     case GET_LSM303DLHC_ACCEL_CALIBRATION_COMMAND:
     dAccelCalibrationResponse = 1;
     break;
     case GET_GSR_RANGE_COMMAND:
     gsrRangeResponse = 1;
     break;
     case GET_ALL_CALIBRATION_COMMAND:
     allCalibrationResponse = 1;
     break;
   */
   case DEPRECATED_GET_DEVICE_VERSION_COMMAND:
   case GET_DEVICE_VERSION_COMMAND:
      deviceVersionBtRsp = 1;
      break;
   case GET_FW_VERSION_COMMAND:
      fwVersionBtRsp = 1;
      break;
   case GET_CHARGE_STATUS_LED_COMMAND:
      blinkLedBtRsp = 1;
      break;/*
  case GET_BUFFER_SIZE_COMMAND:
    bufferSizeResponse = 1;
    break;
  case GET_UNIQUE_SERIAL_COMMAND:
    uniqueSerialResponse = 1;
    break;
*/
   case GET_DAUGHTER_CARD_ID_COMMAND:
      btDcMemLength = btArgs[0];
      btDcMemOffset = btArgs[1];
      if ((btDcMemLength <= 16) && (btDcMemOffset <= 15) && (btDcMemLength + btDcMemOffset <= 16))
         dcIdBtRsp = 1;
      break;
//   case SET_DAUGHTER_CARD_ID_COMMAND:
//      btDcMemLength = args[0];
//      btDcMemOffset = args[1];
//      if((btDcMemLength<=16) && (btDcMemOffset<=15) && (btDcMemLength+btDcMemOffset<=16)) {
//         CAT24C16_init();
//         CAT24C16_write(btDcMemOffset, btDcMemLength, args+2);
//         CAT24C16_powerOff();
//      }
//      break;
   case GET_DAUGHTER_CARD_MEM_COMMAND:
      btDcMemLength = btArgs[0];
      btDcMemOffset = btArgs[1] + (btArgs[2] << 8);
      if ((btDcMemLength <= 128) && (btDcMemOffset <= 2031) && (btDcMemLength + btDcMemOffset <= 2032))
         dcMemBtRsp = 1;
      break;
   case SET_DAUGHTER_CARD_MEM_COMMAND:
      btDcMemLength = btArgs[0];
      btDcMemOffset = btArgs[1] + (btArgs[2] << 8);
      if ((btDcMemLength <= 128) && (btDcMemOffset <= 2031) && (btDcMemLength + btDcMemOffset <= 2032)) {
         CAT24C16_write(btDcMemOffset + 16, &btArgs[3], btDcMemLength);
      }
      break;
   /*   case GET_BT_COMMS_BAUD_RATE:
    btCommsBaudRateResponse = 1;
    break;
    case SET_BT_COMMS_BAUD_RATE:
    if(args[0] != storedConfig[NV_BT_COMMS_BAUD_RATE]) {
      if(args[0]<11) {
        changeBtBaudRate = args[0];
      } else {
        changeBtBaudRate = 9;
      }
    }
    break;
    case GET_DERIVED_CHANNEL_BYTES:
    derivedChannelResponse = 1;
    break;
    case SET_DERIVED_CHANNEL_BYTES:
    memcpy(&storedConfig[NV_DERIVED_CHANNELS_0], &args[0], 3);
    InfoMem_write((void*)NV_DERIVED_CHANNELS_0, &storedConfig[NV_DERIVED_CHANNELS_0], 3);
    memcpy(&sdHeadText[SDH_DERIVED_CHANNELS_0], &storedConfig[NV_DERIVED_CHANNELS_0], 3);
    update_sdconfig = 1;
    break;
   */
   case GET_INFOMEM_COMMAND:
      btInfomemLength = btArgs[0];
      btInfomemOffset = btArgs[1] + (btArgs[2] << 8);
      if ((btInfomemLength <= 128) && (btInfomemOffset <= (NV_NUM_RWMEM_BYTES - 1)) && (btInfomemLength + btInfomemOffset <= NV_NUM_RWMEM_BYTES))
         infomemBtRsp = 1;
      break;
   case SET_INFOMEM_COMMAND:
      btInfomemLength = btArgs[0];
      btInfomemOffset = btArgs[1] + (btArgs[2] << 8);
      if ((btInfomemLength <= 128) && (btInfomemOffset <= (NV_NUM_RWMEM_BYTES - 1)) && (btInfomemLength + btInfomemOffset <= NV_NUM_RWMEM_BYTES)) {
//         memcpy(btMacHex, storedConfig + NV_MAC_ADDRESS, 6);
//         memcpy(storedConfig + btInfomemOffset, btArgs + 3, btInfomemLength);
//         memcpy(storedConfig + NV_MAC_ADDRESS, btMacHex, 6);
         uint8_t temp_btMacHex[6];
         S4Ram_storedConfigGet(temp_btMacHex, NV_MAC_ADDRESS, 6);
         S4Ram_storedConfigSet(&btArgs[3], btInfomemOffset, btInfomemLength);
         S4Ram_storedConfigSet(temp_btMacHex, NV_MAC_ADDRESS, 6);
         //InfoMem_update();
         //Infomem2Names();
         //update_sdconfig = 1;
         //if(((btInfomemOffset>=NV_A_ACCEL_CALIBRATION) && (btInfomemOffset<=NV_CALIBRATION_END)) ||
         //   (((btInfomemLength+btInfomemOffset)>=NV_A_ACCEL_CALIBRATION) && ((btInfomemLength+btInfomemOffset)<=NV_CALIBRATION_END)) ||
         //   ((btInfomemOffset<=NV_A_ACCEL_CALIBRATION) && ((btInfomemLength+btInfomemOffset)>=NV_CALIBRATION_END)))
         //   calib_update = 1;
      }
      else
         return;
      break;
   case GET_RWC_COMMAND:
      rwcResponse = 1;
      break;
   case SET_RWC_COMMAND:
      memcpy((uint8_t*)(&temp64), btArgs, 8);// 64bits = 8bytes
      RTC_init(temp64);
      S4Ram_getStoredConfig()->rtcSetByBt = 1;
      enableRTCAlarm(); //configure RTC alarm after time set from BT.
      //InfoMem_update();
      break;
   default:;
   }
   /* Send ACK back for all commands except when FW has received an ACK */
   if (btAction != ACK_COMMAND_PROCESSED
      /* ACK is sent back as part of SD_SYNC_RESPONSE so no need to send it here */
      && btAction != SET_SD_SYNC_COMMAND)
    {
        sendAck = 1;
        S4_Task_set(TASK_BTRESPONSE);
    }

//   if(update_sdconfig && CheckSdInslot()){
//      if(!docked)
//         UpdateSdConfig();
//      else
//         SetSdCfgFlag(1);
//   }
//   if(calib_update && CheckSdInslot()){
//      if(!docked)
//         CalibNewFile(calib_sensor, calib_range);
//      else
//         SetCalibFlag(1);
//      //   CalibAll();
//      //else
//      //   SetCalibFlag(1);
//   }
}

void BtUart_sendRsp(void) {
   uint16_t packet_length = 0;
   //STATTypeDef * stat = GetStatus();
   uint8_t bt_tx_data[RESPONSE_PACKET_SIZE];
   packet_length = 0;
   if (stat.isBtConnected) {
     if (sendAck)
     {
       *(bt_tx_data + packet_length++) = ACK_COMMAND_PROCESSED;
       sendAck = 0;
     }
      if (inquiryBtRsp) {
        /* Channel order/packet structure need to be assembled before sending the inquiry response so that the information is correct. */
        S4Sens_configureChannels();

         *(bt_tx_data + packet_length++) = INQUIRY_RESPONSE;
         //*(uint16_t *)(bt_tx_data + packet_length) = *(uint16_t *)(storedConfig + NV_SAMPLING_RATE); //ADC sampling rate
         S4Ram_storedConfigGet(bt_tx_data + packet_length, NV_SAMPLING_RATE, 2);
         packet_length += 2;
//         memcpy((bt_tx_data + packet_length), (storedConfig + NV_CONFIG_SETUP_BYTE0), 4);           //4 config bytes
         S4Ram_storedConfigGet(bt_tx_data + packet_length, NV_CONFIG_SETUP_BYTE0, 4);
         packet_length += 4;
         *(bt_tx_data + packet_length++) = sensing.nbrAdcChans + sensing.nbrDigiChans;                        //number of data channels
         *(bt_tx_data + packet_length++) = S4Ram_storedConfigGetByte(NV_BUFFER_SIZE);                      //buffer size
         memcpy((bt_tx_data + packet_length), sensing.cc, (sensing.nbrAdcChans + sensing.nbrDigiChans));
         packet_length += sensing.nbrAdcChans + sensing.nbrDigiChans;
         inquiryBtRsp = 0;
      } else if (samplingRateBtRsp) {
         *(bt_tx_data + packet_length++) = SAMPLING_RATE_RESPONSE;
         //*(uint16_t *)(bt_tx_data + packet_length) = *(uint16_t *)(storedConfig + NV_SAMPLING_RATE); //ADC sampling rate
         S4Ram_storedConfigGet(bt_tx_data + packet_length, NV_SAMPLING_RATE, 2);
         packet_length += 2;
         samplingRateBtRsp = 0;
//      } else if(lsm303dlhcAccelRangeResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_RANGE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE0] & 0x0C) >> 2;
//         lsm303dlhcAccelRangeResponse = 0;
//      } else if(lsm303dlhcMagGainResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_MAG_GAIN_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE2] & 0xE0) >> 5;
//         lsm303dlhcMagGainResponse = 0;
//      } else if(lsm303dlhcMagSamplingRateResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_MAG_SAMPLING_RATE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE2] & 0x1C) >> 2;
//         lsm303dlhcMagSamplingRateResponse = 0;
      } else if (dockStatusBtRsp) {
         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
         *(bt_tx_data + packet_length++) = STATUS_RESPONSE;
         *(bt_tx_data + packet_length++) = ((toggleLedRed & 0x01) << 7)
          + ((stat.badFile & 0x01) << 6)
          + ((stat.isSdInserted & 0x01) << 5)
          + ((stat.isStreaming & 0x01) << 4)
          + ((stat.isLogging & 0x01) << 3)
          + (isRwcTimeSet() << 2)
          + ((stat.isSensing & 0x01) << 1)
          + (stat.isDocked & 0x01);
         dockStatusBtRsp = 0;
#if defined(SHIMMER4_SDK)
      } else if (i2cvBattBtRsp) {
         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
         *(bt_tx_data + packet_length++) = RSP_I2C_BATT_STATUS_COMMAND;
         memcpy((bt_tx_data + packet_length), (uint8_t*)stat.battDigital, STC3100_DATA_LEN);
         packet_length += STC3100_DATA_LEN;
         i2cvBattBtRsp = 0;
#endif
      } else if (vbattBtRsp) {
         //ReadBatt();
         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
         *(bt_tx_data + packet_length++) = VBATT_RESPONSE;
         memcpy((uint8_t*)(bt_tx_data + packet_length), (uint8_t*)stat.battVal, 3);
         packet_length += 3;
         vbattBtRsp = 0;
//      } else if(trialConfigResponse) {
//         *(bt_tx_data + packet_length++) = TRIAL_CONFIG_RESPONSE;
//         memcpy((bt_tx_data + packet_length), (storedConfig+NV_SD_TRIAL_CONFIG0), 3);  //2 trial config bytes + 1 interval byte
//         packet_length+=3;
//         trialConfigResponse = 0;
//      } else if(centerResponse) {
//         centerResponse = 0;
//      } else if(shimmerNameResponse) {
//         SetShimmerName();
//         uint8_t shimmer_name_len = strlen((char*)shimmerName);
//         *(bt_tx_data + packet_length++) = SHIMMERNAME_RESPONSE;
//         *(bt_tx_data + packet_length++) = shimmer_name_len;
//         memcpy((bt_tx_data + packet_length), shimmerName, shimmer_name_len);
//         packet_length+=shimmer_name_len;
//         shimmerNameResponse = 0;
//      } else if(expIDResponse) {
//         SetExpIdName();
//         uint8_t exp_id_name_len = strlen((char*)expIdName);
//         *(bt_tx_data + packet_length++) = EXPID_RESPONSE;
//         *(bt_tx_data + packet_length++) = exp_id_name_len;
//         memcpy((bt_tx_data + packet_length), expIdName, exp_id_name_len);
//         packet_length+=exp_id_name_len;
//         expIDResponse = 0;
//      } else if(configTimeResponse) {
//         SetCfgTime();
//         uint8_t cfgtime_name_len = strlen((char*)configTimeText);
//         *(bt_tx_data + packet_length++) = CONFIGTIME_RESPONSE;
//         *(bt_tx_data + packet_length++) = cfgtime_name_len;
//         memcpy((bt_tx_data + packet_length), configTimeText, cfgtime_name_len);
//         packet_length+=cfgtime_name_len;
//         configTimeResponse = 0;
//      } else if(dirResponse) {
//         uint8_t dir_len = strlen((char*)fileName)-3;
//         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
//         *(bt_tx_data + packet_length++) = DIR_RESPONSE;
//         *(bt_tx_data + packet_length++) = dir_len;
//         memcpy((bt_tx_data + packet_length), fileName, dir_len);
//         packet_length+=dir_len;
//         dirResponse = 0;
//      } else if(nshimmerResponse) {
//         *(bt_tx_data + packet_length++) = NSHIMMER_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_SD_NSHIMMER];
//         nshimmerResponse = 0;
//      } else if(myIDResponse) {
//          *(bt_tx_data + packet_length++) = MYID_RESPONSE;
//          *(bt_tx_data + packet_length++) = storedConfig[NV_SD_MYTRIAL_ID];
//          myIDResponse = 0;
//      } else if(lsm303dlhcAccelSamplingRateResponse) {
//          *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_SAMPLING_RATE_RESPONSE;
//          *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE0] & 0xF0) >> 4;
//          lsm303dlhcAccelSamplingRateResponse = 0;
//      } else if(lsm303dlhcAccelLPModeResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_LPMODE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE0] & 0x02) >> 1;
//         lsm303dlhcAccelLPModeResponse = 0;
//      } else if(lsm303dlhcAccelHRModeResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_HRMODE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_CONFIG_SETUP_BYTE0] & 0x01;
//         lsm303dlhcAccelHRModeResponse = 0;
//      } else if(mpu9250GyroRangeResponse) {
//         *(bt_tx_data + packet_length++) = MPU9250_GYRO_RANGE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_CONFIG_SETUP_BYTE2] & 0x03;
//         mpu9250GyroRangeResponse = 0;
      } else if (bmp180CalibCoeffBtRsp) {
         *(bt_tx_data + packet_length++) = BMP180_CALIBRATION_COEFFICIENTS_RESPONSE;
//         BMP180_init(hi2cSensor);//todo: pre read at init
//         BMP180_getCalib(bt_tx_data + packet_length);
//         packet_length += 22;
//         memset((bt_tx_data + packet_length), 0, 24);
//         packet_length += 24;
//         bmp180CalibCoeffBtRsp = 0;
      } else if (bmp280CalibrationCoefficientsResponse) {
#if defined(SHIMMER3) | defined(SHIMMER4_SDK)
        if (isBmp280InUse())
        {
          *(bt_tx_data + packet_length++) = BMP280_CALIBRATION_COEFFICIENTS_RESPONSE;
          memcpy(resPacket + packet_length, bmpX80Calib, BMP280_CALIB_DATA_SIZE);
          packet_length += BMP280_CALIB_DATA_SIZE;
        }
#elif defined(SHIMMER3R)
        //TODO support BMP-390 once Consensys knows the Shimmer3r doesn't have a BMP280
        *(bt_tx_data + packet_length++) = BMP280_CALIBRATION_COEFFICIENTS_RESPONSE;
        packet_length += BMP280_CALIB_DATA_SIZE;
#endif
        bmp280CalibrationCoefficientsResponse = 0;

//      } else if(mpu9250SamplingRateResponse) {
//         *(bt_tx_data + packet_length++) = MPU9250_SAMPLING_RATE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_CONFIG_SETUP_BYTE1];
//         mpu9250SamplingRateResponse = 0;
//      } else if(mpu9250AccelRangeResponse) {
//         *(bt_tx_data + packet_length++) = MPU9250_ACCEL_RANGE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0xC0) >> 6;
//         mpu9250AccelRangeResponse = 0;
//      } else if(bmp180OversamplingRatioResponse) {
//         *(bt_tx_data + packet_length++) = BMP180_PRES_OVERSAMPLING_RATIO_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0x30) >> 4;
//         bmp180OversamplingRatioResponse = 0;
//      } else if(internalExpPowerEnableResponse) {
//         *(bt_tx_data + packet_length++) = INTERNAL_EXP_POWER_ENABLE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_CONFIG_SETUP_BYTE3]&0x01;
//         internalExpPowerEnableResponse = 0;
      } else if (configSetupBytesResponse) {
         *(bt_tx_data + packet_length++) = CONFIG_SETUP_BYTES_RESPONSE;
         //memcpy((bt_tx_data + packet_length), &storedConfig[NV_CONFIG_SETUP_BYTE0], 4);
         S4Ram_storedConfigGet(bt_tx_data + packet_length, NV_CONFIG_SETUP_BYTE0, 4);
         packet_length += 4;
         configSetupBytesResponse = 0;
      } else if(calibRamResponse) {
         *(bt_tx_data + packet_length++) = RSP_CALIB_DUMP_COMMAND;
         *(bt_tx_data + packet_length++) = btCalibRamLength;
         *(bt_tx_data + packet_length++) = btCalibRamOffset&0xff;
         *(bt_tx_data + packet_length++) = (btCalibRamOffset>>8)&0xff;
         ShimmerCalib_ramRead(bt_tx_data+packet_length, btCalibRamLength, btCalibRamOffset);
         packet_length += btCalibRamLength;
         calibRamResponse = 0;
//      } else if(aAccelCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = A_ACCEL_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_A_ACCEL_CALIBRATION], 21);
//         packet_length += 21;
//         aAccelCalibrationResponse = 0;
//      } else if(gyroCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = MPU9250_GYRO_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_MPU9250_GYRO_CALIBRATION], 21);
//         packet_length += 21;
//         gyroCalibrationResponse = 0;
//      } else if(magCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_MAG_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_LSM303DLHC_MAG_CALIBRATION], 21);
//         packet_length += 21;
//         magCalibrationResponse = 0;
//      } else if(dAccelCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = LSM303DLHC_ACCEL_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_LSM303DLHC_ACCEL_CALIBRATION], 21);
//         packet_length += 21;
//         dAccelCalibrationResponse = 0;
//      } else if(gsrRangeResponse) {
//         *(bt_tx_data + packet_length++) = GSR_RANGE_RESPONSE;
//         *(bt_tx_data + packet_length++) = (storedConfig[NV_CONFIG_SETUP_BYTE3]&0x0E) >> 1;
//         gsrRangeResponse = 0;
//      } else if(allCalibrationResponse) {
//         *(bt_tx_data + packet_length++) = ALL_CALIBRATION_RESPONSE;
//         memcpy((bt_tx_data+packet_length), &storedConfig[NV_A_ACCEL_CALIBRATION], NV_NUM_CALIBRATION_BYTES);
//         packet_length += NV_NUM_CALIBRATION_BYTES;
//         allCalibrationResponse = 0;
      } else if (deviceVersionBtRsp) {
         *(bt_tx_data + packet_length++) = DEVICE_VERSION_RESPONSE;
         *(bt_tx_data + packet_length++) = DEVICE_VER;
         deviceVersionBtRsp = 0;
//      } else if(mpu9250MagSensAdjValsResponse) {
//         MPU9250_init();
//         MPU9250_wake(1);
//         MPU9250_wake(0);
//         *(bt_tx_data + packet_length++) = MPU9250_MAG_SENS_ADJ_VALS_RESPONSE;
//         MPU9250_getMagSensitivityAdj(bt_tx_data+packet_length);
//         packet_length += 3;
//         mpu9250MagSensAdjValsResponse = 0;
      } else if (fwVersionBtRsp) {
         *(bt_tx_data + packet_length++) = FW_VERSION_RESPONSE;
         *(bt_tx_data + packet_length++) = FW_IDENTIFIER & 0xFF;
         *(bt_tx_data + packet_length++) = (FW_IDENTIFIER & 0xFF00) >> 8;
         *(bt_tx_data + packet_length++) = FW_VER_MAJOR & 0xFF;
         *(bt_tx_data + packet_length++) = (FW_VER_MAJOR & 0xFF00) >> 8;
         *(bt_tx_data + packet_length++) = FW_VER_MINOR;
         *(bt_tx_data + packet_length++) = FW_VER_REL;
         fwVersionBtRsp = 0;
      } else if (blinkLedBtRsp) {
         *(bt_tx_data + packet_length++) = CHARGE_STATUS_LED_RESPONSE;
         *(bt_tx_data + packet_length++) = stat.battStat;
         blinkLedBtRsp = 0;
//      } else if (bufferSizeResponse) {
//         *(bt_tx_data + packet_length++) = BUFFER_SIZE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_BUFFER_SIZE];
//         bufferSizeResponse = 0;
//      } else if (uniqueSerialResponse) {
//         *(bt_tx_data + packet_length++) = UNIQUE_SERIAL_RESPONSE;
//         memcpy((bt_tx_data+packet_length), dierecord, 8);
//         packet_length += 8;
//         uniqueSerialResponse = 0;
//      } else if (exgRegsResponse) {
//         *(bt_tx_data + packet_length++) = EXG_REGS_RESPONSE;
//         *(bt_tx_data + packet_length++) = exgLength;
//         if(exgLength) {
//            if(exgChip)
//               memcpy((bt_tx_data+packet_length), (storedConfig+NV_EXG_ADS1292R_2_CONFIG1+exgStartAddr), exgLength);
//            else
//               memcpy((bt_tx_data+packet_length), (storedConfig+NV_EXG_ADS1292R_1_CONFIG1+exgStartAddr), exgLength);
//            packet_length += exgLength;
//         }
//         exgRegsResponse = 0;
      } else if (dcIdBtRsp) {
         *(bt_tx_data + packet_length++) = DAUGHTER_CARD_ID_RESPONSE;
         *(bt_tx_data + packet_length++) = btDcMemLength;
#if IS_CONNECTED_EEPROM
         CAT24C16_read(btDcMemOffset, bt_tx_data + packet_length, btDcMemLength);
#else
         bt_tx_data[packet_length+0] = EXP_BRD_ID_MAJOR;
         bt_tx_data[packet_length+1] = EXP_BRD_ID_MINOR;
         bt_tx_data[packet_length+2] = EXP_BRD_ID_INTERNAL;
#endif
         packet_length += btDcMemLength;
         dcIdBtRsp = 0;
      } else if (dcMemBtRsp) {
         *(bt_tx_data + packet_length++) = DAUGHTER_CARD_MEM_RESPONSE;
         *(bt_tx_data + packet_length++) = btDcMemLength;
#if IS_CONNECTED_EEPROM
         CAT24C16_read(btDcMemOffset + 16, bt_tx_data + packet_length, btDcMemLength);
#endif
         packet_length += btDcMemLength;
         dcMemBtRsp = 0;
//      } else if (btCommsBaudRateResponse) {
//         *(bt_tx_data + packet_length++) = BT_COMMS_BAUD_RATE_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_BT_COMMS_BAUD_RATE];
//         btCommsBaudRateResponse = 0;
      } else if (infomemBtRsp) {
         *(bt_tx_data + packet_length++) = INFOMEM_RESPONSE;
         *(bt_tx_data + packet_length++) = btInfomemLength;
         //memcpy((bt_tx_data + packet_length), &storedConfig[btInfomemOffset], btInfomemLength);
         S4Ram_storedConfigGet(bt_tx_data + packet_length, btInfomemOffset, btInfomemLength);
         packet_length += btInfomemLength;
         infomemBtRsp = 0;
//      } else if (derivedChannelResponse) {
//         *(bt_tx_data + packet_length++) = DERIVED_CHANNEL_BYTES_RESPONSE;
//         *(bt_tx_data + packet_length++) = storedConfig[NV_DERIVED_CHANNELS_0];
//         *(bt_tx_data + packet_length++) = storedConfig[NV_DERIVED_CHANNELS_1];
//         *(bt_tx_data + packet_length++) = storedConfig[NV_DERIVED_CHANNELS_2];
//         derivedChannelResponse = 0;
      } else if(rwcResponse){
         uint64_t temp_rtcCurrentTime = RTC_get64();
         *(bt_tx_data + packet_length++) = RWC_RESPONSE;
         memcpy(bt_tx_data + packet_length, (uint8_t*)(&temp_rtcCurrentTime), 8);
         packet_length += 8;
         rwcResponse = 0;
      }
      else if (btVerResponse)
      {
          uint8_t btVerStrLen = getBtVerStrLen();

          *(bt_tx_data + packet_length++) = BT_VERSION_STR_RESPONSE;
          *(bt_tx_data + packet_length++) = btVerStrLen;
          memcpy((bt_tx_data + packet_length), getBtVerStrPtr(), btVerStrLen);
          packet_length += btVerStrLen;

          btVerResponse = 0;
      }
      else if (btDataRateResponse)
      {
          /* Start test after ACK is sent - this will be handled by the
           * interrupt after ACK byte is transmitted */
          if (btArgs[0] != 0)
          {
              setBtDataRateTestState(1);
          }
          btDataRateResponse = 0;
      }

      uint8_t crcMode = getBtCrcMode();
      if (crcMode != CRC_OFF)
      {
          calculateCrcAndInsert(crcMode, bt_tx_data, packet_length);
          packet_length += crcMode;
      }
      BT_write(bt_tx_data, packet_length);
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
//  if (btCrcMode == CRC_OFF)
//  {
//    HAL_CRC_DeInit(hcrc);
//  }
//  else
//  {
//    MX_CRC_Init();
//  }
}

COMMS_CRC_MODE getBtCrcMode(void)
{
    return btCrcMode;
}

uint8_t BT_getMacAddressAscii(char *macAscii) {
#if defined(SHIMMER3R)
  // MAC is stored as 6 byte array in CYW20820 library
  uint8_t *macAddrPtr = BT_getCyw20820MacAddressPtr();
  (void)sprintf(macAscii, "%02X%02X%02X%02X%02X%02X", macAddrPtr[5], macAddrPtr[4], macAddrPtr[3], macAddrPtr[2], macAddrPtr[1], macAddrPtr[0]);
  return 1;
#elif defined(SHIMMER4_SDK)
  if(BT_getRn42MacAddressPtr(macAddrPtr))
  {
      memcpy(macAscii, macAddrPtr, 12);
      return 0;
   } else {
      return 1;
   }
#endif
}

uint8_t BT_getMacAddressHex(uint8_t *macHex) {
#if defined(SHIMMER3R)
  uint8_t* ptr = BT_getCyw20820MacAddressPtr();
  macHex[0] = *(ptr + 5);
  macHex[1] = *(ptr + 4);
  macHex[2] = *(ptr + 3);
  macHex[3] = *(ptr + 2);
  macHex[4] = *(ptr + 1);
  macHex[5] = *(ptr + 0);
  return 1;
#elif defined(SHIMMER4_SDK)
  uint8_t i, pchar[3];
  if(BT_getRn42MacAddressPtr(macAddrPtr))
  {
    //memcpy(mac, rn42Mac, 12);
    pchar[2] = 0;
    for (i = 0; i < 6; i++)
    {
      pchar[0] = macAddrPtr[i * 2];
      pchar[1] = macAddrPtr[i * 2 + 1];
      macHex[i] = strtoul((char*) pchar, 0, 16);
    }
    return 0;
  }
  else
  {
    return 1;
  }
#endif
}
