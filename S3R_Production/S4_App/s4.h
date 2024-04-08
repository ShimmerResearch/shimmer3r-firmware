// s4.h

#ifndef S4_H
#define S4_H

#if defined(SHIMMER3R)
#define DEVICE_VER            3
#define FW_IDENTIFIER         3      // 12 is the firmware for shimmer4sdk, with LogAndStream in side
#define FW_VER_MAJOR          1      // Maor version number: 0-65535
#define FW_VER_MINOR          0      // Minor version number: 0-255
#define FW_VER_REL            0     // internal version number: 0-255
#elif defined(SHIMMER4_SDK)
#define DEVICE_VER            58      // HARDWARE VERSION: SR_58 for Shimmer4SDK
#define FW_IDENTIFIER         12      // 12 is the firmware for shimmer4sdk, with LogAndStream in side
#define FW_VER_MAJOR          0      // Maor version number: 0-65535
#define FW_VER_MINOR          0      // Minor version number: 0-255
#define FW_VER_REL            23     // internal version number: 0-255
#endif
      
//typedef uint8_t bool;
//#define TRUE 1
//#define FALSE 0

//typedef uint8_t error_t;
//#define SUCCESS 1
//#define FAIL 0

// UART COMMANDS
//================= WP uart 3.0: command names ==============
#define UART_STEP_WAIT4_CMD   4
#define UART_STEP_WAIT4_LEN   3
#define UART_STEP_WAIT4_DATA  2
#define UART_STEP_WAIT4_CRC   1
#define UART_STEP_WAIT4_NONE  0

#define UART_RXBUF_START      0
#define UART_RXBUF_CMD        1
#define UART_RXBUF_LEN        2
#define UART_RXBUF_COMP       3
#define UART_RXBUF_PROP       4  // data in rxbuf starts from byte 3
#define UART_RXBUF_DATA       5  // data in rxbuf starts from byte 3
#define UART_DATA_LEN_MAX     138// max case: '$' + get + length + comp_shimmer+ prop_infomem
#define UART_RSP_PACKET_SIZE  138// + info_len + info_loc*2 + 128bytes data + crc*2 = 138


#define UART_SET                    0x01
#define UART_RESPONSE               0x02
#define UART_GET                    0x03
#define UART_BAD_CMD_RESPONSE       0xfc //252
#define UART_BAD_ARG_RESPONSE       0xfd //253
#define UART_BAD_CRC_RESPONSE       0xfe //254
#define UART_ACK_RESPONSE           0xff //255
//================= WP uart 3.0: components names ==============
#define UART_COMP_SHIMMER           0x01
#define UART_COMP_BAT               0x02 // this is seen as a sensor
#define UART_COMP_DAUGHTER_CARD     0x03
#define UART_COMP_D_ACCEL           0x04
#define UART_COMP_GSR               0x05
//================= WP uart 3.0: property names ==============
// component == UART_COMP_SHIMMER:
#define UART_PROP_ENABLE            0x00 // this is for all sensors
#define UART_PROP_SAMPLE_RATE       0x01
#define UART_PROP_MAC               0x02
#define UART_PROP_VER               0x03
#define UART_PROP_RTC_CFG_TIME      0x04
#define UART_PROP_CURR_LOCAL_TIME   0x05
#define UART_PROP_INFOMEM           0x06
#define UART_PROP_CALIB_DUMP        0x07
// component == UART_COMP_BAT:
//#define UART_PROP_SAMPLE_RATE       0x01
#define UART_PROP_VALUE             0x02
//#define UART_PROP_DIVIDER           0x05
// component == UART_COMP_DAUGHTER_CARD:
#define UART_PROP_CARD_ID           0x02
#define UART_PROP_CARD_MEM          0x03
// component == UART_COMP_D_ACCEL:
//#define UART_PROP_ENABLE            0x00
//#define UART_PROP_SAMPLE_RATE       0x01
#define UART_PROP_DATA_RATE         0x02
#define UART_PROP_RANGE             0x03
#define UART_PROP_LP_MODE           0x04
#define UART_PROP_HR_MODE           0x05
#define UART_PROP_FREQ_DIVIDER      0x06
#define UART_PROP_CALIBRATION       0x07
// component == UART_COMP_GSR:
//#define UART_PROP_ENABLE            0x00
//#define UART_PROP_SAMPLE_RATE       0x01
//#define UART_PROP_RANGE             0x03
//#define UART_PROP_DIVIDER           0x05
//== new uart ends ==


