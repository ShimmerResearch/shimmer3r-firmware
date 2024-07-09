/*
 * shimmer_boards.c
 *
 *  Created on: 2 May 2023
 *      Author: MarkNolan
 */

#include "shimmer_boards.h"
#include <stdio.h>
#include <string.h>

daughter_card_id_page daughterCardIdPage;
char daughtCardIdStr[26];
uint8_t eepromIsPresent;

void setDaugherCardIdPage(uint8_t *pagePtr)
{
  memcpy(daughterCardIdPage.raw, pagePtr, sizeof(daughterCardIdPage.raw));
}

uint8_t isAds1292Present(void)
{
  return isAds1292PresentForSrId(daughterCardIdPage.expansion_brd.exp_brd_id);
}

uint8_t isAds1292PresentForSrId(uint8_t srId)
{
  return (srId == EXP_BRD_EXG) || (srId == EXP_BRD_EXG_UNIFIED)
      || (srId == SHIMMER_ECG_MD) || (srId == SHIMMER4_SDK);
}

void parseDaughterCardId(uint8_t srId)
{
  memset(daughtCardIdStr, 0x00, sizeof(daughtCardIdStr));

  switch (srId)
  {
  case SHIMMER3_IMU: sprintf(daughtCardIdStr, "Shimmer3 IMU"); break;
  case EXP_BRD_GSR:
  case EXP_BRD_GSR_UNIFIED: sprintf(daughtCardIdStr, "Shimmer3 GSR+"); break;
  case EXP_BRD_EXG:
  case EXP_BRD_EXG_UNIFIED: sprintf(daughtCardIdStr, "Shimmer3 ExG"); break;
  case EXP_BRD_BR_AMP:
  case EXP_BRD_BR_AMP_UNIFIED:
    sprintf(daughtCardIdStr, "Shimmer3 Bridge Amplifier");
    break;
  case EXP_BRD_PROTO3_MINI:
    sprintf(daughtCardIdStr, "Shimmer3 Proto3 Mini");
    break;
  case EXP_BRD_PROTO3_DELUXE:
    sprintf(daughtCardIdStr, "Shimmer3 Proto3 Deluxe");
    break;
  case EXP_BRD_ADXL377_ACCEL_200G:
    sprintf(daughtCardIdStr, "Shimmer3 200G Accel");
    break;
  case EXP_BRD_H3LIS331DL_ACCEL_HIGH_G:
    sprintf(daughtCardIdStr, "Shimmer3 100G Accel");
    break;
  case SHIMMER4_SDK: sprintf(daughtCardIdStr, "Shimmer4 SDK"); break;
  case SHIMMER_ECG_MD: sprintf(daughtCardIdStr, "Shimmer3 ECG MD"); break;
  default: sprintf(daughtCardIdStr, "SR%d", srId); break;
  }
}

shimmer_expansion_brd *getDaughtCardId(void)
{
  return &daughterCardIdPage.expansion_brd;
}

char *getDaughtCardIdStrPtr(void)
{
  return &daughtCardIdStr[0];
}

void setEepromIsPresent(uint8_t eeprom_is_preset)
{
  eepromIsPresent = eeprom_is_preset;
}

uint8_t isEepromIsPresent(void)
{
  return eepromIsPresent;
}

shimmer_expansion_brd *getDaughtCardIdPtr(void)
{
  return &daughterCardIdPage.expansion_brd;
}
