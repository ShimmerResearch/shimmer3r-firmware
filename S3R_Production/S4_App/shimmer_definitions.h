//s4.h

#ifndef SHIMMER_DEFINITIONS_H
#define SHIMMER_DEFINITIONS_H

#include <stdint.h>

#if defined(SHIMMER3R)
#define DEVICE_VER    10
#define FW_IDENTIFIER 3  //12 is the firmware for shimmer4sdk
#define FW_VER_MAJOR  1  //Major version number: 0-65535
#define FW_VER_MINOR  0  //Minor version number: 0-255
#define FW_VER_REL    18 //internal version number: 0-255
#elif defined(SHIMMER4_SDK)
#define DEVICE_VER 58 //HARDWARE VERSION: SR_58 for Shimmer4SDK
#define FW_IDENTIFIER \
  12 //12 is the firmware for shimmer4sdk, with LogAndStream in side
#define FW_VER_MAJOR 0  //Major version number: 0-65535
#define FW_VER_MINOR 0  //Minor version number: 0-255
#define FW_VER_REL   23 //internal version number: 0-255
#endif

#define IS_CONNECTED_EEPROM 1

#define USE_DEFAULT_SENSOR  0
#define RTC_FAST \
  0 //need to increase rtc alarm interrupt priority before enabling this option
#define FULL_TEST_MODE        0
#define USE_FREERTOS          0
#define SENS_CLK_RTC0TIM1     0

#define USE_I2C_VBATT_REPORT  1
#define USE_VBATT_ALWAYS      0
#define HAL_TEST_INFOMEM      0
#define USE_BMPX80            2 //x=1 for 180, x=2 for 280
#define NUM_SDWRBUF           64
#define USE_USBX              0
#define SHIMMER_ENABLE_PRINTF 1

#ifdef SR48_6_0
#define SR48_6_0_PATCH_DOCK_DETECT 1
#define SR48_6_0_PATCH_VBUS_SENSE  1
#else
#define SR48_6_0_PATCH_DOCK_DETECT 0
#define SR48_6_0_PATCH_VBUS_SENSE  0
#endif

#ifdef SHIMMER_ENABLE_PRINTF
#define SHIMMER_PRINTF(...) printf(__VA_ARGS__)
#else /* SHIMMER_ENABLE_PRINTF */
#define SHIMMER_PRINTF(...)
#endif /* SHIMMER_ENABLE_PRINTF */

//typedef uint8_t bool;
//#define TRUE 1
//#define FALSE 0

//typedef uint8_t error_t;
//#define SUCCESS 1
//#define FAIL 0

//typedef enum{//bt
//   UART_BT_STAT_IDLE = 0,
//   UART_BT_STAT_CONFIG = 1
//}UART_BT_STATUS;

#endif //S4_H
