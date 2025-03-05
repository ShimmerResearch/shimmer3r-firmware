/**
 * \copyright Copyright (C) 2019-2021 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "ads7028.h"
#include "s4_ram.h"
#include "spi.h"
//****************************************************************************
//
// Internal variables
//
//****************************************************************************

/** Array used to recall device register map configurations */
static uint8_t      registerMap[MAX_REGISTER_ADDRESS + 1];
#define SENSOR_BUS hspi1
//ADCTypeDef Ext_adc;
uint8_t enabledChannels = 0x00;

//****************************************************************************
//
// Internal Function prototypes
//
//****************************************************************************
static void         restoreRegisterDefaults(void);
static int16_t      signExtend(const uint8_t dataBytes[]);


//****************************************************************************
//
// Function Definitions
//
//****************************************************************************

//*****************************************************************************
//
//! \brief Example start up sequence for the ADS7038.
//!
//! \fn void initADS7038(void)
//!
//! Before calling this function, the device must be powered,
//! the SPI pins of the MCU must have already been configured.
//!
//! \return None.
//
//*****************************************************************************

bool ads7028_whoAmI(void)
{
    uint16_t response;
    setAds7028CS(LOW);
    response = readSingleRegister(ADS7028_ADDRESS_DEVICE_ID); // WHO_AM_I Need to verify this.
    setAds7028CS(HIGH);
    return (response == ADS7028_DEVICE_ID);
}

uint8_t initADS7028(void)
{
    // Clear BOR flag
    setAds7028CS(LOW);
    //resetDevice() //Reset
    if(!ads7028_whoAmI())
    {
      return 0;
    }
    writeSingleRegister(DATA_CFG_ADDRESS, DATA_CFG_CPOL_CPHA_POLARITY0_PHASE1); //SPI 1 Since by default it is on SPI0 and returns to SPI0 on power cycle this will be sent via SPI0.
    setAds7028CS(HIGH);
    return 1;
}

//*****************************************************************************
//
//! \brief  Resets the device and reinitializes the register map array
//!         maintained in firmware to default values.
//!
//! \fn     void resetDevice()
//!
//! \return None
//
//*****************************************************************************
void resetDevice()
{
    // Set the RST bit high to reset the device
    setAds7028CS(LOW);
    setRegisterBits(GENERAL_CFG_ADDRESS, GENERAL_CFG_RST_MASK);
    setAds7028CS(HIGH);
    // Update internal register map array
    restoreRegisterDefaults();
}

