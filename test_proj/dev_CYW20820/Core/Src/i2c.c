/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "../Src/S4_App/s4__cfg.h"


//static SENSINGTypeDef *pSensing;
//static STATTypeDef * pStat;


I2CTypeDef i2cSens;
//I2CTypeDef i2cBatt; commented for porting

I2C_HandleTypeDef *hi2cSensor;
//I2C_HandleTypeDef *hi2cBattery; commented for porting

MPU9250MagTypeDef mpu9250Mag;
BMP180TypeDef sensorBmp180;
BMP280TypeDef sensorBmp280;
//struct bmp280_t bmp280; Commented for porting
i2cReadBufTypeDef i2cSens_buf;
//uint8_t stc3100_buf[STC3100_DATA_LEN]; commented for porting

uint8_t i2c_addr_list[128], i2c_addr_list_len;
uint16_t i2c_batt_report_interval = I2C_BATT_REPORT_INTERVAL_DEFAULT;
//I2CBatteryTypeDef i2cBatt;

/* USER CODE END 0 */

//I2C_HandleTypeDef i2c1;
/*I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;
I2C_HandleTypeDef hi2c4;*/  //commented for porting
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;
/*DMA_HandleTypeDef hdma_i2c2_rx;
DMA_HandleTypeDef hdma_i2c2_tx;*/

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00200E34;
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

  /* USER CODE END I2C1_Init 2 */

}

//commented for porting

/* I2C2 init function
void MX_I2C2_Init(void)
{

   USER CODE BEGIN I2C2_Init 0

   USER CODE END I2C2_Init 0

   USER CODE BEGIN I2C2_Init 1

   USER CODE END I2C2_Init 1
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x007074AF;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  * Configure Analogue filter

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  * Configure Digital filter

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
   USER CODE BEGIN I2C2_Init 2

   USER CODE END I2C2_Init 2

}
 I2C3 init function
void MX_I2C3_Init(void)
{

   USER CODE BEGIN I2C3_Init 0

   USER CODE END I2C3_Init 0

   USER CODE BEGIN I2C3_Init 1

   USER CODE END I2C3_Init 1
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x007074AF;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  * Configure Analogue filter

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  * Configure Digital filter

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
   USER CODE BEGIN I2C3_Init 2

   USER CODE END I2C3_Init 2

}
 I2C4 init function
void MX_I2C4_Init(void)
{

   USER CODE BEGIN I2C4_Init 0

   USER CODE END I2C4_Init 0

   USER CODE BEGIN I2C4_Init 1

   USER CODE END I2C4_Init 1
  hi2c4.Instance = I2C4;
  hi2c4.Init.Timing = 0x007074AF;
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

  * Configure Analogue filter

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  * Configure Digital filter

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
  {
    Error_Handler();
  }
   USER CODE BEGIN I2C4_Init 2

   USER CODE END I2C4_Init 2

}*/