// BT  Channel contents
#define MAX_NUM_CHANNELS         34    //3xanalogAccel + 3xdigiGyro + 3xdigiMag +
                                       //3xLSM303DLHCAccel + 3xMPU9250Accel + 3xMPU9250MAG +
                                       //BMP180TEMP + BMP180PRESS + batteryVoltage + 5stc3100
                                       //3xexternalADC + 5xinternalADC
#define X_A_ACCEL                         0x00
#define Y_A_ACCEL                         0x01
#define Z_A_ACCEL                         0x02
#define VBATT                             0x03
#define X_LSM303DLHC_ACCEL                0x04
#define Y_LSM303DLHC_ACCEL                0x05
#define Z_LSM303DLHC_ACCEL                0x06
#define X_LSM303DLHC_MAG                  0x07
#define Y_LSM303DLHC_MAG                  0x08
#define Z_LSM303DLHC_MAG                  0x09
#define X_MPU9250_GYRO                    0x0A
#define Y_MPU9250_GYRO                    0x0B
#define Z_MPU9250_GYRO                    0x0C
#define EXTERNAL_ADC_7                    0x0D
#define EXTERNAL_ADC_6                    0x0E
#define EXTERNAL_ADC_15                   0x0F
#define INTERNAL_ADC_1                    0x10
#define INTERNAL_ADC_12                   0x11
#define INTERNAL_ADC_13                   0x12
#define INTERNAL_ADC_14                   0x13
#define X_MPU9250_ACCEL                   0x14
#define Y_MPU9250_ACCEL                   0x15
#define Z_MPU9250_ACCEL                   0x16
#define X_MPU9250_MAG                     0x17
#define Y_MPU9250_MAG                     0x18
#define Z_MPU9250_MAG                     0x19
#define BMP180_TEMP                       0x1A
#define BMP180_PRESSURE                   0x1B
#define GSR_RAW                           0x1C
#define EXG_ADS1292R_1_STATUS             0x1D
#define EXG_ADS1292R_1_CH1_24BIT          0x1E
#define EXG_ADS1292R_1_CH2_24BIT          0x1F
#define EXG_ADS1292R_2_STATUS             0x20
#define EXG_ADS1292R_2_CH1_24BIT          0x21
#define EXG_ADS1292R_2_CH2_24BIT          0x22
#define EXG_ADS1292R_1_CH1_16BIT          0x23
#define EXG_ADS1292R_1_CH2_16BIT          0x24
#define EXG_ADS1292R_2_CH1_16BIT          0x25
#define EXG_ADS1292R_2_CH2_16BIT          0x26
#define STRAIN_HIGH                       0x27
#define STRAIN_LOW                        0x28
#define STC3100_CH_1                      0x29
#define STC3100_CH_2                      0x2A
#define STC3100_CH_3                      0x2B
#define STC3100_CH_4                      0x2C
#define STC3100_CH_5                      0x2D
#define EXT_ADC_9                         0x2E
#define EXT_ADC_8                         0x2F
#define EXT_ADC_1                         0x30
#define INT_ADC_10                        0x31
#define INT_ADC_12                        0x32
#define INT_ADC_11                        0x33
#define INT_ADC_0                         0x34
#define INT_ADC_2                         0x35
#define PPG_1                             0x36
#define PPG_2                             0x37

// Infomem contents
//#define NV_NUM_CONFIG_BYTES             100
// Infomem contents
#define NV_NUM_SETTINGS_BYTES             34
#define NV_NUM_CALIBRATION_BYTES          84
#define NV_NUM_SD_BYTES                   37
#define NV_TOTAL_NUM_CONFIG_BYTES         384//NV_NUM_SETTINGS_BYTES + NV_NUM_CALIBRATION_BYTES + NV_NUM_SD_BYTES
#define NV_NUM_RWMEM_BYTES                512

