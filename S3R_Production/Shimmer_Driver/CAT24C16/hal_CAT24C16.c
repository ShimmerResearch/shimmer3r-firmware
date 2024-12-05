/*
 * hal_CAT24C16.c
 *
 *  Created on: Nov 7, 2024
 *      Author: MarkNolan
 */

#include "hal_CAT24C16.h"

#include "CAT24C16.h"
#include "hal_Board.h"
#include <shimmer_include.h>

#include <stdlib.h>

cat24c16dev_ctx_t cat24c16_ctx;
cat24c16dev_ctx_t cat24c16_alt_ctx;

static int32_t platform_write(void *handle, uint16_t address, uint16_t length, uint8_t *data);
static int32_t
platform_read(void *handle, uint16_t address, uint16_t length, uint8_t *outBuffer);
static void platform_delay(uint32_t ms);
static uint32_t get_sys_tick(void);

#if !IS_CONNECTED_EEPROM
uint8_t eepromContents[512] = { 0 };
#endif

void eepromInit(I2C_HandleTypeDef *hi2c)
{
  cat24c16_ctx.write = platform_write;
  cat24c16_ctx.read = platform_read;
  cat24c16_ctx.mdelay = platform_delay;
  cat24c16_ctx.sys_tick = get_sys_tick;
  cat24c16_ctx.handle = hi2c;
}

void altEepromInit(I2C_HandleTypeDef *hi2c)
{
  cat24c16_alt_ctx.write = platform_write;
  cat24c16_alt_ctx.read = platform_read;
  cat24c16_alt_ctx.mdelay = platform_delay;
  cat24c16_alt_ctx.sys_tick = get_sys_tick;
  cat24c16_alt_ctx.handle = hi2c;
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

void altEepromPowerOn(void)
{
  Board_SW_EXP_BRD_POWER(1);
  Board_SW_I2C4_ON_PPG(1);
  MX_I2C4_Init();
  HAL_Delay(2); //2ms as per Shimmer3 code
}

void altEepromPowerOff(void)
{
  HAL_Delay(5); //5ms to ensure no writes pending
  I2C4_DeInit();
  Board_SW_EXP_BRD_POWER(0);
  Board_SW_I2C4_ON_PPG(0);
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

static int32_t platform_write(void *handle, uint16_t address, uint16_t length, uint8_t *data)
{
  return HAL_I2C_Master_Transmit(handle, address, data, length, CAT24C16_TIMEOUT);
}

static int32_t platform_read(void *handle, uint16_t address, uint16_t length, uint8_t *outBuffer)
{
  uint8_t addr_hi = CAT24C16_ADDR | (address >> 8);
  uint8_t addr_lo = address & 0xff;

  HAL_StatusTypeDef cat24c16_result;
  cat24c16_result = HAL_I2C_Master_Transmit(handle, addr_hi << 1, &addr_lo, 1, CAT24C16_TIMEOUT);
  if (cat24c16_result != HAL_OK)
  {
    return cat24c16_result;
  }
  cat24c16_result = HAL_I2C_Master_Receive(
      handle, addr_hi << 1, outBuffer, length, CAT24C16_TIMEOUT);
  return cat24c16_result;
}

static void platform_delay(uint32_t ms)
{
  HAL_Delay(ms);
  __NOP();
}

static uint32_t get_sys_tick(void)
{
  return SysTick->VAL;
}