/*
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)          commented for porting
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
   USER CODE BEGIN I2C1_MspInit 0

   USER CODE END I2C1_MspInit 0

  * Initializes the peripherals clock

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    *I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB7     ------> I2C1_SDA

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

     I2C1 clock enable
    __HAL_RCC_I2C1_CLK_ENABLE();

     I2C1 DMA Init
     I2C1_RX Init
    hdma_i2c1_rx.Instance = GPDMA1_Channel0_BASE_S;
    hdma_i2c1_rx.Init.Request = LL_DMA_CHANNEL_1;
    hdma_i2c1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2c1_rx.Init.SrcInc = DMA_SINC_FIXED;
    hdma_i2c1_rx.Init.DestInc = DMA_DINC_FIXED;
    hdma_i2c1_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_i2c1_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_i2c1_rx.Init.Mode = DMA_NORMAL;
    hdma_i2c1_rx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    hdma_i2c1_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    if (HAL_DMA_Init(&hdma_i2c1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2cHandle,hdmarx,hdma_i2c1_rx);

     I2C1_TX Init
    hdma_i2c1_tx.Instance = GPDMA1_Channel6_BASE_S;
    hdma_i2c1_tx.Init.Request = LL_DMA_CHANNEL_1;
    hdma_i2c1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2c1_rx.Init.SrcInc = DMA_SINC_FIXED;
    hdma_i2c1_rx.Init.DestInc = DMA_DINC_FIXED;
    hdma_i2c1_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_i2c1_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_i2c1_rx.Init.Mode = DMA_NORMAL;
    hdma_i2c1_rx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    hdma_i2c1_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    if (HAL_DMA_Init(&hdma_i2c1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2cHandle,hdmatx,hdma_i2c1_tx);

     I2C1 interrupt Init
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
   USER CODE BEGIN I2C1_MspInit 1

   USER CODE END I2C1_MspInit 1
  }
*/

  //commented for porting
 /* else if(i2cHandle->Instance==I2C2)
  {
   USER CODE BEGIN I2C2_MspInit 0

   USER CODE END I2C2_MspInit 0

  * Initializes the peripherals clock

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
    PeriphClkInitStruct.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOF_CLK_ENABLE();
    *I2C2 GPIO Configuration
    PF0     ------> I2C2_SDA
    PF1     ------> I2C2_SCL

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

     I2C2 clock enable
    __HAL_RCC_I2C2_CLK_ENABLE();

     I2C2 DMA Init
     I2C2_RX Init
    hdma_i2c2_rx.Instance = DMA1_Stream2;
    hdma_i2c2_rx.Init.Channel = DMA_CHANNEL_7;
    hdma_i2c2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2c2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2c2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_i2c2_rx.Init.Mode = DMA_NORMAL;
    hdma_i2c2_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_i2c2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_i2c2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2cHandle,hdmarx,hdma_i2c2_rx);

     I2C2_TX Init
    hdma_i2c2_tx.Instance = DMA1_Stream7;
    hdma_i2c2_tx.Init.Channel = DMA_CHANNEL_7;
    hdma_i2c2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_i2c2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2c2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_i2c2_tx.Init.Mode = DMA_NORMAL;
    hdma_i2c2_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_i2c2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_i2c2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2cHandle,hdmatx,hdma_i2c2_tx);

     I2C2 interrupt Init
    HAL_NVIC_SetPriority(I2C2_EV_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
    HAL_NVIC_SetPriority(I2C2_ER_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
   USER CODE BEGIN I2C2_MspInit 1

   USER CODE END I2C2_MspInit 1
  }
  else if(i2cHandle->Instance==I2C3)
  {
   USER CODE BEGIN I2C3_MspInit 0

   USER CODE END I2C3_MspInit 0

  * Initializes the peripherals clock

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
    PeriphClkInitStruct.I2c3ClockSelection = RCC_I2C3CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOH_CLK_ENABLE();
    *I2C3 GPIO Configuration
    PH8     ------> I2C3_SDA
    PH7     ------> I2C3_SCL

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

     I2C3 clock enable
    __HAL_RCC_I2C3_CLK_ENABLE();

     I2C3 interrupt Init
    HAL_NVIC_SetPriority(I2C3_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_SetPriority(I2C3_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);
   USER CODE BEGIN I2C3_MspInit 1

   USER CODE END I2C3_MspInit 1
  }
  else if(i2cHandle->Instance==I2C4)
  {
   USER CODE BEGIN I2C4_MspInit 0

   USER CODE END I2C4_MspInit 0

  * Initializes the peripherals clock

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C4;
    PeriphClkInitStruct.I2c4ClockSelection = RCC_I2C4CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_GPIOH_CLK_ENABLE();
    *I2C4 GPIO Configuration
    PH12     ------> I2C4_SDA
    PH11     ------> I2C4_SCL

    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C4;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

     I2C4 clock enable
    __HAL_RCC_I2C4_CLK_ENABLE();

     I2C4 interrupt Init
    HAL_NVIC_SetPriority(I2C4_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C4_EV_IRQn);
    HAL_NVIC_SetPriority(I2C4_ER_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C4_ER_IRQn);*/
  /* USER CODE BEGIN I2C4_MspInit 1 */

  /* USER CODE END I2C4_MspInit 1 */
 // }
/*}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
   USER CODE BEGIN I2C1_MspDeInit 0

   USER CODE END I2C1_MspDeInit 0
     Peripheral clock disable
    __HAL_RCC_I2C1_CLK_DISABLE();

    *I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB7     ------> I2C1_SDA

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

     I2C1 DMA DeInit
    HAL_DMA_DeInit(i2cHandle->hdmarx);
    HAL_DMA_DeInit(i2cHandle->hdmatx);

     I2C1 interrupt Deinit
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);*/
  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
 // }
  /*else if(i2cHandle->Instance==I2C2)
  {
   USER CODE BEGIN I2C2_MspDeInit 0

   USER CODE END I2C2_MspDeInit 0
     Peripheral clock disable
    __HAL_RCC_I2C2_CLK_DISABLE();

    *I2C2 GPIO Configuration
    PF0     ------> I2C2_SDA
    PF1     ------> I2C2_SCL

    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_0);

    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_1);

     I2C2 DMA DeInit
    HAL_DMA_DeInit(i2cHandle->hdmarx);
    HAL_DMA_DeInit(i2cHandle->hdmatx);

     I2C2 interrupt Deinit
    HAL_NVIC_DisableIRQ(I2C2_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C2_ER_IRQn);
   USER CODE BEGIN I2C2_MspDeInit 1

   USER CODE END I2C2_MspDeInit 1
  }
  else if(i2cHandle->Instance==I2C3)
  {
   USER CODE BEGIN I2C3_MspDeInit 0

   USER CODE END I2C3_MspDeInit 0
     Peripheral clock disable
    __HAL_RCC_I2C3_CLK_DISABLE();

    *I2C3 GPIO Configuration
    PH8     ------> I2C3_SDA
    PH7     ------> I2C3_SCL

    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_7);

     I2C3 interrupt Deinit
    HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C3_ER_IRQn);
   USER CODE BEGIN I2C3_MspDeInit 1

   USER CODE END I2C3_MspDeInit 1
  }
  else if(i2cHandle->Instance==I2C4)
  {
   USER CODE BEGIN I2C4_MspDeInit 0

   USER CODE END I2C4_MspDeInit 0
     Peripheral clock disable
    __HAL_RCC_I2C4_CLK_DISABLE();

    *I2C4 GPIO Configuration
    PH12     ------> I2C4_SDA
    PH11     ------> I2C4_SCL

    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_11);

     I2C4 interrupt Deinit
    HAL_NVIC_DisableIRQ(I2C4_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C4_ER_IRQn);
   USER CODE BEGIN I2C4_MspDeInit 1

   USER CODE END I2C4_MspDeInit 1
  }*/
