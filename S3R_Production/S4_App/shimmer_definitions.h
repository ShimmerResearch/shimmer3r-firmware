//s4.h

#ifndef SHIMMER_DEFINITIONS_H
#define SHIMMER_DEFINITIONS_H

#include <stdint.h>

#define OLD_CONSENSYS_SUPPORT 1

#if defined(SHIMMER3R)
#if OLD_CONSENSYS_SUPPORT
#define DEVICE_VER 3 //For older Consensys support
#else
#define DEVICE_VER 10
#endif
#define FW_IDENTIFIER 3 //12 is the firmware for shimmer4sdk
#define FW_VER_MAJOR  1 //Major version number: 0-65535
#define FW_VER_MINOR  0 //Minor version number: 0-255
#define FW_VER_REL    0 //internal version number: 0-255
#elif defined(SHIMMER4_SDK)
#define DEVICE_VER 58 //HARDWARE VERSION: SR_58 for Shimmer4SDK
#define FW_IDENTIFIER \
  12 //12 is the firmware for shimmer4sdk, with LogAndStream in side
#define FW_VER_MAJOR 0  //Major version number: 0-65535
#define FW_VER_MINOR 0  //Minor version number: 0-255
#define FW_VER_REL   23 //internal version number: 0-255
#endif

#define FW_IS_LOGANDSTREAM  1

#define IS_CONNECTED_EEPROM 1

#define USE_DEFAULT_SENSOR  0
#define RTC_FAST \
  0 //need to increase rtc alarm interrupt priority before enabling this option
#define USE_8BYTES_INIT_TS         0
#define TEST_UNDOCKED              0
#define FULL_TEST_MODE             0
#define USE_FREERTOS               0
#define SENS_CLK_RTC0TIM1          0

#define USE_DEFAULT_LED            1
#define USE_I2C_VBATT_REPORT       1
#define SKIP_50MS                  1
#define USE_VBATT_ALWAYS           0
#define HAL_TEST_INFOMEM           0
#define USE_BT                     1
#define USE_SD                     1
#define USE_BMPX80                 2 //x=1 for 180, x=2 for 280
#define NUM_SDWRBUF                64
#define USE_FATFS                  1
#define USE_USBX                   0
#define SHIMMER_ENABLE_PRINTF      1

#define SR48_6_0                   1
#define SR48_6_0_PATCH_DOCK_DETECT SR48_6_0

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

//BT  Channel contents
#define MAX_NUM_CHANNELS \
  34 //3xanalogAccel + 3xdigiGyro + 3xdigiMag +
     //3xLSM303DLHCAccel + 3xMPU9250Accel + 3xMPU9250MAG +
     //BMP180TEMP + BMP180PRESS + batteryVoltage + 5stc3100
     //3xexternalADC + 5xinternalADC
