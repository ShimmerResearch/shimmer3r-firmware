/*
 * CYW20820.h
 *
 *  Created on: Oct 23, 2023
 *      Author: MarkNolan
 */

#ifndef SRC_CYW20820_H_
#define SRC_CYW20820_H_

void btInit(void);
void btSetCommands(void);

void printHex(uint8_t *data, uint8_t bytes, uint8_t reverse, char separator);

#endif /* SRC_CYW20820_H_ */