//}

/* USER CODE BEGIN 1 */

void I2C_init(void){   
   sensorBmp180.en = 0;
   hi2cSensor  = &hi2c1;
  // hi2cBattery = &hi2c2; commented for porting
//   pSensing = S4Sens_getSensing();
//   pStat = GetStatus();
   memset((uint8_t*)&i2cSens_buf, 0, sizeof(i2cReadBufTypeDef));
   // init eeprom
   CAT24C16_init(hi2cSensor);  
   //STC3100_init(hi2cBattery);
#if USE_I2C_VBATT_REPORT   
   STC3100_wake(1);   
#endif   
}

uint8_t I2C_test(void){   
   uint8_t ret_val = 0;
   Board_SW_I2C(1);
   
   Board_SW_EXP(1);//eeprom
   HAL_Delay(50);
   
   I2C_scan(hi2cSensor);   
   //I2C_scan(hi2cBattery); commented for porting.
   
   MPU9250_init(hi2cSensor);
   if(MPU9250_test())
      ret_val |= 0x01;
   
   LSM303DLHC_init(hi2cSensor);
   if(LSM303DLHC_accelTest())
      ret_val |= 0x02;
   if(LSM303DLHC_magTest())
      ret_val |= 0x04;
   
   Board_SW_EXP(1);
   CAT24C16_init(I2C_getHandlerSensor());
   //HAL_Delay(1000);   
   if (CAT24C16_test()) { //eeprom
      ret_val |= 0x08;
   }
   Board_SW_I2C(1);
   
   if(bmp280_test(hi2cSensor)){
      ret_val |= 0x10;   
   }
   
   return ret_val;
}

void I2C_scan(I2C_HandleTypeDef *hi2c){
   uint8_t buf = 0;
   uint16_t i2c_addr;
   HAL_StatusTypeDef result;
   
   i2c_addr_list_len = 0;
   for(i2c_addr = 0; i2c_addr < 0x80; i2c_addr++)
   {
      result = HAL_I2C_Master_Receive(hi2c, i2c_addr<<1, &buf, 1, 1);
      if(result == HAL_OK){
         i2c_addr_list[i2c_addr_list_len++] = i2c_addr;
      }
   }
}

I2C_HandleTypeDef * I2C_getHandlerSensor(void){
   return hi2cSensor;
}
/*I2C_HandleTypeDef * I2C_getHandlerBatt(void){  commented for porting
   return hi2cBattery;*/
//}
  
void I2C_readBatt(void){
   static uint16_t cnt = 0;
#if USE_I2C_VBATT_REPORT   
   if(stat.isSensing && (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_STC3100)){
   
   } else if(i2c_batt_report_interval == 0){
      
   } else{
      if(++cnt >= i2c_batt_report_interval){    
         cnt = 0;
         STC3100_readData((uint8_t*)stat.battDigital);   
         uint8_t bt_tx_data[131], packet_length = 0;
         *(bt_tx_data + packet_length++) = INSTREAM_CMD_RESPONSE;
         *(bt_tx_data + packet_length++) = RSP_I2C_BATT_STATUS_COMMAND;
         memcpy((bt_tx_data + packet_length), (uint8_t*)stat.battDigital, STC3100_DATA_LEN);
         packet_length += STC3100_DATA_LEN;     
         BT_write(bt_tx_data, packet_length);
      }      
   }
#endif
}
void I2C_readBattSetFreq(uint16_t val){
   i2c_batt_report_interval = val;
}