#define NV_SAMPLING_RATE                  0
#define NV_BUFFER_SIZE                    2
#define NV_SENSORS0                       3
#define NV_SENSORS1                       4
#define NV_SENSORS2                       5
#define NV_CONFIG_SETUP_BYTE0             6 //sensors setting bytes
#define NV_CONFIG_SETUP_BYTE1             7
#define NV_CONFIG_SETUP_BYTE2             8
#define NV_CONFIG_SETUP_BYTE3             9
#define NV_EXG_ADS1292R_1_CONFIG1         10// exg bytes, not implemented yet
#define NV_EXG_ADS1292R_1_CONFIG2         11
#define NV_EXG_ADS1292R_1_LOFF            12
#define NV_EXG_ADS1292R_1_CH1SET          13
#define NV_EXG_ADS1292R_1_CH2SET          14
#define NV_EXG_ADS1292R_1_RLD_SENS        15
#define NV_EXG_ADS1292R_1_LOFF_SENS       16
#define NV_EXG_ADS1292R_1_LOFF_STAT       17
#define NV_EXG_ADS1292R_1_RESP1           18
#define NV_EXG_ADS1292R_1_RESP2           19
#define NV_EXG_ADS1292R_2_CONFIG1         20
#define NV_EXG_ADS1292R_2_CONFIG2         21
#define NV_EXG_ADS1292R_2_LOFF            22
#define NV_EXG_ADS1292R_2_CH1SET          23
#define NV_EXG_ADS1292R_2_CH2SET          24
#define NV_EXG_ADS1292R_2_RLD_SENS        25
#define NV_EXG_ADS1292R_2_LOFF_SENS       26
#define NV_EXG_ADS1292R_2_LOFF_STAT       27
#define NV_EXG_ADS1292R_2_RESP1           28
#define NV_EXG_ADS1292R_2_RESP2           29
#define NV_BT_COMMS_BAUD_RATE             30
#define NV_DERIVED_CHANNELS_0             31
#define NV_DERIVED_CHANNELS_1             32
#define NV_DERIVED_CHANNELS_2             33
#define NV_A_ACCEL_CALIBRATION            34
#define NV_MPU9250_GYRO_CALIBRATION       55
#define NV_LSM303DLHC_MAG_CALIBRATION     76
#define NV_LSM303DLHC_ACCEL_CALIBRATION   97       //97->117
#define NV_CALIBRATION_END                117
#define NV_DERIVED_CHANNELS_3             118
#define NV_DERIVED_CHANNELS_4             119
#define NV_DERIVED_CHANNELS_5             120
#define NV_DERIVED_CHANNELS_6             121
#define NV_DERIVED_CHANNELS_7             122

#define NV_SENSORS3                       (128+0)
#define NV_SENSORS4                       (128+1)
#define NV_CONFIG_SETUP_BYTE4             (128+2)
#define NV_CONFIG_SETUP_BYTE5             (128+3)
#define NV_CONFIG_SETUP_BYTE6             (128+4)
#define NV_MPL_ACCEL_CALIBRATION          (128+5)    //+21
#define NV_MPL_MAG_CALIBRATION            (128+26)   //+21
#define NV_MPL_GYRO_CALIBRATION           (128+47)   //+12
#define NV_SD_SHIMMER_NAME                (128+59)   // +12 bytes
#define NV_SD_EXP_ID_NAME                 (128+71)   // +12 bytes
#define NV_SD_CONFIG_TIME                 (128+83)   // +4 bytes
#define NV_SD_MYTRIAL_ID                  (128+87)   // 1 byte
#define NV_SD_NSHIMMER                    (128+88)   // 1 byte
#define NV_SD_TRIAL_CONFIG0               (128+89)
#define NV_SD_TRIAL_CONFIG1               (128+90)
#define NV_SD_BT_INTERVAL                 (128+91)
#define NV_EST_EXP_LEN_MSB                (128+92)  // 2bytes
#define NV_EST_EXP_LEN_LSB                (128+93)
#define NV_MAX_EXP_LEN_MSB                (128+94)  // 2bytes
#define NV_MAX_EXP_LEN_LSB                (128+95)
#define NV_MAC_ADDRESS                    (128+96)   // 6bytes
#define NV_SD_CONFIG_DELAY_FLAG           (128+102)
#define NV_BT_SET_PIN                     (128+103)
#define NV_TEMP_PRES_CALIBRATION          (128+104) // +22 bytes, till 128+125

