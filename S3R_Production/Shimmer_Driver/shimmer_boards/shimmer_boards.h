/*
 * shimmer_boards.h
 *
 *  Created on: 2 May 2023
 *      Author: MarkNolan
 */

#ifndef SHIMMER3_COMMON_SOURCE_SHIMMER_BOARDS_SHIMMER_BOARDS_H_
#define SHIMMER3_COMMON_SOURCE_SHIMMER_BOARDS_SHIMMER_BOARDS_H_

#include <stdint.h>

enum SR_HW_IDS
{
  HW_ID_SHIMMER3 = 3,
  HW_ID_SHIMMER3R = 10
};

enum SR_BOARD_CODES
{
  EXP_BRD_BR_AMP = 8,
  EXP_BRD_GSR = 14,
  SHIMMER3_IMU = 31,
  EXP_BRD_PROTO3_MINI = 36,
  EXP_BRD_EXG = 37,
  EXP_BRD_PROTO3_DELUXE = 38,
  EXP_BRD_ADXL377_ACCEL_200G = 44,
  EXP_BRD_EXG_UNIFIED = 47,
  EXP_BRD_GSR_UNIFIED = 48,
  EXP_BRD_BR_AMP_UNIFIED = 49,
  EXP_BRD_H3LIS331DL_ACCEL_HIGH_G = 55,
  SHIMMER_GQ_LR = 56,
  SHIMMER_GQ_SR = 57,
  SHIMMER4_SDK = 58,
  SHIMMER_ECG_MD = 59
};

enum WR_ACCEL_AND_MAG_IN_USE
{
  WR_ACCEL_AND_MAG_NONE_IN_USE,
  WR_ACCEL_AND_MAG_LSM303DLHC_IN_USE,
  WR_ACCEL_AND_MAG_LSM303AHTR_IN_USE,
  WR_ACCEL_AND_MAG_ICM20948_IN_USE
};

enum GYRO_IN_USE
{
  GYRO_NONE_IN_USE,
  GYRO_MPU9X50_IN_USE,
  GYRO_ICM20948_IN_USE
};

typedef struct
{
  uint8_t exp_brd_id;
  uint8_t exp_brd_rev;
  uint8_t exp_brd_special_rev;
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

void setHwId(uint8_t hwIdToSet);
void setDaugherCardIdPage(uint8_t *pagePtr);
uint8_t isAds1292Present(void);
uint8_t isAds1292PresentForSrId(uint8_t srId);
#if defined(SHIMMER3)
uint8_t isRn4678PresentAndCmdModeSupport(uint8_t srId, uint8_t srRev, uint8_t srRevSpecial);
uint8_t isSubstitutionNeededForWrAccel(uint8_t srId, uint8_t srRevMajor, uint8_t srRevMinor);
uint8_t are2ndGenImuSensorsPresent(void);
uint8_t are2ndGenSensorsPresentAndUnknownBoard(uint8_t srId);
uint8_t areGsrControlsPinsReversed(uint8_t srId, uint8_t srRevMajor, uint8_t srRevMinor);
#endif
void parseDaughterCardId(uint8_t srId);
shimmer_expansion_brd *getDaughtCardId(void);
char *getDaughtCardIdStrPtr(void);
uint8_t isDaughterCardIdSet(void);

#if defined(SHIMMER3)
void setWrAccelAndMagInUse(uint8_t wr_accel_and_mag_in_use);
uint8_t isWrAccelInUseLsm303dlhc(void);
uint8_t isWrAccelInUseLsm303ahtr(void);
uint8_t isWrAccelInUseIcm20948(void);

void setGyroInUse(uint8_t gyro_in_use);
uint8_t isGyroInUseMpu9x50(void);
uint8_t isGyroInUseIcm20948(void);
#endif

void setEepromIsPresent(uint8_t eeprom_is_preset);
uint8_t isEepromIsPresent(void);

#if defined(SHIMMER3)
uint8_t isLnAccelKxtc9_2050Present(void);
#endif

uint8_t isBmp180InUse(void);
uint8_t isBmp280InUse(void);

#if defined(SHIMMER3R)
uint8_t isAdxl371Present(void);
uint8_t isI2c4Supported(void);
uint8_t isBoardSr48_6_0(void);
uint8_t isBoardSrNumber(uint8_t exp_brd_id, uint8_t exp_brd_rev, uint8_t exp_brd_special_rev);
#endif

#endif /* SHIMMER3_COMMON_SOURCE_SHIMMER_BOARDS_SHIMMER_BOARDS_H_ */