#define X_LN_ACCEL 0x00
#define Y_LN_ACCEL 0x01
#define Z_LN_ACCEL 0x02
#define VBATT      0x03
#define X_WR_ACCEL 0x04
#define Y_WR_ACCEL 0x05
#define Z_WR_ACCEL 0x06
#define X_MAG      0x07
#define Y_MAG      0x08
#define Z_MAG      0x09
#define X_GYRO     0x0A
#define Y_GYRO     0x0B
#define Z_GYRO     0x0C
#if defined(SHIMMER3R)
#define EXTERNAL_ADC_0 0x0D
#define EXTERNAL_ADC_1 0x0E
#define EXTERNAL_ADC_2 0x0F
#define INTERNAL_ADC_3 0x10
#define INTERNAL_ADC_0 0x11
#define INTERNAL_ADC_1 0x12
#define INTERNAL_ADC_2 0x13
#elif defined(SHIMMER3)
#define EXTERNAL_ADC_7  0x0D
#define EXTERNAL_ADC_6  0x0E
#define EXTERNAL_ADC_15 0x0F
#define INTERNAL_ADC_1  0x10
#define INTERNAL_ADC_12 0x11
#define INTERNAL_ADC_13 0x12
#define INTERNAL_ADC_14 0x13
#endif
#define X_ALT_ACCEL              0x14
#define Y_ALT_ACCEL              0x15
#define Z_ALT_ACCEL              0x16
#define X_ALT_MAG                0x17
#define Y_ALT_MAG                0x18
#define Z_ALT_MAG                0x19
#define BMP_TEMPERATURE          0x1A
#define BMP_PRESSURE             0x1B
#define GSR_RAW                  0x1C
#define EXG_ADS1292R_1_STATUS    0x1D
#define EXG_ADS1292R_1_CH1_24BIT 0x1E
#define EXG_ADS1292R_1_CH2_24BIT 0x1F
#define EXG_ADS1292R_2_STATUS    0x20
#define EXG_ADS1292R_2_CH1_24BIT 0x21
#define EXG_ADS1292R_2_CH2_24BIT 0x22
#define EXG_ADS1292R_1_CH1_16BIT 0x23
#define EXG_ADS1292R_1_CH2_16BIT 0x24
#define EXG_ADS1292R_2_CH1_16BIT 0x25
#define EXG_ADS1292R_2_CH2_16BIT 0x26
#define STRAIN_HIGH              0x27
#define STRAIN_LOW               0x28
#if defined(SHIMMER4_SDK)
#define STC3100_CH_1    0x29
#define STC3100_CH_2    0x2A
#define STC3100_CH_3    0x2B
#define STC3100_CH_4    0x2C
#define STC3100_CH_5    0x2D
#define EXTERNAL_ADC_0  0x2E
#define EXTERNAL_ADC_1  0x2F
#define EXTERNAL_ADC_15 0x30
#define INTERNAL_ADC_1  0x31
#define INTERNAL_ADC_12 0x32
#define INTERNAL_ADC_13 0x33
#define INTERNAL_ADC_14 0x34
#define INT_ADC_3       0x35
#define PPG_1           0x36
#define PPG_2           0x37
#endif

#if defined(SHIMMER3)
//SENSORS0
#define SENSOR_A_ACCEL        0x80
#define SENSOR_MPU9250_GYRO   0x40
#define SENSOR_LSM303DLHC_MAG 0x20
#define SENSOR_EXG1_24BIT     0x10
#define SENSOR_EXG2_24BIT     0x08
#define SENSOR_GSR            0x04
#define SENSOR_EXT_ADC_0      0x02
#define SENSOR_EXT_ADC_1      0x01
//SENSORS1
#define SENSOR_STRAIN         0x80
//#define SDH_SENSOR_HR                0x40
#if defined(SHIMMER4_SDK)
#define SENSOR_APP_PPG 0x40
#endif
#define SENSOR_VBATT            0x20
#define SENSOR_LSM303DLHC_ACCEL 0x10
#define SENSOR_EXT_ADC_2        0x08
#define SENSOR_INT_ADC_3        0x04
#define SENSOR_INT_ADC_0        0x02
#define SENSOR_INT_ADC_1        0x01
//SENORS2
#define SENSOR_INT_ADC_2        0x80
#define SENSOR_MPU9250_ACCEL    0x40
#define SENSOR_MPU9250_MAG      0x20
#define SENSOR_EXG1_16BIT       0x10
#define SENSOR_EXG2_16BIT       0x08
#define SENSOR_BMP180_PRESSURE  0x04
#if defined(SHIMMER4_SDK)
#define SENSOR_INT_ADC_4 0x02
#define SENSOR_STC3100   0x01
#endif
NV_SENSORS5
#define SENSOR_APP_STRAIN                     0x80
#define SENSOR_APP_GSR                        0x40
#define SENSOR_APP_PPG                        0x20

