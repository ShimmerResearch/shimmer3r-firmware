/*
 * shimmer_boards.h
 *
 *  Created on: 2 May 2023
 *      Author: MarkNolan
 */

#ifndef SHIMMER3_COMMON_SOURCE_SHIMMER_BOARDS_SHIMMER_BOARDS_H_
#define SHIMMER3_COMMON_SOURCE_SHIMMER_BOARDS_SHIMMER_BOARDS_H_

#include <stdint.h>

#if !IS_CONNECTED_EEPROM & defined(SHIMMER3R)
#define EXP_BRD_ID_MAJOR      31 //0xFF//47
#define EXP_BRD_ID_MINOR      9  //0xFF//1
#define EXP_BRD_ID_INTERNAL   0  //0xFF//0
#endif

enum SR_BOARD_CODES
{
    EXP_BRD_BR_AMP              = 8,
    EXP_BRD_GSR                 = 14,
    SHIMMER3_IMU                = 31,
    EXP_BRD_PROTO3_MINI         = 36,
    EXP_BRD_EXG                 = 37,
    EXP_BRD_PROTO3_DELUXE       = 38,
    EXP_BRD_ADXL377_ACCEL_200G  = 44,
    EXP_BRD_EXG_UNIFIED         = 47,
    EXP_BRD_GSR_UNIFIED         = 48,
    EXP_BRD_BR_AMP_UNIFIED      = 49,
    EXP_BRD_H3LIS331DL_ACCEL_HIGH_G     = 55,
    SHIMMER_GQ_LR               = 56,
    SHIMMER_GQ_SR               = 57,
    SHIMMER4_SDK                = 58,
    SHIMMER_ECG_MD              = 59
};

typedef struct
{
  uint8_t exp_brd_id_major;
  uint8_t exp_brd_id_minor;
  uint8_t exp_brd_id_internal;
} shimmer_expansion_brd;

typedef union
{
  uint8_t raw[16];
  struct __attribute__((packed))
  {
    shimmer_expansion_brd expansion_brd;
    uint8_t padding[13];
  };
} daughter_card_id_page;

void setDaugherCardIdPage(uint8_t *pagePtr);
uint8_t isAds1292Present(void);
uint8_t isAds1292PresentForSrId(uint8_t srId);
void parseDaughterCardId(uint8_t srId);
char* getDaughtCardIdStrPtr(void);

void setEepromIsPresent(uint8_t eeprom_is_preset);
uint8_t isEepromIsPresent(void);

#endif /* SHIMMER3_COMMON_SOURCE_SHIMMER_BOARDS_SHIMMER_BOARDS_H_ */
