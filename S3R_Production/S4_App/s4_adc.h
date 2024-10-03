
#ifndef S4_ADC_H
#define S4_ADC_H

#include "shimmer_include.h"
#include "stm32u5xx_hal.h"
//#include "main.h"

#include "linked_list.h"
#include "shimmer_definitions.h"

typedef struct
{ //adc - Sensors
  uint8_t rxBuf[130];
  uint8_t sensorList[20];
  uint8_t sensorLen;
  uint8_t sensorCnt;
#if defined(SHIMMER4_SDK)
  uint8_t chanCntResv;
#endif
  uint8_t chanCntSens;
  uint8_t chanCntBatt;
} ADCTypeDef;

#define ADC_CONFIG_NONE 0
#define ADC_CONFIG_SENS 1
#define ADC_CONFIG_BATT 2

#define BATT_LOW_MAX    2618
#define BATT_MID_MIN    2568
#define BATT_MID_MAX    2767
#define BATT_HIGH_MIN   2717

#if USE_FREERTOS
#define S4_ADC_init              S4_RTOS_ADC_init
#define S4_ADC_init1             S4_RTOS_ADC_init1
#define S4_ADC_init2             S4_RTOS_ADC_init2
#define S4_ADC_initBatt          S4_RTOS_ADC_initBatt
#define S4_ADC_configureChannels S4_RTOS_ADC_configureChannels
#define S4_ADC_startSensing      S4_RTOS_ADC_startSensing
#define S4_ADC_bufPoll           S4_RTOS_ADC_bufPoll
#define S4_ADC_stopSensing       S4_RTOS_ADC_stopSensing
#define S4_ADC_gatherDataCb      S4_RTOS_ADC_gatherDataCb
#define S4_ADC_gatherDataStart   S4_RTOS_ADC_gatherDataStart
#define S4_ADC_readBatt          S4_RTOS_ADC_readBatt
#define S4_ADC_rankBatt          S4_RTOS_ADC_rankBatt
#else
#define S4_ADC_init              S4_NORM_ADC_init
#define S4_ADC_init1             S4_NORM_ADC_init1
#define S4_ADC_init2             S4_NORM_ADC_init2
#define S4_ADC_initBatt          S4_NORM_ADC_initBatt
#define S4_ADC_configureChannels S4_NORM_ADC_configureChannels
#define S4_ADC_startSensing      S4_NORM_ADC_startSensing
#define S4_ADC_bufPoll           S4_NORM_ADC_bufPoll
#define S4_ADC_stopSensing       S4_NORM_ADC_stopSensing
#define S4_ADC_gatherDataCb      S4_NORM_ADC_gatherDataCb
#define S4_ADC_gatherDataStart   S4_NORM_ADC_gatherDataStart
#define S4_ADC_readBatt          S4_NORM_ADC_readBatt
#define S4_ADC_rankBatt          S4_NORM_ADC_rankBatt
#endif //USE_FREERTOS

void S4_RTOS_ADC_init(void);
void S4_RTOS_ADC_init1(void);
void S4_RTOS_ADC_init2(void);
void S4_RTOS_ADC_initBatt(void);
void S4_RTOS_ADC_configureChannels(void);
void S4_RTOS_ADC_startSensing(void);
void S4_RTOS_ADC_bufPoll(void);
void S4_RTOS_ADC_stopSensing(void);
void S4_RTOS_ADC_gatherDataCb(void (*done_cb)(void));
void S4_RTOS_ADC_gatherDataStart(void);
void S4_RTOS_ADC_readBatt(void);
void S4_RTOS_ADC_rankBatt(void);

void S4_NORM_ADC_init(void);
void S4_NORM_ADC_init1(void);
void S4_NORM_ADC_init2(void);
#if defined(SHIMMER4_SDK)
void S4_NORM_ADC_initBatt(void);
#endif
void S4_NORM_ADC_configureChannels(void);
void S4_NORM_ADC_startSensing(void);
void S4_NORM_ADC_bufPoll(void);
void S4_NORM_ADC_stopSensing(void);
void S4_NORM_ADC_gatherDataCb(void (*done_cb)(void));
void S4_NORM_ADC_gatherDataStart(void);
void S4_NORM_ADC_readBatt(uint8_t isBlockingRead);
void S4_NORM_ADC_rankBatt(void);
void adcGpioInit(uint32_t pin, GPIO_TypeDef *port);
void manageReadBatt(uint8_t isBlockingRead);
void updateBatteryStatus(uint16_t adc_battVal);
battAlarmInterval_t getBatteryInterval(void);

#if defined(SHIMMER3R)
bool areAdcChannelsEnabled(void);
#endif

void setBatteryInterval(battAlarmInterval_t value);
void resetBatteryCriticalCount(void);

#endif /* S4_ADC_H */