void ads7028_configureChannels(void)
{
  uint8_t *channel_contents_ptr = sensing.cc + sensing.ccLen;
  uint8_t nbr_adc_chans = 0;
  gConfigBytes *configBytes = S4Ram_getStoredConfig();
  Ext_adc.sensorLen = 0; //adc.sensorCnt = 0;
  Ext_adc.chanCntSens = Ext_adc.chanCntBatt = 0;
  // Configure pin as analog input
  gConfigBytes *storedConfig = S4Ram_getStoredConfig();

  // Select channel  and enable as per config
  if(storedConfig->chEnVBattery)
  {
  *channel_contents_ptr++ = VBATT;
  nbr_adc_chans += 1;
  sensing.ptr.batteryAnalog = sensing.dataLen;
  sensing.dataLen += 2;
  Ext_adc.sensorList[Ext_adc.sensorLen++] = VBATT;
  enabledChannels |= AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH7_ENABLED;
  }
  //External ADC 0
  if (configBytes->chEnExtADC0)
  {
    *channel_contents_ptr++ = EXTERNAL_ADC_0;
    nbr_adc_chans += 1;
    sensing.ptr.extADC0 = sensing.dataLen;
    sensing.dataLen += 2;
    Ext_adc.sensorList[Ext_adc.sensorLen++] = EXTERNAL_ADC_0;
    enabledChannels |= AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH4_ENABLED;
  }

  //External ADC 1
  if (configBytes->chEnExtADC1)
  {
    *channel_contents_ptr++ = EXTERNAL_ADC_1;
    nbr_adc_chans += 1;
    sensing.ptr.extADC1 = sensing.dataLen;
    sensing.dataLen += 2;
    Ext_adc.sensorList[Ext_adc.sensorLen++] = EXTERNAL_ADC_1;
    enabledChannels |= AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH5_ENABLED;
  }

  //External ADC 2
  if (configBytes->chEnExtADC2)
  {
    *channel_contents_ptr++ = EXTERNAL_ADC_2;
    nbr_adc_chans += 1;
    sensing.ptr.extADC2 = sensing.dataLen;
    sensing.dataLen += 2;
    Ext_adc.sensorList[Ext_adc.sensorLen++] = EXTERNAL_ADC_2;
    enabledChannels |= AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH6_ENABLED;
  }

  //Internal ADC 0
  if (configBytes->chEnIntADC0)
  {
    *channel_contents_ptr++ = INTERNAL_ADC_0;
    nbr_adc_chans += 1;
    sensing.ptr.intADC0 = sensing.dataLen;
    sensing.dataLen += 2;
    Ext_adc.sensorList[Ext_adc.sensorLen++] = INTERNAL_ADC_0;
    enabledChannels |= AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH0_ENABLED;
  }
  //Internal ADC 1
  if (configBytes->chEnIntADC1)
  {
    *channel_contents_ptr++ = INTERNAL_ADC_1;
    nbr_adc_chans += 1;
    sensing.ptr.intADC1 = sensing.dataLen;
    sensing.dataLen += 2;
    Ext_adc.sensorList[Ext_adc.sensorLen++] = INTERNAL_ADC_1;
    enabledChannels |= AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH1_ENABLED;
  }

  //Internal ADC 2
  if (configBytes->chEnIntADC2)
  {
    *channel_contents_ptr++ = INTERNAL_ADC_2;
    nbr_adc_chans += 1;
    sensing.ptr.intADC2 = sensing.dataLen;
    sensing.dataLen += 2;
    Ext_adc.sensorList[Ext_adc.sensorLen++] = INTERNAL_ADC_2;
    enabledChannels |= AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH2_ENABLED;
  }

  //Internal ADC 3
  if (configBytes->chEnIntADC3 || configBytes->chEnGsr)
  {
    if (configBytes->chEnGsr)
    {
      *channel_contents_ptr++ = GSR_RAW;
      sensing.ptr.gsr = sensing.dataLen;
      Ext_adc.sensorList[Ext_adc.sensorLen++] = GSR_RAW;
    }
    else
    {
      *channel_contents_ptr++ = INTERNAL_ADC_3;
      sensing.ptr.intADC3 = sensing.dataLen;
      Ext_adc.sensorList[Ext_adc.sensorLen++] = INTERNAL_ADC_3;
      enabledChannels |= AUTO_SEQ_CHSEL_AUTO_SEQ_CHSEL_CH3_ENABLED;
    }
    nbr_adc_chans += 1;
    sensing.dataLen += 2;
  }

  sensing.nbrAdcChans += nbr_adc_chans;
  sensing.ccLen += nbr_adc_chans;
}

void ads7028Configure(void)
{
  setAds7028CS(LOW);
  writeSingleRegister(AUTO_SEQ_CHSEL_ADDRESS, enabledChannels); //Write registor setting selecting channels.
  writeSingleRegister(GENERAL_CFG_ADDRESS,GENERAL_CFG_RST_RESET_DEVICE_REGS); // Set all Channels as Analog Inputs.
  writeSingleRegister(DATA_CFG_ADDRESS, DATA_CFG_APPEND_STATUS_FOUR_BIT_CHID); // Append Channel ID to ADC data
  setAds7028CS(HIGH);
}

void ads7028DataGet(uint8_t* data)
{
  HAL_StatusTypeDef ret;
  setAds7028CS(LOW);

  setRegisterBits(SEQUENCE_CFG_SEQ_START_ENABLED, SEQUENCE_CFG_SEQ_START_MASK); //Start Conversion
   *data = readData(data);

  setAds7028CS(HIGH);
}

//*****************************************************************************
//
//! Stops ADC conversions
//!
//! \fn void stopConversions(void)
//!
//! \return None.
//
//*****************************************************************************
void stopConversions(void)
{
    // Set nCS pin HIGH, allows MCU to communicate with other devices on SPI bus
    setAds7028CS(HIGH);
}

//*****************************************************************************
//
//! \brief  Reads ADC conversion result and returns 16-bit sign-extended value.
//!
//! \fn void readData(uint8_t dataRx[])
//!
//! \param *dataRx points to receive data byte array
//!
//! \return int16_t (sign-extended data).
//
//*****************************************************************************
int16_t readData(uint8_t* dataRx)
{
    uint8_t numberOfBytes = SPI_CRC_ENABLED ? 4 : 3;

    // NULL command
    uint8_t dataTx[4] = { 0 };
    if (SPI_CRC_ENABLED)
    {
        dataTx[3] = calculateCRC(dataTx, numberOfBytes - 1, CRC_INITIAL_SEED);
    }
    spiSendReceiveArray(&SENSOR_BUS, &dataTx[0], dataRx, numberOfBytes);

    return signExtend(dataRx);
}