//Config byte masks
//Config Byte0
#define LSM303DLHC_ACCEL_SAMPLING_RATE        0xF0
#define LSM303DLHC_ACCEL_RANGE                0x0C
#define LSM303DLHC_ACCEL_LOW_POWER_MODE       0x02
#define LSM303DLHC_ACCEL_HIGH_RESOLUTION_MODE 0x01
//Config Byte1
#define MPU9250_SAMPLING_RATE                 0xFF
//Config Byte2
#define LSM303DLHC_MAG_GAIN                   0xE0
#define LSM303DLHC_MAG_SAMPLING_RATE          0x1C
#define MPU9250_GYRO_RANGE                    0x03
//Config Byte3
#define MPU9250_ACCEL_RANGE                   0xC0
#define BMP180_PRESSURE_RESOLUTION            0x30
#define GSR_RANGE                             0x0E
#define EXP_POWER_ENABLE                      0x01
//Unused bits 3-0

//ADC initialisation mask
#define MASK_A_ACCEL                          0x0001
#define MASK_VBATT                            0x0002
#define MASK_EXT_A7                           0x0004
#define MASK_EXT_A6                           0x0008
#define MASK_EXT_A15                          0x0010
#define MASK_INT_A1                           0x0020
#define MASK_INT_A12                          0x0040
#define MASK_INT_A13                          0x0080
#define MASK_INT_A14                          0x0100
#define MASK_MSP_TEMP                         0x0200
#define MASK_STRAIN                           0x0180 //uses ADC13 and ADC14

//LSM303DLHC Accel Range
//Corresponds to the FS field of the LSM303DLHC's CTRL_REG4_A register
//and the AFS_SEL field of the MPU9250's ACCEL_CONFIG register
#define ACCEL_2G                              0x00
#define ACCEL_4G                              0x01
#define ACCEL_8G                              0x02
#define ACCEL_16G                             0x03

//LSM303DLHC Accel Sampling Rate
//Corresponds to the ODR field of the LSM303DLHC's CTRL_REG1_A register
#define LSM303DLHC_ACCEL_POWER_DOWN           0x00
#define LSM303DLHC_ACCEL_1HZ                  0x01
#define LSM303DLHC_ACCEL_10HZ                 0x02
#define LSM303DLHC_ACCEL_25HZ                 0x03
#define LSM303DLHC_ACCEL_50HZ                 0x04
#define LSM303DLHC_ACCEL_100HZ                0x05
#define LSM303DLHC_ACCEL_200HZ                0x06
#define LSM303DLHC_ACCEL_400HZ                0x07
#define LSM303DLHC_ACCEL_1_620KHZ             0x08 //1.620kHz in Low-power mode only
#define LSM303DLHC_ACCEL_1_344kHz \
  0x09 //1.344kHz in normal mode, 5.376kHz in low-power mode

//LSM303DLHC Mag gain
#define LSM303DLHC_MAG_1_3G   0x01 //+/-1.3 Gauss
#define LSM303DLHC_MAG_1_9G   0x02 //+/-1.9 Gauss
#define LSM303DLHC_MAG_2_5G   0x03 //+/-2.5 Gauss
#define LSM303DLHC_MAG_4_0G   0x04 //+/-4.0 Gauss
#define LSM303DLHC_MAG_4_7G   0x05 //+/-4.7 Gauss
#define LSM303DLHC_MAG_5_6G   0x06 //+/-5.6 Gauss
#define LSM303DLHC_MAG_8_1G   0x07 //+/-8.1 Gauss

//LSM303DLHC Mag sampling rate
#define LSM303DLHC_MAG_0_75HZ 0x00 //0.75 Hz
#define LSM303DLHC_MAG_1_5HZ  0x01 //1.5 Hz
#define LSM303DLHC_MAG_3HZ    0x02 //3.0 Hz
#define LSM303DLHC_MAG_7_5HZ  0x03 //7.5 Hz
#define LSM303DLHC_MAG_15HZ   0x04 //15 Hz
#define LSM303DLHC_MAG_30HZ   0x05 //30 Hz
#define LSM303DLHC_MAG_75HZ   0x06 //75 Hz
#define LSM303DLHC_MAG_220HZ  0x07 //220 Hz
#endif