void I2C_configureChannels(void){
   I2cSens_configureChannels();
   I2cBatt_configureChannels();
}
void I2cSens_configureChannels(void){	
   uint8_t *channel_contents_ptr = sensing.cc+sensing.ccLen;
   i2cSens.sensorLen = i2cSens.sensorCnt = 0;
   uint8_t nbr_i2c1_chans = 0;
   
   //Digi Gyro (MPU9250)
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_MPU9250_GYRO) {
      *channel_contents_ptr++ = X_MPU9250_GYRO;
      *channel_contents_ptr++ = Y_MPU9250_GYRO;
      *channel_contents_ptr++ = Z_MPU9250_GYRO;
      nbr_i2c1_chans += 3;
      sensing.ptr.mpu9250Gyro = sensing.dataLen;
      sensing.dataLen += 6;
      i2cSens.sensorList[i2cSens.sensorLen++] = I2C_MPU9250_GYRO;
   }
   //Digi Accel (LSM303DLHC)
   if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_LSM303DLHC_ACCEL) {
      *channel_contents_ptr++ = X_LSM303DLHC_ACCEL;
      *channel_contents_ptr++ = Y_LSM303DLHC_ACCEL;
      *channel_contents_ptr++ = Z_LSM303DLHC_ACCEL;
      nbr_i2c1_chans += 3;
      sensing.ptr.lsm303dlhcAccel = sensing.dataLen;
      sensing.dataLen += 6;
      i2cSens.sensorList[i2cSens.sensorLen++] = I2C_LSM303DLHC_ACCEL;
   }
   //Mag (LSM303DLHC)
   if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_LSM303DLHC_MAG) {
      *channel_contents_ptr++ = X_LSM303DLHC_MAG;
      *channel_contents_ptr++ = Z_LSM303DLHC_MAG;
      *channel_contents_ptr++ = Y_LSM303DLHC_MAG;
      nbr_i2c1_chans += 3;
      sensing.ptr.lsm303dlhcMag = sensing.dataLen;
      sensing.dataLen += 6;
      i2cSens.sensorList[i2cSens.sensorLen++] = I2C_LSM303DLHC_MAG;
   }
   //Digi Accel (MPU9250)
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_MPU9250_ACCEL) {
      *channel_contents_ptr++ = X_MPU9250_ACCEL;
      *channel_contents_ptr++ = Y_MPU9250_ACCEL;
      *channel_contents_ptr++ = Z_MPU9250_ACCEL;
      nbr_i2c1_chans += 3;
      sensing.ptr.mpu9250Accel = sensing.dataLen;
      sensing.dataLen += 6;
      i2cSens.sensorList[i2cSens.sensorLen++] = I2C_MPU9250_ACCEL;
   }
   //Digi Mag (MPU9250)
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_MPU9250_MAG) {
      *channel_contents_ptr++ = X_MPU9250_MAG;
      *channel_contents_ptr++ = Y_MPU9250_MAG;
      *channel_contents_ptr++ = Z_MPU9250_MAG;
      nbr_i2c1_chans += 3;
      sensing.ptr.mpu9250Mag = sensing.dataLen;
      sensing.dataLen += 6;
      i2cSens.sensorList[i2cSens.sensorLen++] = I2C_MPU9250_MAG;
   }


   // Temp & Pressure sensor (BMP280)
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_BMP180_PRESSURE) {
      *channel_contents_ptr++ = BMP180_TEMP;
      *channel_contents_ptr++ = BMP180_PRESSURE;
      nbr_i2c1_chans += 2; //TEMP & PRES, ON/OFF together
      sensing.ptr.temp = sensing.dataLen;
      sensing.dataLen += 2;
      sensing.ptr.pres = sensing.dataLen;
      sensing.dataLen += 3;
#if USE_BMPX80 == 1
      i2cSens.sensorList[i2cSens.sensorLen++] = I2C_BMP180;
#else
      i2cSens.sensorList[i2cSens.sensorLen++] = I2C_BMP280;
#endif
   }
   sensing.ccLen += nbr_i2c1_chans;
   sensing.nbrDigiChans += nbr_i2c1_chans;
}


/*void I2cBatt_configureChannels(void){    //commented for porting
   uint8_t *channel_contents_ptr = sensing.cc+sensing.ccLen;
   uint8_t nbr_i2c_batt_chans = 0;
   i2cBatt.sensorLen = i2cBatt.sensorCnt = 0; 
   //Digi Battery IC (STC3100)
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_STC3100) {
      // All 5 channels - charge, counter, current, voltage, temperature
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
}*/

void I2C_startSensing(void){
   memset((uint8_t*)&i2cSens_buf, 0, sizeof(i2cReadBufTypeDef));
   
/*   if ((0 != i2cSens.sensorLen) && (HAL_GPIO_ReadPin(SW_I2C_GPIO_Port, SW_I2C_Pin) == GPIO_PIN_RESET)) {   Commented for porting
      //HAL_GPIO_WritePin(SW_I2C_GPIO_Port, SW_I2C_Pin, GPIO_PIN_SET);//I2C
      Board_SW_I2C(1);
      HAL_Delay(1000);
   }*/

   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_STC3100) {
      STC3100_wake(1);
   }

   if ((S4Ram_storedConfigGetByte(NV_SENSORS0)&SENSOR_MPU9250_GYRO) ||
         (S4Ram_storedConfigGetByte(NV_SENSORS2)&SENSOR_MPU9250_ACCEL) ||
         (S4Ram_storedConfigGetByte(NV_SENSORS2)&SENSOR_MPU9250_MAG)) {
      MPU9250_init(hi2cSensor);
      if ((S4Ram_storedConfigGetByte(NV_SENSORS0)&SENSOR_MPU9250_GYRO) ||
            (S4Ram_storedConfigGetByte(NV_SENSORS2)&SENSOR_MPU9250_ACCEL)) {
         MPU9250_wake(1);
         MPU9250_setSamplingRate(S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE1));
      }
      if (S4Ram_storedConfigGetByte(NV_SENSORS0)&SENSOR_MPU9250_GYRO) {
         MPU9250_setGyroSensitivity(S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE2) & 0x03); //This needs to go after the wake?
      }
      if (S4Ram_storedConfigGetByte(NV_SENSORS2)&SENSOR_MPU9250_ACCEL) {
         MPU9250_setAccelRange((S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE3) & 0xC0) >> 6);
      }
      if (S4Ram_storedConfigGetByte(NV_SENSORS2)&SENSOR_MPU9250_MAG) {
         MPU9250MagSetup();
      }
   }

   if ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_LSM303DLHC_MAG) || 
       (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_LSM303DLHC_ACCEL)) {
      LSM303DLHC_init(hi2cSensor);
      if (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_LSM303DLHC_ACCEL) {
         LSM303DLHC_accelInit(((S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE0) & 0xF0) >> 4), //sampling rate
                              ((S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE0) & 0x0C) >> 2), //range
                              ((S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE0) & 0x02) >> 1), //low power mode
                              ( S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE0) & 0x01));    //high resolution mode
      }
      if (S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_LSM303DLHC_MAG)
         LSM303DLHC_magInit(((S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE2) & 0x1C) >> 2), //sampling rate
                            ((S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE2) & 0xE0) >> 5)); //gain
   }

   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_BMP180_PRESSURE) {
#if USE_BMPX80 == 1
      BMP180Setup();
#else //USE_BMPX80 == 2
      BMP280Setup();
#endif
   }
}
void I2C_pollSensors(void){  
}

