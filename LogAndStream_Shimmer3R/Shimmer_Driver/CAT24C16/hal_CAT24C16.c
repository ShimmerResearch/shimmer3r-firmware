/*
 * hal_CAT24C16.c
 *
 * Shimmer3R's side of the CAT24C16 EEPROM: power, the I2C bus, and the bus
 * callbacks the shared driver (log-and-stream-common EEPROM/cat24c16.c) reaches
 * the part through. Two contexts: the expansion board's part on I2C1, and the
 * factory test rig's on I2C4 (S3R_TEST_0024).
 *
 *  Created on: Nov 7, 2024
 *      Author: MarkNolan
 */

#include "hal_CAT24C16.h"

#include "EEPROM/cat24c16.h"
#include "hal_Board.h"
#include "shimmer_include.h"

cat24c16dev_ctx_t cat24c16_ctx;
cat24c16dev_ctx_t cat24c16_alt_ctx;

static int32_t platform_write(void *handle, uint8_t devAddr, uint8_t *buf, uint16_t len);
static int32_t
platform_read(void *handle, uint8_t devAddr, uint8_t wordAddr, uint8_t *outBuffer, uint16_t len);
static void platform_delay(uint32_t ms);

#if !IS_CONNECTED_EEPROM
uint8_t eepromContents[512] = { 0 };
#endif

static void eepromCtxInit(cat24c16dev_ctx_t *ctx, I2C_HandleTypeDef *hi2c)
{
  ctx->write = platform_write;
  ctx->read = platform_read;
  ctx->mdelay = platform_delay;
  ctx->handle = hi2c;
}

void eepromInit(I2C_HandleTypeDef *hi2c)
{
  eepromCtxInit(&cat24c16_ctx, hi2c);
}

void altEepromInit(I2C_HandleTypeDef *hi2c)
{
  eepromCtxInit(&cat24c16_alt_ctx, hi2c);
}

void eepromPowerOn(void)
{
  Board_enableSensingPower(SENSE_PWR_EEPROM, 1);
  MX_I2C1_Init();
}

void eepromPowerOff(void)
{
  HAL_Delay(5); //5ms to ensure no writes pending
  I2C1_DeInit();
  Board_enableSensingPower(SENSE_PWR_EEPROM, 0);
}

uint8_t eepromTest(void)
{
  return CAT24C16_test(&cat24c16_ctx);
}

uint8_t altEepromTest(void)
{
  return CAT24C16_test(&cat24c16_alt_ctx);
}

void eepromRead(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf)
{
  eepromReadWrite(dataAddr, dataSize, dataBuf, EEPROM_READ);
}

void eepromWrite(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf)
{
  eepromReadWrite(dataAddr, dataSize, dataBuf, EEPROM_WRITE);
}

void eepromReadWrite(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf, enum EEPROM_RW eepromRW)
{
#if IS_CONNECTED_EEPROM
  eepromPowerOn();

  //EEPROM needs to be updated with latest bt baud rate, configure here
  if (eepromRW == EEPROM_READ)
  {
    CAT24C16_read(&cat24c16_ctx, dataAddr, dataSize, dataBuf);
  }
  else
  {
    CAT24C16_write(&cat24c16_ctx, dataAddr, dataSize, dataBuf);
  }

  //Wind down EEPROM and required timing peripherals
  eepromPowerOff();
#else
  if (eepromRW == EEPROM_READ)
  {
    memcpy(dataBuf, &eepromContents[dataAddr], dataSize);
  }
  else
  {
    memcpy(&eepromContents[dataAddr], dataBuf, dataSize);
  }
#endif
}

#if !IS_CONNECTED_EEPROM
void setMockExpansionBrdDetails(void)
{
  eepromContents[0] = EXP_BRD_PROTO3_DELUXE; //0xFF
  eepromContents[1] = 9;                     //0xFF
  eepromContents[2] = 0;                     //0xFF
}
#endif

static int32_t platform_write(void *handle, uint8_t devAddr, uint8_t *buf, uint16_t len)
{
  //The HAL takes the address already shifted, with the R/W bit clear
  return HAL_I2C_Master_Transmit(handle, (uint16_t) (devAddr << 1), buf, len, CAT24C16_TIMEOUT);
}

static int32_t
platform_read(void *handle, uint8_t devAddr, uint8_t wordAddr, uint8_t *outBuffer, uint16_t len)
{
  HAL_StatusTypeDef cat24c16_result;

  cat24c16_result = HAL_I2C_Master_Transmit(
      handle, (uint16_t) (devAddr << 1), &wordAddr, 1, CAT24C16_TIMEOUT);
  if (cat24c16_result != HAL_OK)
  {
    return cat24c16_result;
  }
  return HAL_I2C_Master_Receive(
      handle, (uint16_t) (devAddr << 1), outBuffer, len, CAT24C16_TIMEOUT);
}

static void platform_delay(uint32_t ms)
{
  HAL_Delay(ms);
  __NOP();
}