//*****************************************************************************
//
//! \brief  Reads the contents of a single register at the specified address.
//!
//! \fn     uint8_t readSingleRegister(uint8_t address)
//!
//! \param  address is the 8-bit address of the register to read.
//!
//! \return Returns the 8-bit register read result.
//
//*****************************************************************************
uint8_t readSingleRegister(uint8_t address)
{
    // Check that the register address is in range
    assert(address <= MAX_REGISTER_ADDRESS);

    uint8_t dataTx[4] = {0};
    uint8_t dataRx[4] = {0};
    uint8_t numberOfBytes = SPI_CRC_ENABLED ? 4 : 3;
    bool crcError = false;

    //
    // [FRAME 1] RREG command
    //
    dataTx[0] = OPCODE_RREG;
    dataTx[1] = address;
    dataTx[2] = OPCODE_NULL;
    if (SPI_CRC_ENABLED)
    {
        dataTx[3] = calculateCRC(dataTx, numberOfBytes - 1, CRC_INITIAL_SEED);
    }
    spiSendReceiveArray(&SENSOR_BUS,&dataTx[0], &dataRx[0], numberOfBytes);

    //
    // [FRAME 2] NULL command
    //
    dataTx[0] = OPCODE_NULL;
    dataTx[1] = OPCODE_NULL;
    dataTx[2] = OPCODE_NULL;
    if (SPI_CRC_ENABLED)
    {
        dataTx[3] = calculateCRC(dataTx, numberOfBytes - 1, CRC_INITIAL_SEED);
    }
    spiSendReceiveArray(&SENSOR_BUS, &dataTx[0], &dataRx[0], numberOfBytes);

    // Check for CRC error
    if (SPI_CRC_ENABLED)
    {
        // To check the CRC validity you can test either of the following conditions:
        // 1) "dataRx[1] == calculateCRC(dataRx, 1, CRC_INITIAL_SEED)" - true means no CRC error occurred.
        // 2) "0x00 == calculateCRC(dataRx, 2, CRC_INITIAL_SEED) - including the CRC byte in the calculation should return 0x00.
        crcError = (bool) calculateCRC(dataRx, 2, CRC_INITIAL_SEED);
    }
    if (crcError)
    {
        // Update internal register array
        registerMap[SYSTEM_STATUS_ADDRESS] = registerMap[SYSTEM_STATUS_ADDRESS] || SYSTEM_STATUS_CRCERR_IN_MASK;

        // (OPTIONAL) Consider notifying the system of the error and repeating the previous command.
    }
    else
    {
        registerMap[address] = dataRx[0];
    }

    return registerMap[address];
}


//*****************************************************************************
//
//! Getter function to access registerMap array from outside of this module.
//!
//! \fn uint8_t getRegisterValue(uint8_t address)
//!
//! NOTE: The internal registerMap array stores the last known register value,
//! since the last read or write operation to that register. This function
//! does not communicate with the device to retrieve the current register value.
//! For the most up-to-date register data or retrieving the value of a hardware
//! controlled register, it is recommend to use readSingleRegister() to read the
//! current register value.
//!
//! \return unsigned 8-bit register value.
//
//*****************************************************************************
uint8_t getRegisterValue(uint8_t address)
{
    assert(address <= MAX_REGISTER_ADDRESS);
    return registerMap[address];
}


//*****************************************************************************
//
//! \brief  Writes data to a single register and reads it back for confirmation.
//!
//! \fn void writeSingleRegister(uint8_t address, uint8_t data)
//!
//! \param address is the address of the register to write to.
//! \param data is the value to write.
//!
//! \return None.
//
//*****************************************************************************
void writeSingleRegister(uint8_t address, uint8_t data)
{
    // Check that the register address is in range
    assert(address <= MAX_REGISTER_ADDRESS);
    uint8_t dataTx[4] = { 0 };
    uint8_t dataRx[4] = { 0 };
    uint8_t numberOfBytes = SPI_CRC_ENABLED ? 4 : 3;

    // (OPTIONAL) Check for and clear CRC error to proceed with register write.
    // Once a CRC error has occurred, writes are only allowed to the SYSTEM_STATUS and GENERAL_CFG registers
    if (SPI_CRC_ENABLED && (address > GENERAL_CFG_ADDRESS))
    {
        // Read STATUS register to check whether CRC error has occurred or not.
        readSingleRegister(SYSTEM_STATUS_ADDRESS);
        if (SPI_CRCERR_IN)
        {
            // (OPTIONAL) Clear the CRC error by writing 1b to CRCERR_IN bit
            setRegisterBits(SYSTEM_STATUS_ADDRESS, SYSTEM_STATUS_CRCERR_IN_MASK);

            // (OPTIONAL) Consider notifying the system of the error and repeating the previous command.
        }
    }

    // WREG command
    dataTx[0] = OPCODE_WREG;
    dataTx[1] = address;
    dataTx[2] = data;
    if (SPI_CRC_ENABLED)
    {
        dataTx[3] = calculateCRC(dataTx, numberOfBytes - 1, CRC_INITIAL_SEED);
    }
    spiSendReceiveArray(&SENSOR_BUS, &dataTx[0], &dataRx[0], numberOfBytes);

    // Update internal register map array (assume command was successful).
    // NOTE: This is required for writing to the CRC_EN bit to ensure read back uses the correct mode.
    registerMap[address] = data;

    // NOTE: If you modify the CPOL_CPHA bits in the DATA_CFG register, the SPI perhiperal will need to be reconfigured here.

    // (RECOMMENDED) Read back register to confirm register write was successful
    registerMap[address] = readSingleRegister(address);
}


