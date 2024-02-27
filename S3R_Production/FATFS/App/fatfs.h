/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.h
  * @brief  Header for fatfs applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __fatfs_H
#define __fatfs_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h" /* defines SD_Driver as external */

/* USER CODE BEGIN Includes */

#include "s4.h"
#include "s4__cfg.h"

/* USER CODE END Includes */

extern uint8_t retSD; /* Return value for SD */
extern char SDPath[4]; /* SD logical drive path */
extern FATFS SDFatFS; /* File system object for SD logical drive */
extern FIL SDFile; /* File object for SD */

void MX_FATFS_Init(void);

/* USER CODE BEGIN Prototypes */

void SD_init(void);
uint8_t SD_test(void);
uint8_t SD_test_alternative(void);
void SD_insertedCheck(void);
void SD_fileInit(void);
void SD_close(void);
void SD_makeFileName(char* name_buf);
void SD_writeToBuff(uint8_t *buf, uint16_t len);
void SD_writeToCard(void);
//void SD_config2SdHead(void);
void SD_mount(uint8_t val);
void SD_insertedCheck(void);

/* USER CODE END Prototypes */
#ifdef __cplusplus
}
#endif
#endif /*__fatfs_H */
