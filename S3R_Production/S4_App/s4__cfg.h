// #include "s4__cfg.h"

#ifndef SHIMMER4_SDK_CONFIG_H
#define SHIMMER4_SDK_CONFIG_H

#define USE_DEFAULT_SENSOR    1
#define RTC_FAST              0 // need to increase rtc alarm interrupt priority before enabling this option
#define USE_8BYTES_INIT_TS    0      
#define TEST_UNDOCKED         0
#define FULL_TEST_MODE        0
#define USE_FREERTOS          0
#define SENS_CLK_RTC0TIM1     0


#define USE_DEFAULT_LED       1
#define USE_I2C_VBATT_REPORT  1
#define SKIP_50MS             1
#define USE_VBATT_ALWAYS      1
#define HAL_TEST_INFOMEM      1
#define USE_BT                1
#define USE_SD                1
#define USE_BMPX80            2//x=1 for 180, x=2 for 280
#define NUM_SDWRBUF           64

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "s4_led.h"
#include "s4_calc.h"
#include "s4_sensing.h"
#include "s4_ram.h"
#include "s4_calib.h"
#include "s4_adc.h"
#include "hal_Power.h"
#include "STC3100.h"
#include "gsr.h"
#include "RN42.h"
#include "lsm303dlhc.h"
#include "mpu9250.h"
#include "s4_taskList.h"
#include "CAT24C16.h"
#include "hal_Board.h"
#include "hal_Infomem.h"
#include "bmp180.h"
#include "BMP280_driver\bmp280.h"
#include "EXG\exg.h"

#include "adc.h"
//#include "can.h"
//#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
// #include "iwdg.h"
#include "rng.h"
#include "rtc.h"
#include "sdmmc.h"
#include "spi.h"
//#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
// #include "wwdg.h"
#include "gpio.h"

#endif
