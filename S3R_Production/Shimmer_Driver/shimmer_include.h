/*
 * Shimmer Include Header
 *
 * This header file includes all necessary headers for the Shimmer platform.
 * It is designed to be included in various source files to provide access
 * to the Shimmer API and hardware definitions.
 *
*/
#ifndef SHIMMER_INCLUDE_H_
#define SHIMMER_INCLUDE_H_

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log_and_stream_definitions.h"
#include "log_and_stream_externs.h"
#include "log_and_stream_includes.h"

#include "hal_Power.h"
#include "hal_adc.h"
#include "shimmer_definitions.h"
#if defined(SHIMMER4_SDK)
#include <STC3100/STC3100.h>
#endif
#if defined(SHIMMER3R)
#include "../Shimmer_Driver/BMP3/BMP3_SensorAPI/bmp3_defs.h"
#include "../Shimmer_Driver/BMP3/hal_bmp3.h"
#include "ADXL371/hal_adxl371.h"
#include "LIS2DW12/hal_lis2dw12.h"
#include "LIS2DW12/lis2dw12-pid/lis2dw12_reg.h"
#include <ADS7028_38/ads7028_38.h>
#include <CYW20820/CYW20820.h>
#include <CYW20820/hal_CYW20820.h>
#include <LIS2DW12/lis2dw12.h>
#include <LIS2MDL/hal_lis2mdl.h>
#include <LIS2MDL/lis2mdl-pid/lis2mdl_reg.h>
#include <LIS2MDL/lis2mdl.h>
#include <LIS3MDL/hal_lis3mdl.h>
#include <LIS3MDL/lis3mdl-pid/lis3mdl_reg.h>
#include <LIS3MDL/lis3mdl.h>
#include <LSM6DSV/hal_lsm6dsv.h>
#include <LSM6DSV/lsm6dsv-pid/lsm6dsv_reg.h>
#include <LSM6DSV/lsm6dsv.h>
#elif defined(SHIMMER4_SDK)
#include "BMP280_driver\bmp280.h"
#include "RN42.h"
#include "bmp180.h"
#include <LSM303DLHC/lsm303dlhc.h>
#include <MPU9250/mpu9250.h>
#endif
#include "CAT24C16/CAT24C16.h"
#include "CAT24C16/hal_CAT24C16.h"
#include "EXG/ads1292.h"
#include "EXG/exg.h"
#include "GSRTestRig/gsrTestRig.h"
#include "hal_Board.h"
#include "hal_CRC.h"
#include "hal_FactoryTest.h"
#include "hal_Infomem.h"
#include "hal_bootloader.h"

#include "adc.h"
#if defined(SHIMMER4_SDK)
#include "can.h"
#include "dma.h"
#endif
#if USE_FATFS
#include "fatfs.h"
#endif
#include "i2c.h"
//#include "iwdg.h"
#include "rng.h"
#include "rtc.h"
#include "sdmmc.h"
#include "spi.h"
//#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
//#include "wwdg.h"
#include "crc.h"
#include "gpio.h"
#include "mdf.h"

#include "swo.h"

#endif