#if defined(SHIMMER3)
//calibration info
#define S_ACCEL_A            0
#define S_GYRO               1
#define S_MAG                2
#define S_ACCEL_D            3
//#define S_ECG                     3
//#define S_EMG                     4

//MPU9250 Gyro range
#define MPU9150_GYRO_250DPS  0x00 //+/-250 dps
#define MPU9150_GYRO_500DPS  0x01 //+/-500 dps
#define MPU9150_GYRO_1000DPS 0x02 //+/-1000 dps
#define MPU9150_GYRO_2000DPS 0x03 //+/-2000 dps
#define MPU9250_GYRO_250DPS  0x00 //+/-250 dps
#define MPU9250_GYRO_500DPS  0x01 //+/-500 dps
#define MPU9250_GYRO_1000DPS 0x02 //+/-1000 dps
#define MPU9250_GYRO_2000DPS 0x03 //+/-2000 dps

//#digital accel_range
#define RANGE_2G             0
#define RANGE_4G             1
#define RANGE_8G             2
#define RANGE_16G            3

//#mag_gain
#define LSM303_MAG_13GA      1
#define LSM303_MAG_19GA      2
#define LSM303_MAG_25GA      3
#define LSM303_MAG_40GA      4
#define LSM303_MAG_47GA      5
#define LSM303_MAG_56GA      6
#define LSM303_MAG_81GA      7

//BMP Pressure oversampling ratio
#define BMP180_OSS_1         0x00
#define BMP180_OSS_2         0x01
#define BMP180_OSS_4         0x02
#define BMP180_OSS_8         0x03
#endif

//SD Log file header format
#define SD_HEAD_SIZE                    384
#define SD_WRITE_BUF_SIZE               512

