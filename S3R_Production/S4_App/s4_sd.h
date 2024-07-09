/*
 * s4sd.h
 *
 *  Created on: Mar 21, 2024
 *      Author: MarkNolan
 */

#ifndef S4_SD_H_
#define S4_SD_H_

#include <stdint.h>

#define CheckSdInslot SD_insertedCheck

void SD_init(void);
uint8_t SD_test(void);
uint8_t SD_test_alternative(void);
void SD_setShimmerName(void);
void SD_setExpIdName(void);
void SD_setCfgTime(void);
void SD_setFileName(void);
void SD_infomem2Names(void);
uint8_t SD_setBasedir(void);
uint8_t SD_makeBasedir(void);
void SD_makeFileName(char *name_buf);
void SD_fileInit(void);
void SD_close(void);
void SD_writeToBuff(uint8_t *buf, uint16_t len);
void SD_writeToCard(void);
//void SD_config2SdHead(void);
void SD_mount(uint8_t val);
void UpdateSdConfig(void);
void ParseConfig(void);

void ItoaNo0(uint64_t num, uint8_t *buf, uint8_t max_len);
uint8_t isFileStatusOk(void);
uint8_t isSdInfoSyncDelayed(void);
void setSdInfoSyncDelayed(uint8_t state);
uint8_t *getConfigTimeTextPtr(void);
uint8_t *getFileNamePtr(void);
uint8_t *getShimmerNamePtr(void);
uint8_t *getExpIdPtr(void);

#endif /* S4_SD_H_ */