#define NV_CENTER                         (128+128+0)
#define NV_NODE0                          (128+128+6)

//SENSORS0
#define SENSOR_A_ACCEL                 0x80
#define SENSOR_MPU9250_GYRO            0x40
#define SENSOR_LSM303DLHC_MAG          0x20
#define SENSOR_EXG1_24BIT              0x10
#define SENSOR_EXG2_24BIT              0x08
#define SENSOR_GSR                     0x04
#define SENSOR_EXT_ADC_9               0x02
#define SENSOR_EXT_ADC_8               0x01
//SENSORS1
#define SENSOR_STRAIN                  0x80
//#define SDH_SENSOR_HR                0x40
#define SENSOR_APP_PPG                 0x40
#define SENSOR_VBATT                   0x20
#define SENSOR_LSM303DLHC_ACCEL        0x10
#define SENSOR_EXT_ADC_1               0x08
#define SENSOR_INT_ADC_2               0x04
#define SENSOR_INT_ADC_10              0x02
#define SENSOR_INT_ADC_11              0x01
//SENORS2
#define SENSOR_INT_ADC_0               0x80
#define SENSOR_MPU9250_ACCEL           0x40
#define SENSOR_MPU9250_MAG             0x20
#define SENSOR_EXG1_16BIT              0x10
#define SENSOR_EXG2_16BIT              0x08
#define SENSOR_BMP180_PRESSURE         0x04
#define SENSOR_INT_ADC_12              0x02
#define SENSOR_STC3100                 0x01
//NV_SENSORS5
//#define SENSOR_APP_STRAIN              0x80
//#define SENSOR_APP_GSR                 0x40
//#define SENSOR_APP_PPG                 0x20



//Config byte masks
//Config Byte0
#define LSM303DLHC_ACCEL_SAMPLING_RATE          0xF0
#define LSM303DLHC_ACCEL_RANGE                  0x0C
#define LSM303DLHC_ACCEL_LOW_POWER_MODE         0x02
#define LSM303DLHC_ACCEL_HIGH_RESOLUTION_MODE   0x01
//Config Byte1
#define MPU9250_SAMPLING_RATE                   0xFF
//Config Byte2
#define LSM303DLHC_MAG_GAIN                     0xE0
#define LSM303DLHC_MAG_SAMPLING_RATE            0x1C
#define MPU9250_GYRO_RANGE                      0x03
//Config Byte3
#define MPU9250_ACCEL_RANGE                     0xC0
#define BMP180_PRESSURE_RESOLUTION              0x30
#define GSR_RANGE                               0x0E
#define EXP_POWER_ENABLE                        0x01
//Unused bits 3-0


//ADC initialisation mask
#define MASK_A_ACCEL       0x0001
#define MASK_VBATT         0x0002
#define MASK_EXT_A7        0x0004
#define MASK_EXT_A6        0x0008
#define MASK_EXT_A15       0x0010
#define MASK_INT_A1        0x0020
#define MASK_INT_A12       0x0040
#define MASK_INT_A13       0x0080
#define MASK_INT_A14       0x0100
#define MASK_MSP_TEMP      0x0200
#define MASK_STRAIN        0x0180   //uses ADC13 and ADC14


//LSM303DLHC Accel Range
//Corresponds to the FS field of the LSM303DLHC's CTRL_REG4_A register
//and the AFS_SEL field of the MPU9250's ACCEL_CONFIG register
#define ACCEL_2G      0x00
#define ACCEL_4G      0x01
#define ACCEL_8G      0x02
#define ACCEL_16G     0x03