void I2C_stopSensing(void){
   I2cSens_stopSensing();
   I2cBatt_stopSensing();   
   
   //HAL_I2C_MspDeInit(hi2cSensor);//this may save .2-.3 mA?
   //HAL_I2C_MspDeInit(hi2cBattery);//this may save .2-.3 mA?
}
void I2cSens_stopSensing(void){
   if ((S4Ram_storedConfigGetByte(NV_SENSORS0)&SENSOR_MPU9250_GYRO) ||
         (S4Ram_storedConfigGetByte(NV_SENSORS2)&SENSOR_MPU9250_ACCEL)) {
      MPU9250_wake(0);
   }
   if ((S4Ram_storedConfigGetByte(NV_SENSORS0) & SENSOR_LSM303DLHC_MAG) || 
       (S4Ram_storedConfigGetByte(NV_SENSORS1) & SENSOR_LSM303DLHC_ACCEL)) {
      LSM303DLHC_sleep();
   }
   HAL_Delay(10);
   Board_SW_I2C(0);
}

/*void I2cBatt_stopSensing(void){ //commented for porting
   if (S4Ram_storedConfigGetByte(NV_SENSORS2) & SENSOR_STC3100) {
      STC3100_wake(0);
   }
}*/
   

// ================================================
// sensor codes
// ================================================


void (*I2cSens_gatherDataDone_cb)(void);
void (*I2cBatt_gatherDataDone_cb)(void);

void I2cSens_gatherDataCb(void (*done_cb)(void)){
   I2cSens_gatherDataDone_cb = done_cb;   
}
   
void I2C_gatherDataStart(void){
   I2cSens_gatherDataStart();
}

void I2cSens_gatherDataStart(void){   
   I2cSensing(I2C_FIRST_SENSOR);
}

void I2cBatt_gatherDataCb(void (*done_cb)(void)){
   I2cBatt_gatherDataDone_cb = done_cb;   
}
   
void I2cBatt_gatherDataStart(void){  
   //I2cBatt_gatherDataDone_cb();
   I2cBattMonitor(I2C_FIRST_SENSOR);
}
   


void I2cSensing(I2C_SENSING_TYPE start) {
   i2cSens.sensorCnt = (start ==  I2C_FIRST_SENSOR)? 0 : i2cSens.sensorCnt + 1;
   if (i2cSens.sensorCnt == i2cSens.sensorLen) {
      i2cSens.status = I2C_STAT_IDLE;
      i2cSens.sensorCnt = 0;
      I2cSens_gatherDataDone_cb();
   } else if (i2cSens.sensorCnt < i2cSens.sensorLen) {
      I2cSens_sensorNext();
      //Task_set(TASK_NEXTSENSOR);
   } else{
      while (1) {
       //  Board_ledToggle(LED_ALL);     commenting for porting
         HAL_Delay(100);
      }
   }
}

/*void I2cBattMonitor(I2C_SENSING_TYPE start) { //commented for porting
   i2cBatt.sensorCnt = (start ==  I2C_FIRST_SENSOR) ? 0 : i2cBatt.sensorCnt + 1;
   if (i2cBatt.sensorCnt == i2cBatt.sensorLen) {
      i2cBatt.status = I2C_STAT_IDLE;
      I2cBatt_gatherDataDone_cb();
   } else if (i2cBatt.sensorCnt < i2cBatt.sensorLen) {
      I2cBatt_sensorNext();
   } else{
      while (1) {
         Board_ledToggle(LED_ALL);
         HAL_Delay(100);
      }
   }
}*/

void I2cSens_sensorNext(void){
   switch (i2cSens.sensorList[i2cSens.sensorCnt]) {
   case I2C_LSM303DLHC_ACCEL:    Lsm303dlhcAccelSample();   break;
   case I2C_LSM303DLHC_MAG:      Lsm303dlhcMagSample();     break;
   case I2C_MPU9250_GYRO:        MPU9250GyroSample();       break;
   case I2C_MPU9250_ACCEL:       MPU9250AccelSample();      break;
   case I2C_MPU9250_MAG:         MPU9250MagSample();        break;
   case I2C_BMP180:              BMP180Sample();            break;
   case I2C_BMP280:              BMP280Sample();            break;
   default: break;
   }
}

/*void I2cBatt_sensorNext(void) { //commented for porting
   switch (i2cBatt.sensorList[i2cBatt.sensorCnt]) {
   case I2C_STC3100:             STC3100Sample();           break;
   default: break;
   }
}*/


