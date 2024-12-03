/*
 * hal_CAT24C16.h
 *
 *  Created on: Nov 7, 2024
 *      Author: MarkNolan
 */

#ifndef HAL_CAT24C16_H_
#define HAL_CAT24C16_H_

#include <stdint.h>

#include "stm32u5xx_hal.h"

#define CAT24C16_TIMEOUT 1000

enum EEPROM_RW
{
  EEPROM_READ = 0,
  EEPROM_WRITE = 1,
};

//pass over the i2c handler pointer
void eepromInit(I2C_HandleTypeDef *hi2c);

void altEepromInit(I2C_HandleTypeDef *hi2c);

//power on the CAT24C16 chip
void eepromPowerOn(void);

//power off the CAT24C16 chip
void eepromPowerOff(void);

uint8_t eepromTest(void);

void altEepromPowerOn(void);
void altEepromPowerOff(void);
uint8_t altEepromTest(void);

void eepromRead(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf);
void eepromWrite(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf);
void eepromReadWrite(uint16_t dataAddr, uint16_t dataSize, uint8_t *dataBuf, enum EEPROM_RW eepromRW);

#if !IS_CONNECTED_EEPROM
void setMockExpansionBrdDetails(void);
#endif

#endif /* HAL_CAT24C16_H_ */