#define SDH_SAMPLE_RATE_0               0
#define SDH_SAMPLE_RATE_1               1
#define SDH_BUFFER_SIZE                 2
#define SDH_SENSORS0                    3
#define SDH_SENSORS1                    4
#define SDH_SENSORS2                    5
#define SDH_SENSORS3                    6
#define SDH_SENSORS4                    7
#define SDH_CONFIG_SETUP_BYTE0          8 //sensors setting bytes
#define SDH_CONFIG_SETUP_BYTE1          9
#define SDH_CONFIG_SETUP_BYTE2          10
#define SDH_CONFIG_SETUP_BYTE3          11
#define SDH_CONFIG_SETUP_BYTE4          12
#define SDH_CONFIG_SETUP_BYTE5          13
#define SDH_CONFIG_SETUP_BYTE6          14
#define SDH_TRIAL_CONFIG0               16
#define SDH_TRIAL_CONFIG1               17
#define SDH_BROADCAST_INTERVAL          18
#define SDH_BT_COMMS_BAUD_RATE          19
#define SDH_EST_EXP_LEN_MSB             20
#define SDH_EST_EXP_LEN_LSB             21
#define SDH_MAX_EXP_LEN_MSB             22
#define SDH_MAX_EXP_LEN_LSB             23
#define SDH_MAC_ADDR                    24
#define SDH_SHIMMERVERSION_BYTE_0       30
#define SDH_SHIMMERVERSION_BYTE_1       31
#define SDH_MYTRIAL_ID                  32
#define SDH_NSHIMMER                    33
#define SDH_FW_VERSION_TYPE_0           34
#define SDH_FW_VERSION_TYPE_1           35
#define SDH_FW_VERSION_MAJOR_0          36
#define SDH_FW_VERSION_MAJOR_1          37
#define SDH_FW_VERSION_MINOR            38
#define SDH_FW_VERSION_INTERNAL         39
#define SDH_DERIVED_CHANNELS_0          40
#define SDH_DERIVED_CHANNELS_1          41
#define SDH_DERIVED_CHANNELS_2          42
#define SDH_RTC_DIFF_0                  44
#define SDH_RTC_DIFF_1                  45
#define SDH_RTC_DIFF_2                  46
#define SDH_RTC_DIFF_3                  47
#define SDH_RTC_DIFF_4                  48
#define SDH_RTC_DIFF_5                  49
#define SDH_RTC_DIFF_6                  50
#define SDH_RTC_DIFF_7                  51
#define SDH_CONFIG_TIME_0               52
#define SDH_CONFIG_TIME_1               53
#define SDH_CONFIG_TIME_2               54
#define SDH_CONFIG_TIME_3               55
#define SDH_EXG_ADS1292R_1_CONFIG1      56
#define SDH_EXG_ADS1292R_1_CONFIG2      57
#define SDH_EXG_ADS1292R_1_LOFF         58
#define SDH_EXG_ADS1292R_1_CH1SET       59
#define SDH_EXG_ADS1292R_1_CH2SET       60
#define SDH_EXG_ADS1292R_1_RLD_SENS     61
#define SDH_EXG_ADS1292R_1_LOFF_SENS    62
#define SDH_EXG_ADS1292R_1_LOFF_STAT    63
#define SDH_EXG_ADS1292R_1_RESP1        64
#define SDH_EXG_ADS1292R_1_RESP2        65
#define SDH_EXG_ADS1292R_2_CONFIG1      66
#define SDH_EXG_ADS1292R_2_CONFIG2      67
#define SDH_EXG_ADS1292R_2_LOFF         68
#define SDH_EXG_ADS1292R_2_CH1SET       69
#define SDH_EXG_ADS1292R_2_CH2SET       70
#define SDH_EXG_ADS1292R_2_RLD_SENS     71
#define SDH_EXG_ADS1292R_2_LOFF_SENS    72
#define SDH_EXG_ADS1292R_2_LOFF_STAT    73
#define SDH_EXG_ADS1292R_2_RESP1        74
#define SDH_EXG_ADS1292R_2_RESP2        75
#define SDH_WR_ACCEL_CALIBRATION        76  //0x4c
#define SDH_GYRO_CALIBRATION            97  //0x61
#define SDH_MAG_CALIBRATION             118 //0x76
#define SDH_LN_ACCEL_CALIBRATION        139 //0x8b
#define SDH_TEMP_PRES_CALIBRATION       160
#define SDH_WR_ACCEL_CALIB_TS           (182) //+8
#define SDH_GYRO_CALIB_TS               (190) //+8
#define SDH_MAG_CALIB_TS                (198) //+8
#define SDH_LN_ACCEL_CALIB_TS           (206) //+8
#define SDH_DAUGHTER_CARD_ID_BYTE0      (214) //+3
#define SDH_DERIVED_CHANNELS_3          (217)
#define SDH_DERIVED_CHANNELS_4          (218)
#define SDH_DERIVED_CHANNELS_5          (219)
#define SDH_DERIVED_CHANNELS_6          (220)
#define SDH_DERIVED_CHANNELS_7          (221)
#define SDH_TEMP_PRES_EXTRA_CALIB_BYTES (222)
//#define SDH_MY_LOCALTIME_0TH             248
#define SDH_MY_LOCALTIME_5TH            251
#define SDH_MY_LOCALTIME                252 //252-255
#define SDH_ALT_ACCEL_CALIBRATION       256 //+21
#define SDH_ALT_ACCEL_CALIB_TS          277 //+8
#define SDH_ALT_MAG_CALIBRATION         285 //+21
#define SDH_ALT_MAG_CALIB_TS            306 //+8