//*****************************************************************************
//
//! \fn void setRegisterBits(uint8_t address, uint8_t bitMask)
//!
//! \param address is the address of the register to write to.
//! \param bitMask indicates which bit(s) in the register to set.
//!
//! This function does not perform a read back of the register value.
//!
//! \return None.
//
//*****************************************************************************
void setRegisterBits(uint8_t address, uint8_t bitMask)
{
    // Check that the register address is in range
    assert(address <= MAX_REGISTER_ADDRESS);

    uint8_t dataTx[4] = {0};
    uint8_t dataRx[4] = {0};
    uint8_t numberOfBytes = SPI_CRC_ENABLED ? 4 : 3;

    // SETBIT command
    dataTx[0] = OPCODE_SETBIT;
    dataTx[1] = address;
    dataTx[2] = bitMask;
    if (SPI_CRC_ENABLED)
    {
        dataTx[3] = calculateCRC(dataTx, numberOfBytes - 1, CRC_INITIAL_SEED);
    }
    spiSendReceiveArray(&SENSOR_BUS, &dataTx[0], &dataRx[0], numberOfBytes);

    // Update internal register map array (assume command was successful).
    // NOTE: This is required for writing to the CRC_EN bit to ensure read back uses the correct mode.
    registerMap[address] = registerMap[address] | bitMask;

    // (OPTIONAL) Check if a CRC error occurred
}


//*****************************************************************************
//
//!
//! \fn void clearRegisterBits(uint8_t address, uint8_t bitMask)
//!
//! \param address is the address of the register to write to.
//! \param bitMask indicates which bit(s) in the register to clear.
//!
//! This function does not perform a read back of the register value.
//!
//! \return None.
//
//*****************************************************************************
void clearRegisterBits(uint8_t address, uint8_t bitMask)
{
    // Check that the register address is in range
    assert(address <= MAX_REGISTER_ADDRESS);

    uint8_t dataTx[4] = {0};
    uint8_t dataRx[4] = {0};
    uint8_t numberOfBytes = SPI_CRC_ENABLED ? 4 : 3;

    // CLRBIT command
    dataTx[0] = OPCODE_CLRBIT;
    dataTx[1] = address;
    dataTx[2] = bitMask;
    if (SPI_CRC_ENABLED)
    {
        dataTx[3] = calculateCRC(dataTx, numberOfBytes - 1, CRC_INITIAL_SEED);
    }
    spiSendReceiveArray(&SENSOR_BUS, &dataTx[0], &dataRx[0], numberOfBytes);

    // Update internal register map array (assume command was successful).
    // NOTE: This is required for writing to the CRC_EN bit to ensure read back uses the correct mode.
    registerMap[address] = registerMap[address] & ~bitMask;

    // (OPTIONAL) Check if a CRC error occurred
}


