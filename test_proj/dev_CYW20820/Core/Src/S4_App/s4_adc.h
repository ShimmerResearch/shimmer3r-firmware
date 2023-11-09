
#ifndef S4_ADC_H
#define S4_ADC_H

#include "stm32u5xx_hal.h"
//#include "main.h"

#include "s4.h"
#include "s4__cfg.h"

typedef struct {//adc - Sensors
   uint8_t rxBuf[130];
   uint8_t sensorList[20];
   uint8_t sensorLen;
   uint8_t sensorCnt;
   uint8_t chanCntResv;
   uint8_t chanCntSens;
   uint8_t chanCntBatt;
} ADCTypeDef;

#define ADC_CONFIG_NONE 0
#define ADC_CONFIG_SENS 1
#define ADC_CONFIG_BATT 2

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
void S4_NORM_ADC_initBatt(void);   
void S4_NORM_ADC_configureChannels(void);
void S4_NORM_ADC_startSensing(void);
void S4_NORM_ADC_bufPoll(void);
void S4_NORM_ADC_stopSensing(void);
void S4_NORM_ADC_gatherDataCb(void (*done_cb)(void));
void S4_NORM_ADC_gatherDataStart(void); 
void S4_NORM_ADC_readBatt(void);
void S4_NORM_ADC_rankBatt(void);

   
#endif /* S4_ADC_H */


