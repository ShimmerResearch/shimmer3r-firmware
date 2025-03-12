/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    i2c.c
 * @brief   This file provides code for the configuration
 *          of the I2C instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */
#include "shimmer_definitions.h"
#include "shimmer_include.h"

#define BOOT_TIME 20 //LIS2MDL = lis2dw12 = 20ms, LSM6DSV = 10

//static SENSINGTypeDef *pSensing;
//static STATTypeDef * pStat;

I2CTypeDef i2c1Sens;
I2CTypeDef i2c3Sens;
I2CTypeDef i2c4Sens;
I2CTypeDef i2cBatt;

uint8_t expectedI2cBusCbFlags = 0, currentI2cBusCbFlags = 0;

I2C_HandleTypeDef *hi2cMainBus;
#if defined(SHIMMER4_SDK)
I2C_HandleTypeDef *hi2cBattery;
#endif

#if defined(SHIMMER4_SDK)
MPU9250MagTypeDef mpu9250Mag;
BMP180TypeDef sensorBmp180;
BMP280TypeDef sensorBmp280;
struct bmp280_t bmp280;
#endif
i2cReadBufTypeDef i2cSens_buf;
#if defined(SHIMMER4_SDK)
uint8_t stc3100_buf[STC3100_DATA_LEN];
#endif

bool i2c1BusChipPwrFlags[I2C1_CHIP_QTY];

uint8_t i2c_addr_list[128], i2c_addr_list_len;
#if defined(SHIMMER4_SDK)
uint16_t i2c_batt_report_interval = I2C_BATT_REPORT_INTERVAL_DEFAULT;
#endif
//I2CBatteryTypeDef i2cBatt;

/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c4;
DMA_HandleTypeDef handle_GPDMA1_Channel10;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2010091A;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
   */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
   */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  HAL_I2C_RegisterCallback(&hi2c1, HAL_I2C_MEM_RX_COMPLETE_CB_ID, I2C1_MemRxCpltCallback);

  hi2cMainBus = &hi2c1;

  eepromInit(I2C_getHandlerSensor());
  lis2mdl_driver_init();

  HAL_Delay(BOOT_TIME);

  /* USER CODE END I2C1_Init 2 */
}