//LSM303DLHC Accel Sampling Rate
//Corresponds to the ODR field of the LSM303DLHC's CTRL_REG1_A register
#define LSM303DLHC_ACCEL_POWER_DOWN 0x00
#define LSM303DLHC_ACCEL_1HZ        0x01
#define LSM303DLHC_ACCEL_10HZ       0x02
#define LSM303DLHC_ACCEL_25HZ       0x03
#define LSM303DLHC_ACCEL_50HZ       0x04
#define LSM303DLHC_ACCEL_100HZ      0x05
#define LSM303DLHC_ACCEL_200HZ      0x06
#define LSM303DLHC_ACCEL_400HZ      0x07
#define LSM303DLHC_ACCEL_1_620KHZ   0x08 //1.620kHz in Low-power mode only
#define LSM303DLHC_ACCEL_1_344kHz   0x09 //1.344kHz in normal mode, 5.376kHz in low-power mode

//LSM303DLHC Mag gain
#define LSM303DLHC_MAG_1_3G         0x01 //+/-1.3 Gauss
#define LSM303DLHC_MAG_1_9G         0x02 //+/-1.9 Gauss
#define LSM303DLHC_MAG_2_5G         0x03 //+/-2.5 Gauss
#define LSM303DLHC_MAG_4_0G         0x04 //+/-4.0 Gauss
#define LSM303DLHC_MAG_4_7G         0x05 //+/-4.7 Gauss
#define LSM303DLHC_MAG_5_6G         0x06 //+/-5.6 Gauss
#define LSM303DLHC_MAG_8_1G         0x07 //+/-8.1 Gauss

//LSM303DLHC Mag sampling rate
#define LSM303DLHC_MAG_0_75HZ       0x00 //0.75 Hz
#define LSM303DLHC_MAG_1_5HZ        0x01 //1.5 Hz
#define LSM303DLHC_MAG_3HZ          0x02 //3.0 Hz
#define LSM303DLHC_MAG_7_5HZ        0x03 //7.5 Hz
#define LSM303DLHC_MAG_15HZ         0x04 //15 Hz
#define LSM303DLHC_MAG_30HZ         0x05 //30 Hz
#define LSM303DLHC_MAG_75HZ         0x06 //75 Hz
#define LSM303DLHC_MAG_220HZ        0x07 //220 Hz


//calibration info
#define S_ACCEL_A                   0
#define S_GYRO                      1
#define S_MAG                       2
#define S_ACCEL_D                   3
//#define S_ECG                     3
//#define S_EMG                     4

//MPU9250 Gyro range
#define MPU9250_GYRO_250DPS         0x00 //+/-250 dps
#define MPU9250_GYRO_500DPS         0x01 //+/-500 dps
#define MPU9250_GYRO_1000DPS        0x02 //+/-1000 dps
#define MPU9250_GYRO_2000DPS        0x03 //+/-2000 dps

//#digital accel_range
#define RANGE_2G                    0
#define RANGE_4G                    1
#define RANGE_8G                    2
#define RANGE_16G                   3

//#mag_gain
#define LSM303_MAG_13GA             1
#define LSM303_MAG_19GA             2
#define LSM303_MAG_25GA             3
#define LSM303_MAG_40GA             4
#define LSM303_MAG_47GA             5
#define LSM303_MAG_56GA             6
#define LSM303_MAG_81GA             7



//SD Log file header format
#define SDHEAD_LEN                  256// 0-255