//*****************************************************************************
//
//! \brief  Calculates the 8-bit CRC for the selected CRC polynomial.
//!
//! \fn uint8_t calculateCRC(const uint8_t dataBytes[], uint8_t numberBytes, uint8_t initialValue)
//!
//! \param dataBytes[] pointer to first element in the data byte array
//! \param numberBytes number of bytes to be used in CRC calculation
//! \param initialValue the seed value (or partial crc calculation), use CRC_INITIAL_SEED when beginning a new CRC computation
//!
//! NOTE: This calculation is shown as an example and is not optimized for speed.
//!
//! \return 8-bit calculated CRC word
//
//*****************************************************************************
uint8_t calculateCRC(const uint8_t dataBytes[], uint8_t numberBytes, uint8_t initialValue)
{
    // Check that "dataBytes" is not a null pointer
    assert(dataBytes != 0x00);

    int         bitIndex, byteIndex;
    bool        dataMSb;                        /* Most significant bit of data byte */
    bool        crcMSb;                         /* Most significant bit of crc byte  */

    // Initial value of crc register
    // Use 0x00 when starting a new computation OR provide result of previous CRC calculation when continuing an on-going calculation.
    uint8_t crc = initialValue;

    // CRC polynomial = x^8 + x^2 + x^1 + 1
    const uint8_t poly = 0x07;

    /* CRC algorithm */

    // Loop through all bytes in the dataBytes[] array
    for (byteIndex = 0; byteIndex < numberBytes; byteIndex++)
    {
        // Point to MSb in byte
        bitIndex = 0x80u;

        // Loop through all bits in the current byte
        while (bitIndex > 0)
        {
            // Check MSB's of data and crc
            dataMSb = (bool) (dataBytes[byteIndex] & bitIndex);
            crcMSb  = (bool) (crc & 0x80u);

            // Update crc register
            crc <<= 1;
            if (dataMSb ^ crcMSb) { crc ^= poly; }

            // Shift MSb pointer to the next data bit
            bitIndex >>= 1;
        }
    }

    return crc;
}
void ads7028ProcessData(uint8_t ChID, uint16_t data)
{
  if(ChID == ADS7028_INT_EXP0)
  {
    sensing.dataBuf[sensing.ptr.intADC0 + 0] =  data & 0xFF;
    sensing.dataBuf[sensing.ptr.intADC0 + 1] =  (data >> 8) & 0xFF;
  }
  else if(ChID == ADS7028_INT_EXP1)
  {
    sensing.dataBuf[sensing.ptr.intADC1 + 0] =  data & 0xFF;
    sensing.dataBuf[sensing.ptr.intADC1 + 1] =  (data >> 8) & 0xFF;
  }
  else if(ChID == ADS7028_INT_EXP2)
  {
    sensing.dataBuf[sensing.ptr.intADC2 + 0] =  data & 0xFF;
    sensing.dataBuf[sensing.ptr.intADC2 + 1] =  (data >> 8) & 0xFF;
  }
  else if(ChID == ADS7028_INT_EXP3)
  {
    sensing.dataBuf[sensing.ptr.intADC3 + 0] =  data & 0xFF;
    sensing.dataBuf[sensing.ptr.intADC3 + 1] = (data >> 8) & 0xFF;
  }
  else if(ChID == ADS7028_EXT_EXP0)
  {
    sensing.dataBuf[sensing.ptr.extADC0 + 0] =  data & 0xFF;
    sensing.dataBuf[sensing.ptr.extADC0 + 1] = (data >> 8) & 0xFF;
  }
  else if(ChID == ADS7028_EXT_EXP1)
  {
    sensing.dataBuf[sensing.ptr.extADC1 + 0] =  data & 0xFF;
    sensing.dataBuf[sensing.ptr.extADC1 + 1] = (data >> 8) & 0xFF;
  }
  else if(ChID == ADS7028_EXT_EXP2)
  {
    sensing.dataBuf[sensing.ptr.extADC2 + 0] = data & 0xFF;
    sensing.dataBuf[sensing.ptr.extADC2 + 1] = (data >> 8) & 0xFF;
  }
  else if(ChID == ADS7028_VBATT)
  {
    sensing.dataBuf[sensing.ptr.batteryAnalog + 0] = data & 0xFF;
    sensing.dataBuf[sensing.ptr.batteryAnalog + 1] = (data >> 8) & 0xFF;
  }
}

//*****************************************************************************
//
//! Configure the selected channel as an analog Input
//!
//! \fn void setChannelAsAnalogInput(uint8_t channelID)
//!
//! \param channelID is the channel number.
//!
//! \return None
//
//*****************************************************************************
void setChannelAsAnalogInput(uint8_t channelID)
{
    // Check that channel ID is in range.
    assert(channelID < 8);

    // Clear the corresponding channel bit to configure channel as an analog input
    clearRegisterBits(PIN_CFG_ADDRESS, (1 << channelID));
}


//****************************************************************************
//
// Helper functions
//
//****************************************************************************