/* I2C4 init function */
void MX_I2C4_Init(void)
{

  /* USER CODE BEGIN I2C4_Init 0 */

  /* USER CODE END I2C4_Init 0 */

  /* USER CODE BEGIN I2C4_Init 1 */

  /* USER CODE END I2C4_Init 1 */
  hi2c4.Instance = I2C4;
  hi2c4.Init.Timing = 0x20303E5D;
  hi2c4.Init.OwnAddress1 = 0;
  hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c4.Init.OwnAddress2 = 0;
  hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
   */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
   */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C4_Init 2 */

  HAL_Delay(BOOT_TIME);

  /* USER CODE END I2C4_Init 2 */
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (i2cHandle->Instance == I2C1)
  {
    /* USER CODE BEGIN I2C1_MspInit 0 */

    /* USER CODE END I2C1_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB9     ------> I2C1_SDA
    PB8     ------> I2C1_SCL
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C1 DMA Init */
    /* GPDMA1_REQUEST_I2C1_RX Init */
    handle_GPDMA1_Channel10.Instance = GPDMA1_Channel10;
    handle_GPDMA1_Channel10.Init.Request = GPDMA1_REQUEST_I2C1_RX;
    handle_GPDMA1_Channel10.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel10.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel10.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel10.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel10.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel10.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel10.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel10.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel10.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel10.Init.TransferAllocatedPort
        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel10.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel10.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel10) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2cHandle, hdmarx, handle_GPDMA1_Channel10);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel10, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
    /* USER CODE BEGIN I2C1_MspInit 1 */

    /* USER CODE END I2C1_MspInit 1 */
  }
  else if (i2cHandle->Instance == I2C4)
  {
    /* USER CODE BEGIN I2C4_MspInit 0 */

    /* USER CODE END I2C4_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C4;
    PeriphClkInit.I2c4ClockSelection = RCC_I2C4CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOF_CLK_ENABLE();
    /**I2C4 GPIO Configuration
    PF14     ------> I2C4_SCL
    PF15     ------> I2C4_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C4;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* I2C4 clock enable */
    __HAL_RCC_I2C4_CLK_ENABLE();
    /* USER CODE BEGIN I2C4_MspInit 1 */

    /* USER CODE END I2C4_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *i2cHandle)
{

  if (i2cHandle->Instance == I2C1)
  {
    /* USER CODE BEGIN I2C1_MspDeInit 0 */

    /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB9     ------> I2C1_SDA
    PB8     ------> I2C1_SCL
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

    /* I2C1 DMA DeInit */
    HAL_DMA_DeInit(i2cHandle->hdmarx);

    /* I2C1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    /* USER CODE BEGIN I2C1_MspDeInit 1 */

    /* USER CODE END I2C1_MspDeInit 1 */
  }
  else if (i2cHandle->Instance == I2C4)
  {
    /* USER CODE BEGIN I2C4_MspDeInit 0 */

    /* USER CODE END I2C4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C4_CLK_DISABLE();

    /**I2C4 GPIO Configuration
    PF14     ------> I2C4_SCL
    PF15     ------> I2C4_SDA
    */
    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_14);

    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_15);

    /* USER CODE BEGIN I2C4_MspDeInit 1 */

    /* USER CODE END I2C4_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void I2C1_DeInit(void)
{
  HAL_I2C_DeInit(hi2cMainBus);
}

void I2C4_DeInit(void)
{
  HAL_I2C_DeInit(&hi2c4);
}

void I2C_scan_busses(void)
{
  I2C_scan(hi2cMainBus, i2c_addr_list, &i2c_addr_list_len);
#if defined(SHIMMER4_SDK)
  I2C_scan(hi2cBattery);
#endif
}

void I2C_scan_internal_expansion_bus(uint8_t *i2c_addr_list_ptr, uint8_t *i2c_addr_list_len_ptr)
{
  I2C_scan(&hi2c4, i2c_addr_list_ptr, i2c_addr_list_len_ptr);
}

void I2C_scan(I2C_HandleTypeDef *hi2c, uint8_t *i2c_addr_list_ptr, uint8_t *i2c_addr_list_len_ptr)
{
  uint8_t buf = 0;
  uint16_t i2c_addr;
  HAL_StatusTypeDef result;

  *i2c_addr_list_len_ptr = 0;
  for (i2c_addr = 0; i2c_addr < 0x80; i2c_addr++)
  {
    result = HAL_I2C_Master_Receive(hi2c, i2c_addr << 1, &buf, 1, 1);
    if (result == HAL_OK)
    {
      i2c_addr_list_ptr[(*i2c_addr_list_len_ptr)++] = i2c_addr;
    }
  }
}

I2C_HandleTypeDef *I2C_getHandlerSensor(void)
{
  return hi2cMainBus;
}
#if defined(SHIMMER4_SDK)
I2C_HandleTypeDef *I2C_getHandlerBatt(void)
{
  return hi2cBattery;
}

void I2C_readBatt(void)
{
  static uint16_t cnt = 0;
#if USE_I2C_VBATT_REPORT
  if (shimmerStatus.isSensing && S4Ram_getStoredConfig()->chEnStc3100)
  {
  }
  else if (i2c_batt_report_interval == 0)
  {
  }
  else
  {
    if (++cnt >= i2c_batt_report_interval)
    {
      cnt = 0;
      STC3100_readData((uint8_t *) shimmerStatus.battDigital);
      uint8_t bt_tx_data[131], packet_length = 0;
      *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
      *(bt_tx_data + packet_length++) = RSP_I2C_BATT_STATUS_COMMAND;
      memcpy((bt_tx_data + packet_length),
          (uint8_t *) shimmerStatus.battDigital, STC3100_DATA_LEN);
      packet_length += STC3100_DATA_LEN;
      BT_write(bt_tx_data, packet_length);
    }
  }
#endif
}

void I2C_readBattSetFreq(uint16_t val)
{
  i2c_batt_report_interval = val;
}
#endif

void I2C_configureChannels(void)
{
  I2cSens_configureChannels();
#if defined(SHIMMER4_SDK)
  I2cBatt_configureChannels();
#endif
}

void I2cSens_configureChannels(void)
{
  uint8_t *channel_contents_ptr = sensing.cc + sensing.ccLen;
  uint8_t nbr_i2c1_chans = 0;
  gConfigBytes *configBytes = S4Ram_getStoredConfig();

#if defined(SHIMMER3R)
  memset((uint8_t *) &i2c1Sens, 0, sizeof(i2c1Sens));
  memset((uint8_t *) &i2c3Sens, 0, sizeof(i2c3Sens));
  memset((uint8_t *) &i2c4Sens, 0, sizeof(i2c4Sens));

  i2c1Sens.busId = I2C1_BUS_FLAG;
  i2c3Sens.busId = I2C3_BUS_FLAG;
  i2c4Sens.busId = I2C4_BUS_FLAG;

  //Mag (LIS2MDL)
  if (configBytes->chEnAltMag)
  {
    *channel_contents_ptr++ = X_ALT_MAG;
    *channel_contents_ptr++ = Y_ALT_MAG;
    *channel_contents_ptr++ = Z_ALT_MAG;
    nbr_i2c1_chans += 3;
    sensing.ptr.mag1 = sensing.dataLen;
    sensing.dataLen += 6;
    i2c1Sens.sensorList[i2c1Sens.sensorLen++] = I2C_LIS2MDL_MAG;
  }
#elif defined(SHIMMER4_SDK)
  //Digi Gyro (MPU9250)
  if (configBytes->chEnGyro)
  {
    *channel_contents_ptr++ = X_GYRO;
    *channel_contents_ptr++ = Y_GYRO;
    *channel_contents_ptr++ = Z_GYRO;
    nbr_i2c1_chans += 3;
    sensing.ptr.gyro = sensing.dataLen;
    sensing.dataLen += 6;
    i2c1Sens.sensorList[i2c1Sens.sensorLen++] = I2C_MPU9250_GYRO;
  }
  //Digi Accel (LSM303DLHC)
  if (configBytes->chEnWrAccel)
  {
    *channel_contents_ptr++ = X_ACCEL_2;
    *channel_contents_ptr++ = Y_ACCEL_2;
    *channel_contents_ptr++ = Z_ACCEL_2;
    nbr_i2c1_chans += 3;
    sensing.ptr.accel2 = sensing.dataLen;
    sensing.dataLen += 6;
    i2c1Sens.sensorList[i2c1Sens.sensorLen++] = I2C_LSM303DLHC_ACCEL;
  }
  //Mag (LSM303DLHC)
  if (configBytes->chEnMag)
  {
    *channel_contents_ptr++ = X_MAG;
    *channel_contents_ptr++ = Z_MAG;
    *channel_contents_ptr++ = Y_MAG;
    nbr_i2c1_chans += 3;
    sensing.ptr.mag1 = sensing.dataLen;
    sensing.dataLen += 6;
    i2c1Sens.sensorList[i2c1Sens.sensorLen++] = I2C_LSM303DLHC_MAG;
  }
  //Digi Accel (MPU9250)
  if (configBytes->chEnAltAccel)
  {
    *channel_contents_ptr++ = X_ACCEL_3;
    *channel_contents_ptr++ = Y_ACCEL_3;
    *channel_contents_ptr++ = Z_ACCEL_3;
    nbr_i2c1_chans += 3;
    sensing.ptr.accel3 = sensing.dataLen;
    sensing.dataLen += 6;
    i2c1Sens.sensorList[i2c1Sens.sensorLen++] = I2C_MPU9250_ACCEL;
  }
  //Digi Mag (MPU9250)
  if (configBytes->chEnAltMag)
  {
    *channel_contents_ptr++ = X_MAG_2;
    *channel_contents_ptr++ = Y_MAG_2;
    *channel_contents_ptr++ = Z_MAG_2;
    nbr_i2c1_chans += 3;
    sensing.ptr.mag2 = sensing.dataLen;
    sensing.dataLen += 6;
    i2c1Sens.sensorList[i2c1Sens.sensorLen++] = I2C_MPU9250_MAG;
  }

  //Temp & Pressure sensor (BMP280)
  if (configBytes->chEnPressureAndTemperature)
  {
    *channel_contents_ptr++ = BMP_TEMPERATURE;
    *channel_contents_ptr++ = BMP_PRESSURE;
    nbr_i2c1_chans += 2; //TEMP & PRES, ON/OFF together
    sensing.ptr.temperature = sensing.dataLen;
    sensing.dataLen += 2;
    sensing.ptr.pressure = sensing.dataLen;
    sensing.dataLen += 3;
#if USE_BMPX80 == 1
    i2c1Sens.sensorList[i2c1Sens.sensorLen++] = I2C_BMP180;
#else
    i2c1Sens.sensorList[i2c1Sens.sensorLen++] = I2C_BMP280;
#endif
  }
#endif
  sensing.ccLen += nbr_i2c1_chans;
  sensing.nbrDigiChans += nbr_i2c1_chans;

#if defined(SHIMMER3R)
  expectedI2cBusCbFlags = 0;
  if (i2c1Sens.sensorLen > 0)
  {
    expectedI2cBusCbFlags |= I2C1_BUS_FLAG;
  }
  if (i2c3Sens.sensorLen > 0)
  {
    expectedI2cBusCbFlags |= I2C3_BUS_FLAG;
  }
  if (i2c4Sens.sensorLen > 0)
  {
    expectedI2cBusCbFlags |= I2C4_BUS_FLAG;
  }
#endif
}

#if defined(SHIMMER4_SDK)
void I2cBatt_configureChannels(void)
{
  uint8_t *channel_contents_ptr = sensing.cc + sensing.ccLen;
  uint8_t nbr_i2c_batt_chans = 0;
  i2cBatt.sensorLen = i2cBatt.sensorCnt = 0;
  //Digi Battery IC (STC3100)
  if (S4Ram_getStoredConfig()->chEnStc3100)
  {
    //All 5 channels - charge, counter, current, voltage, temperature
    *channel_contents_ptr++ = STC3100_CH_1;
    *channel_contents_ptr++ = STC3100_CH_2;
    *channel_contents_ptr++ = STC3100_CH_3;
    *channel_contents_ptr++ = STC3100_CH_4;
    *channel_contents_ptr++ = STC3100_CH_5;
    nbr_i2c_batt_chans += 5;
    sensing.ptr.stc3100Batt = sensing.dataLen;
    sensing.dataLen += STC3100_DATA_LEN;
    i2cBatt.sensorList[i2cBatt.sensorLen++] = I2C_STC3100;
  }
  sensing.nbrDigiChans += nbr_i2c_batt_chans;
  sensing.ccLen += nbr_i2c_batt_chans;
}
#endif

void I2C_startSensing(void)
{
  gConfigBytes *configBytes = S4Ram_getStoredConfig();
  float shimmerSamplingFreq = get_shimmer_sampling_freq();

  memset((uint8_t *) &i2cSens_buf, 0, sizeof(i2cReadBufTypeDef));

  //if ((0 != i2cSens.sensorLen)
  //    && (HAL_GPIO_ReadPin(SW_I2C1_GPIO_Port, SW_I2C1_Pin) == GPIO_PIN_RESET))
  //{
  //  //HAL_GPIO_WritePin(SW_I2C_GPIO_Port, SW_I2C_Pin, GPIO_PIN_SET);//I2C
  //  Board_SW_I2C(1);
  //  HAL_Delay(1000);
  //}

  if (i2c1Sens.sensorLen > 0)
  {
    MX_I2C1_Init();
  }

#if defined(SHIMMER3R)
  if (configBytes->chEnAltMag)
  {
    lis2mdl_configure(shimmerSamplingFreq, configBytes->altMagRate);
  }

#elif defined(SHIMMER4_SDK)
  if (configBytes->chEnStc3100)
  {
    STC3100_wake(1);
  }

  if (configBytes->chEnGyro || configBytes->chEnAltAccel || configBytes->chEnAltMag)
  {
    MPU9250_init(hi2cMainBus);
    if (configBytes->chEnGyro || configBytes->chEnAltAccel)
    {
      MPU9250_wake(1);
      MPU9250_setSamplingRate(configBytes->gyroRate);
    }
    if (configBytes->chEnGyro)
    {
      //This needs to go after the wake?
      MPU9250_setGyroSensitivity(configBytes->gyroRange);
    }
    if (configBytes->chEnAltAccel)
    {
      MPU9250_setAccelRange(configBytes->altAccelRange);
    }
    if (configBytes->chEnAltMag)
    {
      MPU9250MagSetup();
    }
  }

  if (configBytes->chEnMag || configBytes->chEnWrAccel)
  {
    LSM303DLHC_init(hi2cMainBus);
    if (configBytes->chEnWrAccel)
    {
      LSM303DLHC_accelInit(configBytes->wrAccelRate, //sampling rate
          configBytes->wrAccelRange,                 //range
          configBytes->wrAccelLPM,                   //low power mode
          configBytes->wrAccelHRM);                  //high resolution mode
    }
    if (configBytes->chEnMag)
    {
      LSM303DLHC_magInit(get_config_byte_mag_rate(), //sampling rate
          configBytes->magRange);                    //gain
    }
  }

  if (configBytes->chEnPressureAndTemperature)
  {
#if USE_BMPX80 == 1
    BMP180Setup();
#else //USE_BMPX80 == 2
    BMP280Setup();
#endif
  }
#endif
}

void I2C_pollSensors(void)
{
  if (i2c1Sens.sensorLen > 0)
  {
    I2cSensing(&i2c1Sens, I2C_FIRST_SENSOR);
  }
  if (i2c3Sens.sensorLen > 0)
  {
    I2cSensing(&i2c3Sens, I2C_FIRST_SENSOR);
  }
  if (i2c4Sens.sensorLen > 0)
  {
    I2cSensing(&i2c4Sens, I2C_FIRST_SENSOR);
  }
}

void I2C_stopSensing(void)
{
  I2cSens_stopSensing();
#if defined(SHIMMER4_SDK)
  I2cBatt_stopSensing();
#endif

  //HAL_I2C_MspDeInit(hi2cMainBus);//this may save .2-.3 mA?
  //HAL_I2C_MspDeInit(hi2cBattery);//this may save .2-.3 mA?
}

void I2cSens_stopSensing(void)
{
  gConfigBytes *configBytes = S4Ram_getStoredConfig();

#if defined(SHIMMER3R)
//if (configBytes->chEnMag)
//{
//  lis2mdl_sleep();
//}
#elif defined(SHIMMER4_SDK)
  if (configBytes->chEnGyro || configBytes->chEnAltAccel || configBytes->chEnAltMag)
  {
    MPU9250_wake(0);
  }
  if (configBytes->chEnMag || configBytes->chEnWrAccel)
  {
    LSM303DLHC_sleep();
  }
  HAL_Delay(10);
  Board_SW_I2C(0);
#endif

  I2C1_DeInit();
}

#if defined(SHIMMER4_SDK)
void I2cBatt_stopSensing(void)
{
  if (S4Ram_getStoredConfig()->chEnStc3100)
  {
    STC3100_wake(0);
  }
}
#endif

//================================================
//sensor codes
//================================================

void (*I2cSens_gatherDataDone_cb)(void);
#if defined(SHIMMER4_SDK)
void (*I2cBatt_gatherDataDone_cb)(void);
#endif

void I2cSens_gatherDataCb(void (*done_cb)(void))
{
  I2cSens_gatherDataDone_cb = done_cb;
}

#if defined(SHIMMER3R)
void I2C_busGatherDataDone_cb(uint8_t flag)
{
  //if all I2C buses complete, call main callback to sensing.c
  currentI2cBusCbFlags |= flag;
  if (currentI2cBusCbFlags == expectedI2cBusCbFlags)
  {
    I2cSens_gatherDataDone_cb();
  }
}
#endif

#if defined(SHIMMER4_SDK)
void I2C_gatherDataStart(void)
{
  I2cSens_gatherDataStart();
}

void I2cSens_gatherDataStart(void)
{
  I2cSensing(&i2c1Sens, I2C_FIRST_SENSOR);
}

void I2cBatt_gatherDataCb(void (*done_cb)(void))
{
  I2cBatt_gatherDataDone_cb = done_cb;
}

void I2cBatt_gatherDataStart(void)
{
  //I2cBatt_gatherDataDone_cb();
  I2cBattMonitor(I2C_FIRST_SENSOR);
}
#endif

void I2cSensing(I2CTypeDef *i2cSensingInfo, I2C_SENSING_TYPE start)
{
  i2cSensingInfo->sensorCnt
      = (start == I2C_FIRST_SENSOR) ? 0 : i2cSensingInfo->sensorCnt + 1;
  if (i2cSensingInfo->sensorCnt == i2cSensingInfo->sensorLen)
  {
    i2cSensingInfo->status = I2C_STAT_IDLE;
    i2cSensingInfo->sensorCnt = 0;
    I2C_busGatherDataDone_cb(i2cSensingInfo->busId);
  }
  else if (i2cSensingInfo->sensorCnt < i2cSensingInfo->sensorLen)
  {
    uint8_t waitingDmaRxCb = 0;
    while ((waitingDmaRxCb = I2cSens_sensorNext(i2cSensingInfo)) == 0)
    {
      i2cSensingInfo->sensorCnt++;

      if (i2cSensingInfo->sensorCnt == i2cSensingInfo->sensorLen)
      {
        i2cSensingInfo->status = I2C_STAT_IDLE;
        i2cSensingInfo->sensorCnt = 0;
        I2C_busGatherDataDone_cb(i2cSensingInfo->busId);
        break;
      }
    }
  }
  else
  {
    //TODO handle this differently
    while (1)
    {
      Board_ledToggle(LED_ALL);
      HAL_Delay(100);
    }
  }
}

#if defined(SHIMMER4_SDK)
void I2cBattMonitor(I2C_SENSING_TYPE start)
{
  i2cBatt.sensorCnt = (start == I2C_FIRST_SENSOR) ? 0 : i2cBatt.sensorCnt + 1;
  if (i2cBatt.sensorCnt == i2cBatt.sensorLen)
  {
    i2cBatt.status = I2C_STAT_IDLE;
    I2cBatt_gatherDataDone_cb();
  }
  else if (i2cBatt.sensorCnt < i2cBatt.sensorLen)
  {
    I2cBatt_sensorNext();
  }
  else
  {
    while (1)
    {
      Board_ledToggle(LED_ALL);
      HAL_Delay(100);
    }
  }
}
#endif

uint8_t I2cSens_sensorNext(I2CTypeDef *i2cSensingInfo)
{
  uint8_t retVal = 0;

  switch (i2cSensingInfo->sensorList[i2cSensingInfo->sensorCnt])
  {
#if defined(SHIMMER3R)
  case I2C_LIS2MDL_MAG:
    if (!lis2mdl_is_drdy_int_enabled() || LIS2MDL_DRDY)
    {
      i2cSensingInfo->status = I2C_STAT_LIS2MDL_MAG_GET;
      lis2mdl_mag_get(i2cSens_buf.lis2mdlMagBuf);
      retVal = 1;
    }
    break;
#elif defined(SHIMMER4_SDK)
  case I2C_LSM303DLHC_ACCEL:
    Lsm303dlhcAccelSample();
    break;
  case I2C_LSM303DLHC_MAG:
    Lsm303dlhcMagSample();
    break;
  case I2C_MPU9250_GYRO:
    MPU9250GyroSample();
    break;
  case I2C_MPU9250_ACCEL:
    MPU9250AccelSample();
    break;
  case I2C_MPU9250_MAG:
    MPU9250MagSample();
    break;
  case I2C_BMP180:
    BMP180Sample();
    break;
  case I2C_BMP280:
    BMP280Sample();
    break;
#endif
  default:
    break;
  }
  return retVal;
}

#if defined(SHIMMER3R)
bool areI2cChannelsEnabled(void)
{
  return (i2c1Sens.sensorLen + i2c3Sens.sensorLen + i2c4Sens.sensorLen) > 0 ? true : false;
}

void I2C1_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  switch (i2c1Sens.sensorList[i2c1Sens.sensorCnt])
  {
  case I2C_LIS2MDL_MAG:
    memcpy(sensing.dataBuf + sensing.ptr.mag1, &i2cSens_buf.lis2mdlMagBuf[0],
        sizeof(i2cSens_buf.lis2mdlMagBuf));
    break;
  default:
    break;
  }

  i2c1Sens.status = I2C_STAT_IDLE;
  I2cSensing(&i2c1Sens, I2C_NEXT_SENSOR);
}

#elif defined(SHIMMER4_SDK)
void I2cBatt_sensorNext(void)
{
  switch (i2cBatt.sensorList[i2cBatt.sensorCnt])
  {
  case I2C_STC3100:
    STC3100Sample();
    break;
  default:
    break;
  }
}

void BMP180Setup(void)
{
  BMP180_init(hi2cMainBus);

  sensorBmp180.en = 1;
  sensorBmp180.oss = S4Ram_getStoredConfig()->pressurePrecision;
  switch (sensorBmp180.oss)
  {
  case 0:
    sensorBmp180.preSampleClkInterval = 45;
    break;
  case 1:
    sensorBmp180.preSampleClkInterval = 75;
    break;
  case 2:
    sensorBmp180.preSampleClkInterval = 135;
    break;
  case 3:
    sensorBmp180.preSampleClkInterval = 255;
    break;
  default:
    sensorBmp180.preSampleClkInterval = 45;
    break;
  }

  BMP180_tempStartMeasurement();
  sensorBmp180.presCnt = sensorBmp180.presMax = (uint16_t) ceil(
      (float) sensorBmp180.preSampleClkInterval / (float) sensing.clkInterval4096);

  sensorBmp180.tempMax = sensing.freq < 3 ?
      1 :
      (uint16_t) (sensing.freq / (float) sensorBmp180.presMax - 1);
  sensorBmp180.tempCnt = 0;
}

void BMP180Sample(void)
{
  if (!--sensorBmp180.presCnt)
  { //--sensorBmp180.presCnt or sensorBmp180.presCnt--?
    if (!sensorBmp180.tempCnt--)
    {
      sensorBmp180.tempCnt = sensorBmp180.tempMax;
      i2c1Sens.status = I2C_STAT_BMP180_TEMP_GET_T;
      BMP180_tempReadTx();
    }
    else
    {
      i2c1Sens.status = I2C_STAT_BMP180_PRES_GET_T;
      BMP180_presReadTx();
    }
    sensorBmp180.presCnt = sensorBmp180.presMax;
  }
}

void BMP180TxDoneHandler(void)
{
  if (i2c1Sens.status == I2C_STAT_BMP180_TEMP_START)
  {
    I2cSensing(I2C_NEXT_SENSOR); //goto next sensor
  }
  else if (i2c1Sens.status == I2C_STAT_BMP180_TEMP_GET_T)
  {
    i2c1Sens.status = I2C_STAT_BMP180_TEMP_GET_R;
    BMP180_tempReadRx(sensing.dataBuf + sensing.ptr.temperature);
  }
  else if (i2c1Sens.status == I2C_STAT_BMP180_PRES_START)
  {
    I2cSensing(I2C_NEXT_SENSOR); //goto next sensor
  }
  else if (i2c1Sens.status == I2C_STAT_BMP180_PRES_GET_T)
  {
    i2c1Sens.status = I2C_STAT_BMP180_PRES_GET_R;
    BMP180_presReadRx(sensing.dataBuf + sensing.ptr.pressure);
  }
  else
  {
    i2c1Sens.status = I2C_STAT_IDLE;
  }
}

void BMP180RxDoneHandler(void)
{
  if (i2c1Sens.status == I2C_STAT_BMP180_TEMP_GET_R)
  {
    if (shimmerStatus.isSensing)
    {
      i2c1Sens.status = I2C_STAT_BMP180_PRES_START;
      BMP180_presStartMeasurement(sensorBmp180.oss);
    }
    else
    {
      i2c1Sens.status = I2C_STAT_IDLE;
    }
  }
  else if (i2c1Sens.status == I2C_STAT_BMP180_PRES_GET_R)
  {
    if (shimmerStatus.isSensing)
    {
      if (!sensorBmp180.tempCnt)
      {
        i2c1Sens.status = I2C_STAT_BMP180_TEMP_START;
        BMP180_tempStartMeasurement();
      }
      else
      {
        i2c1Sens.status = I2C_STAT_BMP180_PRES_START;
        BMP180_presStartMeasurement(sensorBmp180.oss);
      }
    }
    else
    {
      i2c1Sens.status = I2C_STAT_IDLE;
    }
  }
  else
  {
    i2c1Sens.status = I2C_STAT_IDLE;
  }
}

void BMP280Setup(void)
{
  uint8_t val;
  sensorBmp280.en = 1;
  //use values 0,1,2,3 to represent mode 0,2,3,4. mode 1 abandoned
  val = S4Ram_getStoredConfig()->pressurePrecision;
  val = 0;
  bmp280.working_mode = (val == 0) ? 0 : val + 1;
  bmp280.dev_addr = BMP280_I2C_ADDRESS1 << 1;
  bmp280.hi2c = hi2cMainBus;
  bmp280_init(&bmp280);

  switch (bmp280.working_mode)
  {
  case 0:
    sensorBmp280.preSampleClkInterval = 64;
    break; //64+10? for BMP280_STANDBY_TIME_1_MS
  case 1:
    sensorBmp280.preSampleClkInterval = 87;
    break;
  case 2:
    sensorBmp280.preSampleClkInterval = 133;
    break;
  case 3:
    sensorBmp280.preSampleClkInterval = 225;
    break;
  case 4:
    sensorBmp280.preSampleClkInterval = 432;
    break;
  default:
    sensorBmp280.preSampleClkInterval = 432;
    break;
  }

  sensorBmp280.presCnt = sensorBmp280.presMax = (uint16_t) ceil(
      (float) sensorBmp280.preSampleClkInterval / (float) sensing.clkInterval4096);

  sensorBmp280.tempMax = sensing.freq < 3 ?
      1 :
      (uint16_t) (sensing.freq / (float) sensorBmp280.presMax - 1);
  sensorBmp280.tempCnt = 0;
}

void BMP280Sample(void)
{
  if (!--sensorBmp280.presCnt)
  {
    if (!sensorBmp280.tempCnt--)
    {
      sensorBmp280.tempCnt = sensorBmp280.tempMax;
      i2c1Sens.status = I2C_STAT_BMP280_TEMP_GET_T;
      BMP280_tempReadTx();
    }
    else
    {
      i2c1Sens.status = I2C_STAT_BMP280_PRES_GET_T;
      BMP280_presReadTx();
    }
    sensorBmp280.presCnt = sensorBmp280.presMax;
  }
}

void BMP280TxDoneHandler(void)
{
  if (i2c1Sens.status == I2C_STAT_BMP280_START)
  {
    I2cSensing(I2C_NEXT_SENSOR); //goto next sensor
  }
  else if (i2c1Sens.status == I2C_STAT_BMP280_TEMP_GET_T)
  {
    i2c1Sens.status = I2C_STAT_BMP280_TEMP_GET_R;
    BMP280_tempReadRx();
  }
  else if (i2c1Sens.status == I2C_STAT_BMP280_PRES_GET_T)
  {
    i2c1Sens.status = I2C_STAT_BMP280_PRES_GET_R;
    BMP280_presReadRx();
  }
  else
  {
    i2c1Sens.status = I2C_STAT_IDLE;
  }
}

void BMP280RxDoneHandler(void)
{
  if (i2c1Sens.status == I2C_STAT_BMP280_TEMP_GET_R)
  {
    BMP280_tempReadDone(sensing.dataBuf + sensing.ptr.temperature);
    I2cSensing(I2C_NEXT_SENSOR);
  }
  else if (i2c1Sens.status == I2C_STAT_BMP280_PRES_GET_R)
  {
    BMP280_presReadDone(sensing.dataBuf + sensing.ptr.pressure);
    i2c1Sens.status = I2C_STAT_BMP280_START;
    BMP280_startTx();
    I2cSensing(I2C_NEXT_SENSOR);
  }
  else
  {
    i2c1Sens.status = I2C_STAT_IDLE;
  }
}

void MPU9250GyroSample(void)
{
  i2c1Sens.status = I2C_STAT_MPU9250_GYRO_GET_T;
  MPU9250_gyroReadStart();
}

void MPU9250GyroTxDoneHandler(void)
{
  i2c1Sens.status = I2C_STAT_MPU9250_GYRO_GET_R;
  //MPU9250_gyroReadDone(sensing.dataBuf + sensing.ptr.mpu9250Gyro);
  MPU9250_gyroReadDone(i2cSens_buf.mpu9250GyroBuf);
}

void MPU9250GyroRxDoneHandler(void)
{
  memcpy(sensing.dataBuf + sensing.ptr.gyro, i2cSens_buf.mpu9250GyroBuf, 6);
  i2c1Sens.status = I2C_STAT_IDLE;
  I2cSensing(I2C_NEXT_SENSOR); //goto next sensor
}

void MPU9250AccelSample(void)
{
  i2c1Sens.status = I2C_STAT_MPU9250_ACCEL_GET_T;
  MPU9250_accelReadStart();
}

void MPU9250AccelTxDoneHandler(void)
{
  i2c1Sens.status = I2C_STAT_MPU9250_ACCEL_GET_R;
  //MPU9250_accelReadDone(sensing.dataBuf + sensing.ptr.mpu9250Accel);
  MPU9250_accelReadDone(i2cSens_buf.mpu9250AccelBuf);
}

void MPU9250AccelRxDoneHandler(void)
{
  memcpy(sensing.dataBuf + sensing.ptr.accel3, i2cSens_buf.mpu9250AccelBuf, 6);
  i2c1Sens.status = I2C_STAT_IDLE;
  I2cSensing(I2C_NEXT_SENSOR); //goto next sensor
}

void MPU9250MagSetup(void)
{
  //max of approx. 3ms to sample everything + 9ms between starting mag to data ready
  mpu9250Mag.en = 1;
  mpu9250Mag.preSampleClkInterval = 90;

  //MPU9250_magStartSingleMeasurement();
  MPU9250_magStart();
  mpu9250Mag.cnt = mpu9250Mag.max = (uint16_t) ceil(
      (float) mpu9250Mag.preSampleClkInterval / (float) sensing.clkInterval4096);
}

void MPU9250MagSample(void)
{
  i2c1Sens.status = I2C_STAT_MPU9250_MAG_GET_T;
  MPU9250_magReadStart();
  //if (0 == --mpu9250Mag.cnt) {
  //  i2cSens.status = I2C_STAT_MPU9250_MAG_GET_T;
  //  MPU9250_magReadStart();
  //  mpu9250Mag.cnt = mpu9250Mag.max;
  //}
}

void MPU9250MagTxDoneHandler(void)
{
  i2c1Sens.status = I2C_STAT_MPU9250_MAG_GET_R;
  MPU9250_magReadDone(i2cSens_buf.mpu9250MagBuf);
  //if (I2C_STAT_MPU9250_MAG_START == i2cSens.status) {
  //  I2cSensing(I2C_NEXT_SENSOR); // goto next sensor
  //} else if (I2C_STAT_MPU9250_MAG_GET_T == i2cSens.status) {
  //  i2cSens.status = I2C_STAT_MPU9250_MAG_GET_R;
  //  //MPU9250_magReadDone(sensing.dataBuf + sensing.ptr.mpu9250Mag);
  //  MPU9250_magReadDone(i2cSens_buf.mpu9250MagBuf);
  //} else {
  //  i2cSens.status = I2C_STAT_IDLE;
  //}
}

void MPU9250MagRxDoneHandler(void)
{
  memcpy(sensing.dataBuf + sensing.ptr.mag2, i2cSens_buf.mpu9250MagBuf, 6);
  i2c1Sens.status = I2C_STAT_IDLE;
  I2cSensing(I2C_NEXT_SENSOR); //goto next sensor

  //if (I2C_STAT_MPU9250_MAG_GET_R == i2cSens.status) {
  //  memcpy(sensing.dataBuf + sensing.ptr.mpu9250Mag,
  //  i2cSens_buf.mpu9250MagBuf, 6); i2cSens.status = I2C_STAT_IDLE;
  //  //MPU9250_magStartSingleMeasurement();
  //} else {
  //  i2cSens.status = I2C_STAT_IDLE;
  //}
}

void Lsm303dlhcAccelSample(void)
{
  i2c1Sens.status = I2C_STAT_LSM303DLHC_ACCEL_GET_T;
  LSM303DLHC_accelReadStart();
}

void Lsm303dlhcAccelTxDoneHandler(void)
{
  i2c1Sens.status = I2C_STAT_LSM303DLHC_ACCEL_GET_R;
  //LSM303DLHC_accelReadDone(sensing.dataBuf + sensing.ptr.lsm303dlhcAccel);
  LSM303DLHC_accelReadDone(i2cSens_buf.lsm303AccelBuf);
}

void Lsm303dlhcAccelRxDoneHandler(void)
{
  memcpy(sensing.dataBuf + sensing.ptr.accel2, i2cSens_buf.lsm303AccelBuf, 6);
  i2c1Sens.status = I2C_STAT_IDLE;
  I2cSensing(I2C_NEXT_SENSOR); //goto next sensor
}

void Lsm303dlhcMagSample(void)
{
  i2c1Sens.status = I2C_STAT_LSM303DLHC_MAG_GET_T;
  LSM303DLHC_magReadStart();
}

void Lsm303dlhcMagTxDoneHandler(void)
{
  i2c1Sens.status = I2C_STAT_LSM303DLHC_MAG_GET_R;
  //LSM303DLHC_magReadDone(sensing.dataBuf + sensing.ptr.lsm303dlhcMag);
  LSM303DLHC_magReadDone(i2cSens_buf.lsm303MagBuf);
}

void Lsm303dlhcMagRxDoneHandler(void)
{
  memcpy(sensing.dataBuf + sensing.ptr.mag1, i2cSens_buf.lsm303MagBuf, 6);
  i2c1Sens.status = I2C_STAT_IDLE;
  I2cSensing(I2C_NEXT_SENSOR); //goto next sensor
}

void STC3100Sample(void)
{
  i2cBatt.status = I2C_STAT_STC3100_DATA_GET;
  STC3100_readData_it(stc3100_buf);
}

void STC3100BatteryRxDoneHandler(void)
{
  //static uint8_t i=0;
  memcpy(sensing.dataBuf + sensing.ptr.stc3100Batt, stc3100_buf, STC3100_DATA_LEN);
  i2cBatt.status = I2C_STAT_IDLE;
  I2cBattMonitor(I2C_NEXT_SENSOR);
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == hi2cMainBus->Instance)
  {
    switch (i2c1Sens.sensorList[i2c1Sens.sensorCnt])
    {
    case I2C_BMP180:
      BMP180TxDoneHandler();
      break;
    case I2C_BMP280:
      BMP280TxDoneHandler();
      break;
    case I2C_MPU9250_GYRO:
      MPU9250GyroTxDoneHandler();
      break;
    case I2C_MPU9250_ACCEL:
      MPU9250AccelTxDoneHandler();
      break;
    case I2C_MPU9250_MAG:
      MPU9250MagTxDoneHandler();
      break;
    case I2C_LSM303DLHC_ACCEL:
      Lsm303dlhcAccelTxDoneHandler();
      break;
    case I2C_LSM303DLHC_MAG:
      Lsm303dlhcMagTxDoneHandler();
      break;
    default:
      break;
    }
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == hi2cMainBus->Instance)
  {
    switch (i2c1Sens.sensorList[i2c1Sens.sensorCnt])
    {
    case I2C_BMP180:
      BMP180RxDoneHandler();
      break;
    case I2C_BMP280:
      BMP280RxDoneHandler();
      break;
    case I2C_MPU9250_GYRO:
      MPU9250GyroRxDoneHandler();
      break;
    case I2C_MPU9250_ACCEL:
      MPU9250AccelRxDoneHandler();
      break;
    case I2C_MPU9250_MAG:
      MPU9250MagRxDoneHandler();
      break;
    case I2C_LSM303DLHC_ACCEL:
      Lsm303dlhcAccelRxDoneHandler();
      break;
    case I2C_LSM303DLHC_MAG:
      Lsm303dlhcMagRxDoneHandler();
      break;
    default:
      break;
    }
  }
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  //if(hi2c->Instance == hi2cBattery->Instance){
  //  switch(i2cBatt.sensorList[i2cBatt.sensorCnt]){
  //  case I2C_STC3100:          STC3100BatteryTxDoneHandler();    break;
  //  default: break;
  //  }
  //}
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == hi2cBattery->Instance)
  {
    switch (i2cBatt.sensorList[i2cBatt.sensorCnt])
    {
    case I2C_STC3100:
      STC3100BatteryRxDoneHandler();
      break;
    default:
      break;
    }
  }
}
#endif

void loadDaughterCardIdFromEeprom(void)
{
  uint8_t daughterCardIdBuf[CAT24C16_PAGE_SIZE];
  eepromRead(0, CAT24C16_PAGE_SIZE, &daughterCardIdBuf[0]);
  setDaugherCardIdPage(daughterCardIdBuf);
  parseDaughterCardId(getDaughtCardId()->exp_brd_id);
  HAL_Delay(5); //5ms to ensure no writes pending
}

void enableI2cOnInternalExpansionBrd(uint8_t state)
{
  if (state)
  {
    Board_SW_EXP_BRD_POWER(1);
#ifdef SR48_6_0
    Board_SW_I2C4_ON_PPG(1);
#else
    swI2C4PpgOnAds7028(1);
#endif
    MX_I2C4_Init();
  }
  else
  {
    I2C4_DeInit();
    Board_SW_EXP_BRD_POWER(0);
#ifdef SR48_6_0
    Board_SW_I2C4_ON_PPG(0);
#else
    swI2C4PpgOnAds7028(0);
#endif
  }
}

/* USER CODE END 1 */