#define SDH_SAMPLE_RATE_0           0
#define SDH_SAMPLE_RATE_1           1
#define SDH_BUFFER_SIZE             2
#define SDH_SENSORS0                3
#define SDH_SENSORS1                4
#define SDH_SENSORS2                5
#define SDH_SENSORS3                6
#define SDH_SENSORS4                7
#define SDH_CONFIG_SETUP_BYTE0      8 //sensors setting bytes
#define SDH_CONFIG_SETUP_BYTE1      9
#define SDH_CONFIG_SETUP_BYTE2      10
#define SDH_CONFIG_SETUP_BYTE3      11
#define SDH_CONFIG_SETUP_BYTE4      12
#define SDH_CONFIG_SETUP_BYTE5      13
#define SDH_CONFIG_SETUP_BYTE6      14
#define SDH_TRIAL_CONFIG0           16
#define SDH_TRIAL_CONFIG1           17
#define SDH_BROADCAST_INTERVAL      18
#define SDH_BT_COMMS_BAUD_RATE      19
#define SDH_EST_EXP_LEN_MSB         20
#define SDH_EST_EXP_LEN_LSB         21
#define SDH_MAX_EXP_LEN_MSB         22
#define SDH_MAX_EXP_LEN_LSB         23
#define SDH_MAC_ADDR                24
#define SDH_SHIMMERVERSION_BYTE_0   30
#define SDH_SHIMMERVERSION_BYTE_1   31
#define SDH_MYTRIAL_ID              32
#define SDH_NSHIMMER                33
#define SDH_FW_VERSION_TYPE_0       34
#define SDH_FW_VERSION_TYPE_1       35
#define SDH_FW_VERSION_MAJOR_0      36
#define SDH_FW_VERSION_MAJOR_1      37
#define SDH_FW_VERSION_MINOR        38
#define SDH_FW_VERSION_INTERNAL     39
#define SDH_DERIVED_CHANNELS_0      40
#define SDH_DERIVED_CHANNELS_1      41
#define SDH_DERIVED_CHANNELS_2      42
#define SDH_RTC_DIFF_0              44
#define SDH_RTC_DIFF_1              45
#define SDH_RTC_DIFF_2              46
#define SDH_RTC_DIFF_3              47
#define SDH_RTC_DIFF_4              48
#define SDH_RTC_DIFF_5              49
#define SDH_RTC_DIFF_6              50
#define SDH_RTC_DIFF_7              51
#define SDH_CONFIG_TIME_0           52
#define SDH_CONFIG_TIME_1           53
#define SDH_CONFIG_TIME_2           54
#define SDH_CONFIG_TIME_3           55
#define SDH_EXG_ADS1292R_1_CONFIG1        56
#define SDH_EXG_ADS1292R_1_CONFIG2        57
#define SDH_EXG_ADS1292R_1_LOFF           58
#define SDH_EXG_ADS1292R_1_CH1SET         59
#define SDH_EXG_ADS1292R_1_CH2SET         60
#define SDH_EXG_ADS1292R_1_RLD_SENS       61
#define SDH_EXG_ADS1292R_1_LOFF_SENS      62
#define SDH_EXG_ADS1292R_1_LOFF_STAT      63
#define SDH_EXG_ADS1292R_1_RESP1          64
#define SDH_EXG_ADS1292R_1_RESP2          65
#define SDH_EXG_ADS1292R_2_CONFIG1        66
#define SDH_EXG_ADS1292R_2_CONFIG2        67
#define SDH_EXG_ADS1292R_2_LOFF           68
#define SDH_EXG_ADS1292R_2_CH1SET         69
#define SDH_EXG_ADS1292R_2_CH2SET         70
#define SDH_EXG_ADS1292R_2_RLD_SENS       71
#define SDH_EXG_ADS1292R_2_LOFF_SENS      72
#define SDH_EXG_ADS1292R_2_LOFF_STAT      73
#define SDH_EXG_ADS1292R_2_RESP1          74
#define SDH_EXG_ADS1292R_2_RESP2          75
#define SDH_LSM303DLHC_ACCEL_CALIBRATION  76 //0x4c
#define SDH_MPU9250_GYRO_CALIBRATION      97 //0x61
#define SDH_LSM303DLHC_MAG_CALIBRATION    118//0x76
#define SDH_A_ACCEL_CALIBRATION           139//0x8b
#define SDH_TEMP_PRES_CALIBRATION         160
#define SDH_MY_LOCALTIME_0TH              248
#define SDH_MY_LOCALTIME_5TH              251
#define SDH_MY_LOCALTIME                  252   //252-255