//*****************************************************************************
//
//! \brief  Updates the registerMap[] array to its default values.
//!
//! \fn static void restoreRegisterDefaults(void)
//!
//! NOTES:
//! - If the MCU keeps a copy of the ADS7038 register settings in memory,
//! then it is important to ensure that these values remain in sync with the
//! actual hardware settings. In order to help facilitate this, this function
//! should be called after powering up or resetting the device (either by
//! hardware pin control or SPI software command).
//!
//! - Reading back all of the registers after resetting the device can
//! accomplish the same result; however, this might be problematic if the
//! device was previously in CRC mode , since resetting the device exits
//! these modes. If the MCU is not aware of this mode change, then read
//! register commands will fail.
//!
//! \return None.
//
//*****************************************************************************
static void restoreRegisterDefaults(void)
{
    registerMap[SYSTEM_STATUS_ADDRESS]          = SYSTEM_STATUS_DEFAULT;
    registerMap[GENERAL_CFG_ADDRESS]            = GENERAL_CFG_DEFAULT;

    registerMap[DATA_CFG_ADDRESS]               = DATA_CFG_DEFAULT;
    registerMap[OSR_CFG_ADDRESS]                = OSR_CFG_DEFAULT;
    registerMap[OPMODE_CFG_ADDRESS]             = OPMODE_CFG_DEFAULT;
    registerMap[PIN_CFG_ADDRESS]                = PIN_CFG_DEFAULT;

    registerMap[GPIO_CFG_ADDRESS]               = GPIO_CFG_DEFAULT;
    registerMap[GPO_DRIVE_CFG_ADDRESS]          = GPO_DRIVE_CFG_DEFAULT;
    registerMap[GPO_OUTPUT_VALUE_ADDRESS]       = GPO_OUTPUT_VALUE_DEFAULT;
    registerMap[GPI_VALUE_ADDRESS]              = GPI_VALUE_DEFAULT;

    registerMap[SEQUENCE_CFG_ADDRESS]           = SEQUENCE_CFG_DEFAULT;
    registerMap[CHANNEL_SEL_ADDRESS]            = CHANNEL_SEL_DEFAULT;
    registerMap[AUTO_SEQ_CHSEL_ADDRESS]         = AUTO_SEQ_CHSEL_DEFAULT;

    registerMap[ALERT_CH_SEL_ADDRESS]           = ALERT_CH_SEL_DEFAULT;
    registerMap[ALERT_MAP_ADDRESS]              = ALERT_MAP_DEFAULT;
    registerMap[ALERT_PIN_CFG_ADDRESS]          = ALERT_PIN_CFG_DEFAULT;

    registerMap[EVENT_FLAG_ADDRESS]             = EVENT_FLAG_DEFAULT;
    registerMap[EVENT_HIGH_FLAG_ADDRESS]        = EVENT_HIGH_FLAG_DEFAULT;
    registerMap[EVENT_LOW_FLAG_ADDRESS]         = EVENT_LOW_FLAG_DEFAULT;
    registerMap[EVENT_RGN_ADDRESS]              = EVENT_RGN_DEFAULT;

    registerMap[HYSTERESIS_CH0_ADDRESS]         = HYSTERESIS_CHx_DEFAULT;
    registerMap[HYSTERESIS_CH1_ADDRESS]         = HYSTERESIS_CHx_DEFAULT;
    registerMap[HYSTERESIS_CH2_ADDRESS]         = HYSTERESIS_CHx_DEFAULT;
    registerMap[HYSTERESIS_CH3_ADDRESS]         = HYSTERESIS_CHx_DEFAULT;
    registerMap[HYSTERESIS_CH4_ADDRESS]         = HYSTERESIS_CHx_DEFAULT;
    registerMap[HYSTERESIS_CH5_ADDRESS]         = HYSTERESIS_CHx_DEFAULT;
    registerMap[HYSTERESIS_CH6_ADDRESS]         = HYSTERESIS_CHx_DEFAULT;
    registerMap[HYSTERESIS_CH7_ADDRESS]         = HYSTERESIS_CHx_DEFAULT;

    registerMap[EVENT_COUNT_CH0_ADDRESS]        = EVENT_COUNT_CHx_DEFAULT;
    registerMap[EVENT_COUNT_CH1_ADDRESS]        = EVENT_COUNT_CHx_DEFAULT;
    registerMap[EVENT_COUNT_CH2_ADDRESS]        = EVENT_COUNT_CHx_DEFAULT;
    registerMap[EVENT_COUNT_CH3_ADDRESS]        = EVENT_COUNT_CHx_DEFAULT;
    registerMap[EVENT_COUNT_CH4_ADDRESS]        = EVENT_COUNT_CHx_DEFAULT;
    registerMap[EVENT_COUNT_CH5_ADDRESS]        = EVENT_COUNT_CHx_DEFAULT;
    registerMap[EVENT_COUNT_CH6_ADDRESS]        = EVENT_COUNT_CHx_DEFAULT;
    registerMap[EVENT_COUNT_CH7_ADDRESS]        = EVENT_COUNT_CHx_DEFAULT;

    registerMap[HIGH_TH_CH0_ADDRESS]            = HIGH_TH_CHx_DEFAULT;
    registerMap[HIGH_TH_CH1_ADDRESS]            = HIGH_TH_CHx_DEFAULT;
    registerMap[HIGH_TH_CH2_ADDRESS]            = HIGH_TH_CHx_DEFAULT;
    registerMap[HIGH_TH_CH3_ADDRESS]            = HIGH_TH_CHx_DEFAULT;
    registerMap[HIGH_TH_CH4_ADDRESS]            = HIGH_TH_CHx_DEFAULT;
    registerMap[HIGH_TH_CH5_ADDRESS]            = HIGH_TH_CHx_DEFAULT;
    registerMap[HIGH_TH_CH6_ADDRESS]            = HIGH_TH_CHx_DEFAULT;
    registerMap[HIGH_TH_CH7_ADDRESS]            = HIGH_TH_CHx_DEFAULT;

    registerMap[LOW_TH_CH0_ADDRESS]             = LOW_TH_CHx_DEFAULT;
    registerMap[LOW_TH_CH1_ADDRESS]             = LOW_TH_CHx_DEFAULT;
    registerMap[LOW_TH_CH2_ADDRESS]             = LOW_TH_CHx_DEFAULT;
    registerMap[LOW_TH_CH3_ADDRESS]             = LOW_TH_CHx_DEFAULT;
    registerMap[LOW_TH_CH4_ADDRESS]             = LOW_TH_CHx_DEFAULT;
    registerMap[LOW_TH_CH5_ADDRESS]             = LOW_TH_CHx_DEFAULT;
    registerMap[LOW_TH_CH6_ADDRESS]             = LOW_TH_CHx_DEFAULT;
    registerMap[LOW_TH_CH7_ADDRESS]             = LOW_TH_CHx_DEFAULT;

    registerMap[MAX_CH0_LSB_ADDRESS]            = MAX_CHx_LSB_DEFAULT;
    registerMap[MAX_CH1_LSB_ADDRESS]            = MAX_CHx_LSB_DEFAULT;
    registerMap[MAX_CH2_LSB_ADDRESS]            = MAX_CHx_LSB_DEFAULT;
    registerMap[MAX_CH3_LSB_ADDRESS]            = MAX_CHx_LSB_DEFAULT;
    registerMap[MAX_CH4_LSB_ADDRESS]            = MAX_CHx_LSB_DEFAULT;
    registerMap[MAX_CH5_LSB_ADDRESS]            = MAX_CHx_LSB_DEFAULT;
    registerMap[MAX_CH6_LSB_ADDRESS]            = MAX_CHx_LSB_DEFAULT;
    registerMap[MAX_CH7_LSB_ADDRESS]            = MAX_CHx_LSB_DEFAULT;

    registerMap[MAX_CH0_MSB_ADDRESS]            = MAX_CHx_MSB_DEFAULT;
    registerMap[MAX_CH1_MSB_ADDRESS]            = MAX_CHx_MSB_DEFAULT;
    registerMap[MAX_CH2_MSB_ADDRESS]            = MAX_CHx_MSB_DEFAULT;
    registerMap[MAX_CH3_MSB_ADDRESS]            = MAX_CHx_MSB_DEFAULT;
    registerMap[MAX_CH4_MSB_ADDRESS]            = MAX_CHx_MSB_DEFAULT;
    registerMap[MAX_CH5_MSB_ADDRESS]            = MAX_CHx_MSB_DEFAULT;
    registerMap[MAX_CH6_MSB_ADDRESS]            = MAX_CHx_MSB_DEFAULT;
    registerMap[MAX_CH7_MSB_ADDRESS]            = MAX_CHx_MSB_DEFAULT;

    registerMap[MIN_CH0_LSB_ADDRESS]            = MIN_CHx_LSB_DEFAULT;
    registerMap[MIN_CH1_LSB_ADDRESS]            = MIN_CHx_LSB_DEFAULT;
    registerMap[MIN_CH2_LSB_ADDRESS]            = MIN_CHx_LSB_DEFAULT;
    registerMap[MIN_CH3_LSB_ADDRESS]            = MIN_CHx_LSB_DEFAULT;
    registerMap[MIN_CH4_LSB_ADDRESS]            = MIN_CHx_LSB_DEFAULT;
    registerMap[MIN_CH5_LSB_ADDRESS]            = MIN_CHx_LSB_DEFAULT;
    registerMap[MIN_CH6_LSB_ADDRESS]            = MIN_CHx_LSB_DEFAULT;
    registerMap[MIN_CH7_LSB_ADDRESS]            = MIN_CHx_LSB_DEFAULT;

    registerMap[MIN_CH0_MSB_ADDRESS]            = MIN_CHx_MSB_DEFAULT;
    registerMap[MIN_CH1_MSB_ADDRESS]            = MIN_CHx_MSB_DEFAULT;
    registerMap[MIN_CH2_MSB_ADDRESS]            = MIN_CHx_MSB_DEFAULT;
    registerMap[MIN_CH3_MSB_ADDRESS]            = MIN_CHx_MSB_DEFAULT;
    registerMap[MIN_CH4_MSB_ADDRESS]            = MIN_CHx_MSB_DEFAULT;
    registerMap[MIN_CH5_MSB_ADDRESS]            = MIN_CHx_MSB_DEFAULT;
    registerMap[MIN_CH6_MSB_ADDRESS]            = MIN_CHx_MSB_DEFAULT;
    registerMap[MIN_CH7_MSB_ADDRESS]            = MIN_CHx_MSB_DEFAULT;

    registerMap[RECENT_CH0_LSB_ADDRESS]         = RECENT_CHx_LSB_DEFAULT;
    registerMap[RECENT_CH1_LSB_ADDRESS]         = RECENT_CHx_LSB_DEFAULT;
    registerMap[RECENT_CH2_LSB_ADDRESS]         = RECENT_CHx_LSB_DEFAULT;
    registerMap[RECENT_CH3_LSB_ADDRESS]         = RECENT_CHx_LSB_DEFAULT;
    registerMap[RECENT_CH4_LSB_ADDRESS]         = RECENT_CHx_LSB_DEFAULT;
    registerMap[RECENT_CH5_LSB_ADDRESS]         = RECENT_CHx_LSB_DEFAULT;
    registerMap[RECENT_CH6_LSB_ADDRESS]         = RECENT_CHx_LSB_DEFAULT;
    registerMap[RECENT_CH7_LSB_ADDRESS]         = RECENT_CHx_LSB_DEFAULT;

    registerMap[RECENT_CH0_MSB_ADDRESS]         = RECENT_CHx_MSB_DEFAULT;
    registerMap[RECENT_CH1_MSB_ADDRESS]         = RECENT_CHx_MSB_DEFAULT;
    registerMap[RECENT_CH2_MSB_ADDRESS]         = RECENT_CHx_MSB_DEFAULT;
    registerMap[RECENT_CH3_MSB_ADDRESS]         = RECENT_CHx_MSB_DEFAULT;
    registerMap[RECENT_CH4_MSB_ADDRESS]         = RECENT_CHx_MSB_DEFAULT;
    registerMap[RECENT_CH5_MSB_ADDRESS]         = RECENT_CHx_MSB_DEFAULT;
    registerMap[RECENT_CH6_MSB_ADDRESS]         = RECENT_CHx_MSB_DEFAULT;
    registerMap[RECENT_CH7_MSB_ADDRESS]         = RECENT_CHx_MSB_DEFAULT;

    registerMap[GPO0_TRIG_EVENT_SEL_ADDRESS]    = GPOx_TRIG_EVENT_SEL_DEFAULT;
    registerMap[GPO1_TRIG_EVENT_SEL_ADDRESS]    = GPOx_TRIG_EVENT_SEL_DEFAULT;
    registerMap[GPO2_TRIG_EVENT_SEL_ADDRESS]    = GPOx_TRIG_EVENT_SEL_DEFAULT;
    registerMap[GPO3_TRIG_EVENT_SEL_ADDRESS]    = GPOx_TRIG_EVENT_SEL_DEFAULT;
    registerMap[GPO4_TRIG_EVENT_SEL_ADDRESS]    = GPOx_TRIG_EVENT_SEL_DEFAULT;
    registerMap[GPO5_TRIG_EVENT_SEL_ADDRESS]    = GPOx_TRIG_EVENT_SEL_DEFAULT;
    registerMap[GPO6_TRIG_EVENT_SEL_ADDRESS]    = GPOx_TRIG_EVENT_SEL_DEFAULT;
    registerMap[GPO7_TRIG_EVENT_SEL_ADDRESS]    = GPOx_TRIG_EVENT_SEL_DEFAULT;

    registerMap[GPO_TRIGGER_CFG_ADDRESS]        = GPO_TRIGGER_CFG_DEFAULT;
    registerMap[GPO_VALUE_TRIG_ADDRESS]         = GPO_VALUE_TRIG_DEFAULT;
}



//*****************************************************************************
//
//! Called by readData() to convert ADC data from multiple unsigned
//! bytes into a single, signed 16-bit word.
//!
//! \fn int16_t signExtend(const uint8_t dataBytes[])
//!
//! \param dataBytes pointer to data array (big-endian).
//!
//! \return Returns the signed-extend 16-bit result.
//
//*****************************************************************************
static int16_t signExtend(const uint8_t dataBytes[])
{
    int16_t upperByte = ((int32_t)dataBytes[0] << 8);
    int16_t lowerByte = ((int32_t)dataBytes[1] << 0);

    // NOTE: This right-shift operation on signed data maintains the sign bit
    uint8_t shiftDistance = AVERAGING_ENABLED ? 0 : 4;
    return (((int16_t)(upperByte | lowerByte)) >> shiftDistance);
}