void BMP180Setup(void) {
   BMP180_init(hi2cSensor);

   sensorBmp180.en = 1;
   sensorBmp180.oss = (S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE3) & BMP180_PRESSURE_RESOLUTION) >> 4;
   switch (sensorBmp180.oss) {
   case 0: sensorBmp180.preSampleClkInterval = 45; break;
   case 1: sensorBmp180.preSampleClkInterval = 75; break;
   case 2: sensorBmp180.preSampleClkInterval = 135; break;
   case 3: sensorBmp180.preSampleClkInterval = 255; break;
   default : sensorBmp180.preSampleClkInterval = 45; break;
   }

   BMP180_tempStartMeasurement();
   sensorBmp180.presCnt = sensorBmp180.presMax = (uint16_t)ceil((float)sensorBmp180.preSampleClkInterval / (float)sensing.clkInterval4096);

   sensorBmp180.tempMax = sensing.freq < 3 ? 1 : (uint16_t)(sensing.freq / (float)sensorBmp180.presMax - 1);
   sensorBmp180.tempCnt = 0;
}

void BMP180Sample(void) {
   if (!--sensorBmp180.presCnt) { //--sensorBmp180.presCnt or sensorBmp180.presCnt--?
      if (!sensorBmp180.tempCnt--) {
         sensorBmp180.tempCnt = sensorBmp180.tempMax;
         i2cSens.status = I2C_STAT_BMP180_TEMP_GET_T;
         BMP180_tempReadTx();
      } else {
         i2cSens.status = I2C_STAT_BMP180_PRES_GET_T;
         BMP180_presReadTx();
      }
      sensorBmp180.presCnt = sensorBmp180.presMax;
   }
}

void BMP180TxDoneHandler(void) {
   if (i2cSens.status == I2C_STAT_BMP180_TEMP_START) {
      I2cSensing(I2C_NEXT_SENSOR);// goto next sensor
   } else if (i2cSens.status == I2C_STAT_BMP180_TEMP_GET_T) {
      i2cSens.status = I2C_STAT_BMP180_TEMP_GET_R;
      BMP180_tempReadRx(sensing.dataBuf + sensing.ptr.temp);
   } else if (i2cSens.status == I2C_STAT_BMP180_PRES_START) {
      I2cSensing(I2C_NEXT_SENSOR);// goto next sensor
   } else if (i2cSens.status == I2C_STAT_BMP180_PRES_GET_T) {
      i2cSens.status = I2C_STAT_BMP180_PRES_GET_R;
      BMP180_presReadRx(sensing.dataBuf + sensing.ptr.pres);
   }
   else {
      i2cSens.status = I2C_STAT_IDLE;
   }
}
void BMP180RxDoneHandler(void) {
   if (i2cSens.status == I2C_STAT_BMP180_TEMP_GET_R) {
      if (stat.isSensing) {
         i2cSens.status = I2C_STAT_BMP180_PRES_START;
         BMP180_presStartMeasurement(sensorBmp180.oss);
      } else
         i2cSens.status = I2C_STAT_IDLE;
   } else if (i2cSens.status == I2C_STAT_BMP180_PRES_GET_R) {
      if (stat.isSensing) {
         if (!sensorBmp180.tempCnt) {
            i2cSens.status = I2C_STAT_BMP180_TEMP_START;
            BMP180_tempStartMeasurement();
         } else {
            i2cSens.status = I2C_STAT_BMP180_PRES_START;
            BMP180_presStartMeasurement(sensorBmp180.oss);
         }
      } else
         i2cSens.status = I2C_STAT_IDLE;
   }
   else {
      i2cSens.status = I2C_STAT_IDLE;
   }
}


/*void BMP280Setup(void) { // commented for porting
   uint8_t val;
   sensorBmp280.en = 1;
   // use values 0,1,2,3 to represent mode 0,2,3,4. mode 1 abandoned
   val = (S4Ram_storedConfigGetByte(NV_CONFIG_SETUP_BYTE3) & BMP180_PRESSURE_RESOLUTION) >> 4;
   val = 0;
   bmp280.working_mode = (val == 0) ? 0 : val + 1;  //commenting for porting
   bmp280.dev_addr = BMP280_I2C_ADDRESS1 << 1;
  // bmp280.hi2c = hi2cSensor;  commented for porting
   //bmp280_init(&bmp280); commented for porting

   switch (bmp280.working_mode) {
   case 0: sensorBmp280.preSampleClkInterval = 64;       break; //64+10? for BMP280_STANDBY_TIME_1_MS
   case 1: sensorBmp280.preSampleClkInterval = 87;       break;
   case 2: sensorBmp280.preSampleClkInterval = 133;      break;
   case 3: sensorBmp280.preSampleClkInterval = 225;      break;
   case 4: sensorBmp280.preSampleClkInterval = 432;      break;
   default : sensorBmp280.preSampleClkInterval = 432;    break;
   }

   sensorBmp280.presCnt = sensorBmp280.presMax = (uint16_t)ceil((float)sensorBmp280.preSampleClkInterval / (float)sensing.clkInterval4096);

   sensorBmp280.tempMax = sensing.freq < 3 ? 1 : (uint16_t)(sensing.freq / (float)sensorBmp280.presMax - 1);
   sensorBmp280.tempCnt = 0;
}*/

void BMP280Sample(void) {
   if (!--sensorBmp280.presCnt) {
      if (!sensorBmp280.tempCnt--) {
         sensorBmp280.tempCnt = sensorBmp280.tempMax;
         i2cSens.status = I2C_STAT_BMP280_TEMP_GET_T;
         BMP280_tempReadTx();
      } else {
         i2cSens.status = I2C_STAT_BMP280_PRES_GET_T;
         BMP280_presReadTx();
      }
      sensorBmp280.presCnt = sensorBmp280.presMax;
   }
}