//SENSORS0
#define SDH_SENSOR_A_ACCEL             0x80
#define SDH_SENSOR_MPU9250_GYRO        0x40
#define SDH_SENSOR_LSM303DLHC_MAG      0x20
#define SDH_SENSOR_EXG1_24BIT          0x10
#define SDH_SENSOR_EXG2_24BIT          0x08
#define SDH_SENSOR_GSR                 0x04
#define SDH_SENSOR_EXTCH7              0x02
#define SDH_SENSOR_EXTCH6              0x01
//SENSORS1
#define SDH_SENSOR_STRAIN              0x80
//#define SDH_SENSOR_HR                0x40
#define SDH_SENSOR_VBATT               0x20
#define SDH_SENSOR_LSM303DLHC_ACCEL    0x10
#define SDH_SENSOR_EXTCH15             0x08
#define SDH_SENSOR_INTCH1              0x04
#define SDH_SENSOR_INTCH12             0x02
#define SDH_SENSOR_INTCH13             0x01
//SENSORS2
#define SDH_SENSOR_INTCH14             0x80
#define SDH_SENSOR_MPU9250_ACCEL       0x40
#define SDH_SENSOR_MPU9250_MAG         0x20
#define SDH_SENSOR_EXG1_16BIT          0x10
#define SDH_SENSOR_EXG2_16BIT          0x08
#define SDH_SENSOR_BMP180_PRES         0x04
//#define SDH_SENSOR_BMP180_TEMP       0x02
//SENSORS3
#define SDH_SENSOR_MSP430_TEMP         0x01
#define SDH_SENSOR_TCXO                0x80

//SDH_TRIAL_CONFIG0
#define SDH_IAMMASTER                  0x02
#define SDH_TIME_SYNC                  0x04
#define SDH_TIME_STAMP                 0x08// not used now, reserved as 1
#define SDH_RWCERROR_EN                0x10// when 0, won't flash error. when 1, will flash error if RTC isn't set (RTC_offset == 0)
//#define SDH_GYRO_BUTTON_ENABLE         0x10
#define SDH_USER_BUTTON_ENABLE         0x20
#define SDH_SET_PMUX                   0x40// not used now, reserved as 0
#define SDH_RTC_SET_BY_BT              0x80
//SDH_TRIAL_CONFIG1
#define SDH_SINGLETOUCH                0x80
//#define SDH_ACCEL_LPM                  0x40//config has this bit
//#define SDH_ACCEL_HRM                  0x20//config has this bit
#define SDH_TCXO                       0x10
//#define SDH_EXP_POWER                  0x08//config has this bit
//#define SDH_MONITOR                    0x04
#define SD_WRITE_BUF_SIZE  512
#define SD_HEAD_SIZE       256

#define MAX_CHARS       13
#define UINT32_LEN      11 // 10+1, where the last byte should be 0x00
#define UINT64_LEN      21 // 20+1, where the last byte should be 0x00
#define RESPONSE_PACKET_SIZE 1024//133


// BATTERY
#define BATT_LOW        0x01
#define BATT_MID        0x02
#define BATT_HIGH       0x04

//LSM303DLHC Accel Range
//Corresponds to the FS field of the LSM303DLHC's CTRL_REG4_A register
//and the AFS_SEL field of the MPU9150's ACCEL_CONFIG register
#define ACCEL_2G      0x00
#define ACCEL_4G      0x01
#define ACCEL_8G      0x02
#define ACCEL_16G     0x03

//LSM303DLHC Accel Sampling Rate
//Corresponds to the ODR field of the LSM303DLHC's CTRL_REG1_A register
#define LSM303DLHC_ACCEL_POWER_DOWN 0x00
#define LSM303DLHC_ACCEL_1HZ        0x01
#define LSM303DLHC_ACCEL_10HZ       0x02
#define LSM303DLHC_ACCEL_25HZ       0x03
#define LSM303DLHC_ACCEL_50HZ       0x04
#define LSM303DLHC_ACCEL_100HZ      0x05
#define LSM303DLHC_ACCEL_200HZ      0x06
#define LSM303DLHC_ACCEL_400HZ      0x07
#define LSM303DLHC_ACCEL_1_620KHZ   0x08 //1.620kHz in Low-power mode only
#define LSM303DLHC_ACCEL_1_344kHz   0x09 //1.344kHz in normal mode, 5.376kHz in low-power mode

//LSM303DLHC Mag gain
#define LSM303DLHC_MAG_1_3G         0x01 //+/-1.3 Gauss
#define LSM303DLHC_MAG_1_9G         0x02 //+/-1.9 Gauss
#define LSM303DLHC_MAG_2_5G         0x03 //+/-2.5 Gauss
#define LSM303DLHC_MAG_4_0G         0x04 //+/-4.0 Gauss
#define LSM303DLHC_MAG_4_7G         0x05 //+/-4.7 Gauss
#define LSM303DLHC_MAG_5_6G         0x06 //+/-5.6 Gauss
#define LSM303DLHC_MAG_8_1G         0x07 //+/-8.1 Gauss

