/*
 * s4sd.h
 *
 *  Created on: Mar 21, 2024
 *      Author: MarkNolan
 */

#ifndef S4_SD_H_
#define S4_SD_H_

#include <stdint.h>

void SD_init(void);
uint8_t SD_test(void);
uint8_t SD_test_alternative(void);
void SD_insertedCheck(void);
void SD_fileInit(void);
void SD_close(void);
void SD_makeFileName(char *name_buf);
void SD_writeToBuff(uint8_t *buf, uint16_t len);
void SD_writeToCard(void);
//void SD_config2SdHead(void);
void SD_mount(uint8_t val);
void SD_insertedCheck(void);

#endif /* S4_SD_H_ */