void BMP280TxDoneHandler(void) {
   if (i2cSens.status == I2C_STAT_BMP280_START) {
      I2cSensing(I2C_NEXT_SENSOR);// goto next sensor
   } else if (i2cSens.status == I2C_STAT_BMP280_TEMP_GET_T) {
      i2cSens.status = I2C_STAT_BMP280_TEMP_GET_R;
      BMP280_tempReadRx();
   } else if (i2cSens.status == I2C_STAT_BMP280_PRES_GET_T) {
      i2cSens.status = I2C_STAT_BMP280_PRES_GET_R;
      BMP280_presReadRx();
   } else {
      i2cSens.status = I2C_STAT_IDLE;
   }
}

void BMP280RxDoneHandler(void) {
   if (i2cSens.status == I2C_STAT_BMP280_TEMP_GET_R) {
      BMP280_tempReadDone(sensing.dataBuf + sensing.ptr.temp);
      I2cSensing(I2C_NEXT_SENSOR);
   } else if (i2cSens.status == I2C_STAT_BMP280_PRES_GET_R) {
      BMP280_presReadDone(sensing.dataBuf + sensing.ptr.pres);
      i2cSens.status = I2C_STAT_BMP280_START;
      BMP280_startTx();
      I2cSensing(I2C_NEXT_SENSOR);
   } else {
      i2cSens.status = I2C_STAT_IDLE;
   }
}

/*void STC3100Sample(void) {    //commenting for porting
   i2cBatt.status = I2C_STAT_STC3100_DATA_GET;
   STC3100_readData_it(stc3100_buf);
}

void STC3100BatteryRxDoneHandler(void) {   
   //static uint8_t i=0;
   memcpy(sensing.dataBuf + sensing.ptr.stc3100Batt, stc3100_buf, STC3100_DATA_LEN);
   i2cBatt.status = I2C_STAT_IDLE;
   I2cBattMonitor(I2C_NEXT_SENSOR); 
}*/


void MPU9250GyroSample(void) {
   i2cSens.status = I2C_STAT_MPU9250_GYRO_GET_T;
   MPU9250_gyroReadStart();
}

void MPU9250GyroTxDoneHandler(void) {
   i2cSens.status = I2C_STAT_MPU9250_GYRO_GET_R;
   //MPU9250_gyroReadDone(sensing.dataBuf + sensing.ptr.mpu9250Gyro);
   MPU9250_gyroReadDone(i2cSens_buf.mpu9250GyroBuf);
}

void MPU9250GyroRxDoneHandler(void) {
   memcpy(sensing.dataBuf + sensing.ptr.mpu9250Gyro, i2cSens_buf.mpu9250GyroBuf, 6);
   i2cSens.status = I2C_STAT_IDLE;
   I2cSensing(I2C_NEXT_SENSOR); // goto next sensor
}

void MPU9250AccelSample(void) {
   i2cSens.status = I2C_STAT_MPU9250_ACCEL_GET_T;
   MPU9250_accelReadStart();
}

void MPU9250AccelTxDoneHandler(void) {
   i2cSens.status = I2C_STAT_MPU9250_ACCEL_GET_R;
   //MPU9250_accelReadDone(sensing.dataBuf + sensing.ptr.mpu9250Accel);
   MPU9250_accelReadDone(i2cSens_buf.mpu9250AccelBuf);
}

void MPU9250AccelRxDoneHandler(void) {
   memcpy(sensing.dataBuf + sensing.ptr.mpu9250Accel, i2cSens_buf.mpu9250AccelBuf, 6);
   i2cSens.status = I2C_STAT_IDLE;
   I2cSensing(I2C_NEXT_SENSOR); // goto next sensor
}

void MPU9250MagSetup(void) {
   //max of approx. 3ms to sample everything + 9ms between starting mag to data ready
   mpu9250Mag.en = 1;
   mpu9250Mag.preSampleClkInterval = 90;

   //MPU9250_magStartSingleMeasurement();
   MPU9250_magStart();
   mpu9250Mag.cnt = mpu9250Mag.max = (uint16_t)ceil((float)mpu9250Mag.preSampleClkInterval / (float)sensing.clkInterval4096);
}

void MPU9250MagSample(void) {
   i2cSens.status = I2C_STAT_MPU9250_MAG_GET_T;
   MPU9250_magReadStart();
//   if (0 == --mpu9250Mag.cnt) {
//      i2cSens.status = I2C_STAT_MPU9250_MAG_GET_T;
//      MPU9250_magReadStart();
//      mpu9250Mag.cnt = mpu9250Mag.max;
//   }
}

void MPU9250MagTxDoneHandler(void) {
   i2cSens.status = I2C_STAT_MPU9250_MAG_GET_R;
   MPU9250_magReadDone(i2cSens_buf.mpu9250MagBuf);
//   if (I2C_STAT_MPU9250_MAG_START == i2cSens.status) {
//      I2cSensing(I2C_NEXT_SENSOR); // goto next sensor
//   } else if (I2C_STAT_MPU9250_MAG_GET_T == i2cSens.status) {
//      i2cSens.status = I2C_STAT_MPU9250_MAG_GET_R;
//      //MPU9250_magReadDone(sensing.dataBuf + sensing.ptr.mpu9250Mag);
//      MPU9250_magReadDone(i2cSens_buf.mpu9250MagBuf);      
//   } else {
//      i2cSens.status = I2C_STAT_IDLE;
//   }
}