/*
//SENSORS0
#define SDH_SENSOR_A_ACCEL               0x80
#define SDH_SENSOR_MPU9250_GYRO          0x40
#define SDH_SENSOR_LSM303DLHC_MAG        0x20
#define SDH_SENSOR_EXG1_24BIT            0x10
#define SDH_SENSOR_EXG2_24BIT            0x08
#define SDH_SENSOR_GSR                   0x04
#define SDH_SENSOR_EXTCH7                0x02
#define SDH_SENSOR_EXTCH6                0x01
//SENSORS1
#define SDH_SENSOR_STRAIN                0x80
//#define SDH_SENSOR_HR                0x40
#define SDH_SENSOR_VBATT                 0x20
#define SDH_SENSOR_LSM303DLHC_ACCEL      0x10
#define SDH_SENSOR_EXTCH15               0x08
#define SDH_SENSOR_INTCH1                0x04
#define SDH_SENSOR_INTCH12               0x02
#define SDH_SENSOR_INTCH13               0x01
//SENSORS2
#define SDH_SENSOR_INTCH14               0x80
#define SDH_SENSOR_MPU9250_ACCEL         0x40
#define SDH_SENSOR_MPU9250_MAG           0x20
#define SDH_SENSOR_EXG1_16BIT            0x10
#define SDH_SENSOR_EXG2_16BIT            0x08
#define SDH_SENSOR_BMP180_PRES           0x04
//#define SDH_SENSOR_BMP180_TEMP       0x02
//SENSORS3
#define SDH_SENSOR_MSP430_TEMP           0x01
#define SDH_SENSOR_TCXO                  0x80

//SDH_TRIAL_CONFIG0
 */
#define SDH_IAMMASTER                   0x02
/*
#define SDH_TIME_SYNC                    0x04
#define SDH_TIME_STAMP                   0x08 //not used now, reserved as 1
#define SDH_RWCERROR_EN \
  0x10 //when 0, won't flash error. when 1, will flash error if RTC isn't set (RTC_offset == 0)
//#define SDH_GYRO_BUTTON_ENABLE         0x10
#define SDH_USER_BUTTON_ENABLE 0x20
#define SDH_SET_PMUX           0x40 //not used now, reserved as 0
#define SDH_RTC_SET_BY_BT      0x80
//SDH_TRIAL_CONFIG1
#define SDH_SINGLETOUCH        0x80
//#define SDH_ACCEL_LPM                  0x40//config has this bit
//#define SDH_ACCEL_HRM                  0x20//config has this bit
 */
#define SDH_TCXO                        0x10
/*
//#define SDH_EXP_POWER                  0x08//config has this bit
//#define SDH_MONITOR                    0x04
 */

#define MAX_CHARS                       13
#define UINT32_LEN                      11 //10+1, where the last byte should be 0x00
#define UINT64_LEN                      21 //20+1, where the last byte should be 0x00
#define RESPONSE_PACKET_SIZE            1024 //133

//BATTERY
#define BATT_LOW                        0x01
#define BATT_MID                        0x02
#define BATT_HIGH                       0x04
#if defined(SHIMMER4_SDK)
#define BATT_INTERVAL   600 //600 seconds = 10min interval
#define BATT_INTERVAL_D 30  //30 seconds
#endif

#define BATTERY_ERROR_VOLTAGE_MAX 4500 //mV
#define BATTERY_ERROR_VOLTAGE_MIN 3200 //mV

enum
{
  //STAT2 = bit7, STAT1 = bit 6
  CHRG_CHIP_STATUS_SUSPENDED = 0xC0,       //STAT2 high (off), STAT1 high (off)
  CHRG_CHIP_STATUS_PRECONDITIONING = 0x80, //STAT2 high (off), STAT1 low (on)
  CHRG_CHIP_STATUS_FULLY_CHARGED = 0x40,   //STAT2 low (on), STAT1 high (off)
  CHRG_CHIP_STATUS_BAD_BATTERY = 0x00,     //STAT2 low (on), STAT1 low (on)
  CHRG_CHIP_STATUS_UNKNOWN = 0xFF,
};

typedef enum
{
  CHARGING_STATUS_UNKNOWN,
  CHARGING_STATUS_CHECKING,
  CHARGING_STATUS_SUSPENDED,
  CHARGING_STATUS_FULLY_CHARGED,
  CHARGING_STATUS_CHARGING,
  CHARGING_STATUS_BAD_BATTERY,
  CHARGING_STATUS_ERROR
} chargingStatus_t;

