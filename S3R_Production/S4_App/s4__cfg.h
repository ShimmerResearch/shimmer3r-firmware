//#include "s4__cfg.h"

#ifndef SHIMMER4_SDK_CONFIG_H
#define SHIMMER4_SDK_CONFIG_H

#define IS_CONNECTED_EEPROM 0

#define USE_DEFAULT_SENSOR  0
#define RTC_FAST \
  0 //need to increase rtc alarm interrupt priority before enabling this option
#define USE_8BYTES_INIT_TS    0
#define TEST_UNDOCKED         0
#define FULL_TEST_MODE        0
#define USE_FREERTOS          0
#define SENS_CLK_RTC0TIM1     0

#define USE_DEFAULT_LED       1
#define USE_I2C_VBATT_REPORT  1
#define SKIP_50MS             1
#define USE_VBATT_ALWAYS      0
#define HAL_TEST_INFOMEM      0
#define USE_BT                1
#define USE_SD                1
#define USE_BMPX80            2 //x=1 for 180, x=2 for 280
#define NUM_SDWRBUF           64
#define USE_FATFS             1
#define USE_USBX              0
#define SHIMMER_ENABLE_PRINTF 1

#ifdef SHIMMER_ENABLE_PRINTF
#define SHIMMER_PRINTF(...) printf(__VA_ARGS__)
#else /* SHIMMER_ENABLE_PRINTF */
#define SHIMMER_PRINTF(...)
#endif /* SHIMMER_ENABLE_PRINTF */

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
#include "../Shimmer_Driver/ADXL371/adxl371_spi.h"
#include "../Shimmer_Driver/BMP3/hal_bmp3.h"
#include "../Shimmer_Driver/LIS2DW12/lis2dw12.h"
#include "../Shimmer_Driver/LIS2MDL/lis2mdl.h"
#include "../Shimmer_Driver/LIS3MDL/lis3mdl.h"
#include "../Shimmer_Driver/LSM6DSV/lsm6dsv.h"
#endif
#include "BMP280_driver\bmp280.h"
#include "CAT24C16.h"
#include "EXG\exg.h"
#include "bmp180.h"
#include "hal_Board.h"
#include "hal_CRC.h"
#include "hal_Infomem.h"
#include "hal_FactoryTest.h"
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

#include "swo.h"

#endif