//LSM303DLHC Mag sampling rate
#define LSM303DLHC_MAG_0_75HZ       0x00 //0.75 Hz
#define LSM303DLHC_MAG_1_5HZ        0x01 //1.5 Hz
#define LSM303DLHC_MAG_3HZ          0x02 //3.0 Hz
#define LSM303DLHC_MAG_7_5HZ        0x03 //7.5 Hz
#define LSM303DLHC_MAG_15HZ         0x04 //15 Hz
#define LSM303DLHC_MAG_30HZ         0x05 //30 Hz
#define LSM303DLHC_MAG_75HZ         0x06 //75 Hz
#define LSM303DLHC_MAG_220HZ        0x07 //220 Hz


//calibration info
#define S_ACCEL_A                   0
#define S_GYRO                      1
#define S_MAG                       2
#define S_ACCEL_D                   3
//#define S_ECG                     3
//#define S_EMG                     4

//MPU9150 Gyro range
#define MPU9150_GYRO_250DPS         0x00 //+/-250 dps
#define MPU9150_GYRO_500DPS         0x01 //+/-500 dps
#define MPU9150_GYRO_1000DPS        0x02 //+/-1000 dps
#define MPU9150_GYRO_2000DPS        0x03 //+/-2000 dps

//#digital accel_range
#define RANGE_2G                    0
#define RANGE_4G                    1
#define RANGE_8G                    2
#define RANGE_16G                   3

//#mag_gain
#define LSM303_MAG_13GA             1
#define LSM303_MAG_19GA             2
#define LSM303_MAG_25GA             3
#define LSM303_MAG_40GA             4
#define LSM303_MAG_47GA             5
#define LSM303_MAG_56GA             6
#define LSM303_MAG_81GA             7

//BMP Pressure oversampling ratio
#define BMP180_OSS_1                0x00
#define BMP180_OSS_2                0x01
#define BMP180_OSS_4                0x02
#define BMP180_OSS_8                0x03

//BtStream specific extension to range values : should SDLog keep it?
#define GSR_AUTORANGE               0x04

#define STAT_PERI_ADC               0x01
#define STAT_PERI_I2C_SENS          0x02
#define STAT_PERI_I2C_BATT          0x04
#define STAT_PERI_SPI_SENS          0x08
#define STAT_PERI_SDMMC             0x10
#define STAT_PERI_BT                0x20
#define PeriStat_Set(x) do{\
stat.periStat |= x;\
}while(0)
#define PeriStat_Clr(x) do{\
stat.periStat &= ~x;\
}while(0)

typedef volatile struct STATTypeDef_t{//STATUS
   uint8_t     isSensing;   
   uint8_t     isDocked;   
   uint8_t     isLogging;   
   uint8_t     isStreaming; 
   uint8_t     isConfiguring;   
   uint8_t     isButtonPressed;
   uint8_t     isBtConnected; 
   uint8_t     isBtPoweredOn;
   uint8_t     isSdInserted;
   uint8_t     isLedRed;
   uint8_t     isDockUartStep;   
   uint8_t     battStat;
   uint8_t     battStatLed;
   uint8_t     battVal[3];
   uint8_t     battDigital[10];
   uint8_t     badFile;
//   uint8_t     sdlogEn;
//   uint8_t     btstreamEn;
   uint8_t     sdlogCmd;
   uint8_t     btstreamCmd;
   uint8_t     toggleLedRedCmd;
   uint32_t    testResult;   
   uint8_t     pinPvI2c;   
   uint8_t     pinPvSd;   
   uint8_t     pinPvExt;   
   uint8_t     periStat;
} STATTypeDef;

//STATTypeDef * GetStatus(void);
extern STATTypeDef stat;

//typedef enum{//bt
//   UART_BT_STAT_IDLE = 0,
//   UART_BT_STAT_CONFIG = 1
//}UART_BT_STATUS;



#endif //S4_H