#define STAT_PERI_ADC      0x01
#define STAT_PERI_I2C_SENS 0x02
#if defined(SHIMMER4_SDK)
#define STAT_PERI_I2C_BATT 0x04
#endif
#define STAT_PERI_SPI_SENS 0x08
#define STAT_PERI_SDMMC    0x10
#define STAT_PERI_BT       0x20
#define PeriStat_Set(x)          \
  do                             \
  {                              \
    shimmerStatus.periStat |= x; \
  } while (0)
#define PeriStat_Clr(x)           \
  do                              \
  {                               \
    shimmerStatus.periStat &= ~x; \
  } while (0)

typedef volatile struct STATTypeDef_t
{ //STATUS
  uint8_t initialising  : 1;
  uint8_t docked        : 1;
  uint8_t sensing       : 1;
  uint8_t configuring   : 1;
  uint8_t buttonPressed : 1;

  uint8_t btConnected   : 1;
  uint8_t btPowerOn     : 1;
  uint8_t btStreaming   : 1;
  uint8_t btstreamReady : 1;
  uint8_t btstreamCmd   : 2;

#if defined(SHIMMER3R)
  uint8_t sdPeripheralInit : 1;
#endif
  uint8_t sdInserted : 1;
  uint8_t sdPowerOn  : 1;
#if defined(SHIMMER3R)
  uint8_t sdMcu0Pc1 : 1;
#endif
  uint8_t sdLogging              : 1;
  uint8_t sdlogReady             : 1;
  uint8_t sdlogCmd               : 2;
  uint8_t sdBadFile              : 1;
  uint8_t sdSyncEnabled          : 1;
  uint8_t sdSyncCommTimerRunning : 1;

  uint8_t toggleLedRedCmd        : 1;
#if defined(SHIMMER3R)
  uint32_t testResult;
  uint8_t pinPvI2c : 1;
  uint8_t pinPvSd  : 1;
  uint8_t pinPvExt : 1;
  uint8_t periStat;
#endif
} STATTypeDef;

typedef union
{
  uint8_t rawBytes[3];

  struct __attribute__((packed))
  {
    uint16_t adcBattVal;

    //STAT2 sits in Bit7 and STAT1 in Bit6
    uint8_t unusedBits : 6;
    uint8_t STAT1      : 1;
    uint8_t STAT2      : 1;
  };
} BattStatusRaw;

typedef volatile struct batt_status_t
{
  /* General battery level based on ADC voltage with buffered min/max values */
  uint8_t battStat;
#if defined(SHIMMER3R)
  /* LED colour to show when undocked based on the latest battStat */
  uint32_t battStatLed;
  /* LED colour to show when docked */
  uint32_t battStatLedCharging;
  /* Lets the LED timer know whether the LED should flash or stay solid */
  uint8_t battStatLedFlash : 1;
#endif
  /* The ADC value and charger status bytes which are sent via dock/BT */
  BattStatusRaw battStatusRaw;
  /* Latest measured battery voltage in mV */
  uint16_t battValMV;
  /* Overall status based on batt mV, charger chip status and docked/undocked */
  chargingStatus_t battChargingStatus;
#if defined(SHIMMER4_SDK)
  uint8_t battDigital[10];
#endif
} BattStatus;

typedef enum
{
  BATT_INTERVAL_UNDOCKED, //10 Minutes
  BATT_INTERVAL_DOCKED    //30 Seconds
} battAlarmInterval_t;

typedef enum
{
  BOOT_STAGE_START,
  BOOT_STAGE_I2C,
  BOOT_STAGE_BLUETOOTH,
  BOOT_STAGE_BLUETOOTH_FAILURE,
  BOOT_STAGE_CONFIGURATION,
  BOOT_STAGE_END
} boot_stage_t;

enum
{
  SD_LOG_CMD_STATE_IDLE = 0,
  SD_LOG_CMD_STATE_START = 1,
  SD_LOG_CMD_STATE_STOP = 2
};

//typedef enum{//bt
//   UART_BT_STAT_IDLE = 0,
//   UART_BT_STAT_CONFIG = 1
//}UART_BT_STATUS;

#endif //S4_H
