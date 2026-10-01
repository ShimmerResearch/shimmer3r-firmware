/*
 * hal_pressure.c
 *
 *  Created on: Jul 7, 2026
 *      Author: SuhasVarna
 */

#include "hal_pressure.h"

#include "BMP3/BMP3_SensorAPI/bmp3_defs.h"
#include "BMP3/hal_bmp3.h"
#include "BMP5/BMP5_SensorAPI/bmp5_defs.h"
#include "BMP5/hal_bmp5.h"

#include "Boards/shimmer_boards.h"

static uint8_t bmp581InUse = 0;
static uint8_t pressureSensorDetected = 0;
static uint8_t identifiedByChipId = 0;

/* Determine whether a BMP390 or a BMP581 is fitted. The chip ID is read first
 * (BMP581 = 0x50 @ reg 0x01, BMP390 = 0x60 @ reg 0x00): if exactly one part
 * answers, that part is used. Trusting the chip that responds keeps the
 * firmware robust during the BMP390->BMP581 supply transition. Only when that
 * is inconclusive - neither part answers, or both do - does the SR number
 * decide which sensor should be present (DEV-818). Which of the two decided is
 * recorded in the SD header (DEV-1123), because the fallback is what a damaged
 * or missing sensor looks like. Must only be called while the sensing rail is
 * powered and SPI1 is initialised. */
void PressureSensor_detect(void)
{
  uint8_t bmp581IdOk, bmp390IdOk;

  bmp3_setup_dev();
  bmp5_setup_dev();

  bmp581IdOk = (bmp5_verify_chip_id() == BMP5_OK);
  bmp390IdOk = (bmp3_verify_chip_id() == BMP3_OK);

  if (bmp581IdOk != bmp390IdOk)
  {
    bmp581InUse = bmp581IdOk;
    identifiedByChipId = 1;
  }
  else
  {
    /* Ambiguous or no response - fall back to what the SR number says
     * should be fitted */
    bmp581InUse = ShimBrd_isBmp581PresentPerSrNumber();
    identifiedByChipId = 0;
  }
  pressureSensorDetected = 1;
}

void PressureSensor_init(void)
{
  if (!pressureSensorDetected)
  {
    PressureSensor_detect();
  }

  if (bmp581InUse)
  {
    bmp5_driver_init();
  }
  else
  {
    bmp3_driver_init();
  }
}

int8_t PressureSensor_configure(float shimmerSamplingFreq, uint8_t overSamplingRatio)
{
  if (bmp581InUse)
  {
    return bmp5_configure(shimmerSamplingFreq, overSamplingRatio);
  }
  return bmp3_configure(shimmerSamplingFreq, overSamplingRatio);
}

HAL_StatusTypeDef PressureSensor_getDataDma(uint8_t *buf)
{
  if (bmp581InUse)
  {
    return bmp5_pressure_temperature_get(buf);
  }
  return bmp3_pressure_temperature_get(buf);
}

bool PressureSensor_isDrdyIntEnabled(void)
{
  if (bmp581InUse)
  {
    return bmp5_is_drdy_int_enabled();
  }
  return bmp3_is_drdy_int_enabled();
}

void PressureSensor_clearDrdyInt(void)
{
  if (bmp581InUse)
  {
    bmp5_read_int_status();
  }
  else
  {
    bmp3_read_sensor_status();
  }
}

void PressureSensor_selectDevice(void)
{
  /* The BMP390 and BMP581 share the same chip-select line */
  bmp3_selectDevice();
}

void PressureSensor_unselectDevice(void)
{
  /* The BMP390 and BMP581 share the same chip-select line */
  bmp3_unselectDevice();
}

uint8_t PressureSensor_selfTest(void)
{
  if (bmp581InUse)
  {
    return bmp5_self_test();
  }
  return bmp3_self_test();
}

uint8_t isBmp390InUse(void)
{
  return !bmp581InUse;
}

uint8_t isBmp581InUse(void)
{
  return bmp581InUse;
}

/* Non-zero when PressureSensor_detect() chose the sensor because exactly one
 * chip answered its ID check; zero when it fell back to the SR number. */
uint8_t PressureSensor_wasIdentifiedByChipId(void)
{
  return identifiedByChipId;
}
