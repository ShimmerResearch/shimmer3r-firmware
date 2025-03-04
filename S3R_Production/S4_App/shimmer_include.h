//#include "s4__cfg.h"

#ifndef SHIMMER_INCLUDE_H_
#define SHIMMER_INCLUDE_H_

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal_Power.h"
#include "s4_adc.h"
#include "s4_calc.h"
#include "s4_calib.h"
#include "s4_led.h"
#include "s4_ram.h"
#include "s4_sd.h"
#include "s4_sensing.h"
#include "shimmer_definitions.h"
#if defined(SHIMMER4_SDK)
#include "STC3100.h"
#endif
#include "../Shimmer_Driver/shimmer_boards/shimmer_boards.h"
#include "gsr.h"
#if defined(SHIMMER3R)
#include "../Shimmer_Driver/Bluetooth/CYW20820.h"
#include "../Shimmer_Driver/Bluetooth/EZ-Serial/handlers.h"
#include "../Shimmer_Driver/Bluetooth/sd_sync.h"
#include "../Shimmer_Driver/Bluetooth/shimmer_bt_comms.h"
#elif defined(SHIMMER4_SDK)
#include "RN42.h"
#endif
#include "../Shimmer_Driver/dock_comms/shimmer_dock_comms.h"
#if defined(SHIMMER3R)
#include "../Shimmer_Driver/ADXL371/adxl371.h"
#include "../Shimmer_Driver/BMP3/BMP3_SensorAPI/bmp3_defs.h"
#include "../Shimmer_Driver/BMP3/hal_bmp3.h"
#include "ADXL371/hal_adxl371.h"
#include "LIS2DW12/hal_lis2dw12.h"
#include "LIS2DW12/lis2dw12-pid/lis2dw12_reg.h"
#include "battery.h"
#include "log_and_stream_definitions.h"
#include "log_and_stream_externs.h"
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
#include "ADS7028/ads7028.h"
#include "ADS7028/hal_ads7028.h"
#endif
#include "BMP280_driver\bmp280.h"
#include "CAT24C16/CAT24C16.h"
#include "CAT24C16/hal_CAT24C16.h"
#include "EXG\exg.h"
#include "GSRTestRig/gsrTestRig.h"
#include "bmp180.h"
#include "hal_Board.h"
#include "hal_CRC.h"
#include "hal_FactoryTest.h"
#include "hal_Infomem.h"
#include "lsm303dlhc.h"
#include "mpu9250.h"
#include "s4_taskList.h"

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