void MPU9250MagRxDoneHandler(void) {   
   memcpy(sensing.dataBuf + sensing.ptr.mpu9250Mag, i2cSens_buf.mpu9250MagBuf, 6);  
   i2cSens.status = I2C_STAT_IDLE; 
   I2cSensing(I2C_NEXT_SENSOR); // goto next sensor
   
//   if (I2C_STAT_MPU9250_MAG_GET_R == i2cSens.status) {
//      memcpy(sensing.dataBuf + sensing.ptr.mpu9250Mag, i2cSens_buf.mpu9250MagBuf, 6);
//      i2cSens.status = I2C_STAT_IDLE;
//      //MPU9250_magStartSingleMeasurement();
//   } else {
//      i2cSens.status = I2C_STAT_IDLE;
//   }
}

void Lsm303dlhcAccelSample(void) {
   i2cSens.status = I2C_STAT_LSM303DLHC_ACCEL_GET_T;
   LSM303DLHC_accelReadStart();
}

void Lsm303dlhcAccelTxDoneHandler(void) {
   i2cSens.status = I2C_STAT_LSM303DLHC_ACCEL_GET_R;
   //LSM303DLHC_accelReadDone(sensing.dataBuf + sensing.ptr.lsm303dlhcAccel);
   LSM303DLHC_accelReadDone(i2cSens_buf.lsm303AccelBuf);
}

void Lsm303dlhcAccelRxDoneHandler(void) {
   memcpy(sensing.dataBuf + sensing.ptr.lsm303dlhcAccel, i2cSens_buf.lsm303AccelBuf, 6);
   i2cSens.status = I2C_STAT_IDLE;
   I2cSensing(I2C_NEXT_SENSOR); // goto next sensor
}

void Lsm303dlhcMagSample(void) {
   i2cSens.status = I2C_STAT_LSM303DLHC_MAG_GET_T;
   LSM303DLHC_magReadStart();
}

void Lsm303dlhcMagTxDoneHandler(void) {
   i2cSens.status = I2C_STAT_LSM303DLHC_MAG_GET_R;
   //LSM303DLHC_magReadDone(sensing.dataBuf + sensing.ptr.lsm303dlhcMag);
   LSM303DLHC_magReadDone(i2cSens_buf.lsm303MagBuf);
}
void Lsm303dlhcMagRxDoneHandler(void) {
   memcpy(sensing.dataBuf + sensing.ptr.lsm303dlhcMag, i2cSens_buf.lsm303MagBuf, 6);
   i2cSens.status = I2C_STAT_IDLE;
   I2cSensing(I2C_NEXT_SENSOR); // goto next sensor
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
   if (hi2c->Instance == hi2cSensor->Instance) {
      switch (i2cSens.sensorList[i2cSens.sensorCnt]) {
      case I2C_BMP180:              BMP180TxDoneHandler();              break;
      case I2C_BMP280:              BMP280TxDoneHandler();              break;
      case I2C_MPU9250_GYRO:        MPU9250GyroTxDoneHandler();         break;
      case I2C_MPU9250_ACCEL:       MPU9250AccelTxDoneHandler();        break;
      case I2C_MPU9250_MAG:         MPU9250MagTxDoneHandler();          break;
      case I2C_LSM303DLHC_ACCEL:    Lsm303dlhcAccelTxDoneHandler();     break;
      case I2C_LSM303DLHC_MAG:      Lsm303dlhcMagTxDoneHandler();       break;
      default: break;
      }
   }
}
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
   if (hi2c->Instance == hi2cSensor->Instance) {
      switch (i2cSens.sensorList[i2cSens.sensorCnt]) {
      case I2C_BMP180:              BMP180RxDoneHandler();              break;
      case I2C_BMP280:              BMP280RxDoneHandler();              break;
      case I2C_MPU9250_GYRO:        MPU9250GyroRxDoneHandler();         break;
      case I2C_MPU9250_ACCEL:       MPU9250AccelRxDoneHandler();        break;
      case I2C_MPU9250_MAG:         MPU9250MagRxDoneHandler();          break;
      case I2C_LSM303DLHC_ACCEL:    Lsm303dlhcAccelRxDoneHandler();     break;
      case I2C_LSM303DLHC_MAG:      Lsm303dlhcMagRxDoneHandler();       break;
      default: break;
      }
   }
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
//   if(hi2c->Instance == hi2cBattery->Instance){
//      switch(i2cBatt.sensorList[i2cBatt.sensorCnt]){
//      case I2C_STC3100:              STC3100BatteryTxDoneHandler();     break;
//      default: break;
//      }
//   }
}

/*void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) commented for porting
{
   if(hi2c->Instance == hi2cBattery->Instance){
      switch(i2cBatt.sensorList[i2cBatt.sensorCnt]){
      case I2C_STC3100:              STC3100BatteryRxDoneHandler();     break;
      default: break;
      }
   }
}*/

/* USER CODE END 1 */
