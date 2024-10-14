/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    gpio.c
 * @brief   This file provides code for the configuration
 *          of all used GPIO pins.
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
#include "gpio.h"

/* USER CODE BEGIN 0 */

#include "usb_device.h"
#include "usbd_core.h"

#include "shimmer_externs.h"

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

uint64_t GPIO_tsPress = 0, GPIO_tsLastRelease = 0, GPIO_tsRelease = 0;

/* USER CODE END 1 */

/** Configure pins
     PC14-OSC32_IN (PC14)   ------> RCC_OSC32_IN
     PC15-OSC32_OUT (PC15)   ------> RCC_OSC32_OUT
     PH0-OSC_IN (PH0)   ------> RCC_OSC_IN
     PH1-OSC_OUT (PH1)   ------> RCC_OSC_OUT
     PA13 (JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
     PA14 (JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_LIS3MDL_GPIO_Port, CS_LIS3MDL_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, CS_LSM6DSV_Pin | BT_CP_ROLE_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, SW_SENSE_IO_Pin | SW_GSR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD,
      BT_LP_MODE_Pin | BT_RST_Pin | CS_BMP390_Pin | CS_LIS2DW12_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SW_BT_GPIO_Port, SW_BT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SW_MIC_Pin | SW_SD_MCU_DOCK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_HIGH_G_GPIO_Port, CS_HIGH_G_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, SW_FLASH_Pin | SW_SENSE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PEPin PEPin PEPin PEPin
                           PEPin PEPin */
  GPIO_InitStruct.Pin = LIS3MDL_DRDY_Pin | LSM6DSV_INT1_Pin | BT_HOST_WAKE_Pin
      | GPIO_EXTERNAL_Pin | LIS2MDL_DRDY_Pin | BMP390_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PEPin PEPin PEPin */
  GPIO_InitStruct.Pin = CS_LIS3MDL_Pin | SW_SENSE_IO_Pin | SW_GSR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = GPIO_INTERNAL0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIO_INTERNAL0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PCPin PCPin PCPin */
  GPIO_InitStruct.Pin = CS_LSM6DSV_Pin | BT_CP_ROLE_Pin | SW_BT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = DOCK_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(DOCK_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = GPIO_ADC_INT_EXP1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIO_ADC_INT_EXP1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PBPin PBPin */
  GPIO_InitStruct.Pin = CHG_STAT1_Pin | CHG_STAT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PDPin PDPin PDPin PDPin
                           PDPin PDPin */
  GPIO_InitStruct.Pin = BT_LP_MODE_Pin | BT_RST_Pin | SW_FLASH_Pin
      | SW_SENSE_Pin | CS_BMP390_Pin | CS_LIS2DW12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PDPin PDPin */
  GPIO_InitStruct.Pin = BT_CONNECTION_Pin | BT_CYSPP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PAPin PAPin PAPin */
  GPIO_InitStruct.Pin = SW_MIC_Pin | CS_HIGH_G_Pin | SW_SD_MCU_DOCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = SD_DETECT_N_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SD_DETECT_N_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PDPin PDPin */
  GPIO_InitStruct.Pin = GPIO_INTERNAL1_Pin | GPIO_INTERNAL2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = BOOT0_USER_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT0_USER_BTN_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI9_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_IRQn);

  HAL_NVIC_SetPriority(EXTI14_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI14_IRQn);
}

/* USER CODE BEGIN 2 */

void GPIO_VBUS_init(uint8_t state)
{
  //pSensing = S4Sens_getSensing();
  //pStat = GetStatus();
  //GPIO_tsPress = GPIO_tsRelease = GPIO_tsLastRelease = 0;
  if (state)
  {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = USB_VBUS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(USB_VBUS_GPIO_Port, &GPIO_InitStruct);
  }
  else
  {
    HAL_GPIO_DeInit(USB_VBUS_GPIO_Port, USB_VBUS_Pin);
  }
}

void GPIO_userButtonCheck()
{
  if (HAL_GPIO_ReadPin(BOOT0_USER_BTN_GPIO_Port, BOOT0_USER_BTN_Pin) == GPIO_PIN_SET)
  { //pressed
    shimmerStatus.isButtonPressed = 1;
    Board_ledOn(LED_YELLOW);
    GPIO_tsPress = RTC_get64();
  }
  else
  {
    shimmerStatus.isButtonPressed = 0;
    Board_ledOff(LED_YELLOW);
    GPIO_tsRelease = RTC_get64();
    if (GPIO_tsRelease - GPIO_tsLastRelease > 3277)
    {
      if (shimmerStatus.isSensing == 0)
      {
        shimmerStatus.sdlogCmd = 1;
        S4_Task_set(TASK_STARTSENSING);
      }
      else
      {
        shimmerStatus.sdlogCmd = 2;
        S4_Task_set(TASK_STOPSENSING);
      }
    }
    GPIO_tsLastRelease = GPIO_tsRelease;
  }
}

uint16_t ext_cnt1 = 0;
uint16_t ext_cnt2 = 0;
uint16_t ext_cnt3 = 0;
uint16_t ext_cnt4 = 0;
uint16_t ext_cnt5 = 0;
uint16_t ext_cnt6 = 0;

//TODO copy Shimmer4 pins from HAL_GPIO_EXTI_Callback to HAL_GPIO_EXTI_Rising_Callback and HAL_GPIO_EXTI_Falling_Callback
#if defined(SHIMMER3R)
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
  case USB_VBUS_Pin:
    if (!(S4_NORM_Task_getList() & TASK_USB_SETUP))
    {
      S4_Task_set(TASK_USB_SETUP);
    }
    break;
  default:
    gpioExtiCommon(GPIO_Pin, 1);
    break;
  }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
  case GPIO_INTERNAL1_Pin:
    //TODO check if product is ExG unit
    //EXG1 DRDY active low
    if (shimmerStatus.isSensing)
    {
      //EXG_dataReadyChip1();
      ext_cnt1++;
      if (!(ext_cnt1 % 100))
      {
        __NOP();
        __NOP();
        __NOP();
      }
      EXG_gatherDataStart();
    }
    break;
  case GPIO_INTERNAL0_Pin:
    //TODO check if product is ExG unit
    //EXG2 DRDY active low
    if (shimmerStatus.isSensing)
    {
      //EXG_gatherDataStart();
      __NOP();
      __NOP();
      __NOP();
      //EXG_dataReadyChip2();
    }
    break;
  default:
    gpioExtiCommon(GPIO_Pin, 0);
    break;
  }
}

void gpioExtiCommon(uint16_t GPIO_Pin, uint8_t isRising)
{
  switch (GPIO_Pin)
  {
  case BT_CONNECTION_Pin:
    //setBtConnectionState(isRising);
    break;
  case BT_CYSPP_Pin:
    //setBtCysppState(isRising);
    break;
  case DOCK_DETECT_Pin:
    DockUart_interruptCheck();
    S4_Task_set(TASK_DOCKSETUP);
    break;
  case BOOT0_USER_BTN_Pin:
    GPIO_userButtonCheck();
    break;
  case SD_DETECT_N_Pin:
    SD_insertedCheck();
    break;
  default:
    break;
  }
}

#elif defined(SHIMMER4_SDK)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
  case GPIO_INTERNAL1_Pin:
    if (shimmerStatus.isSensing)
    {
      //EXG_dataReadyChip1();
      ext_cnt1++;
      if (!(ext_cnt1 % 100))
      {
        __NOP();
        __NOP();
        __NOP();
      }
      EXG_gatherDataStart();
    }
    break; //EXG1
           //DOCK_Pin and gpio_internal share the same pin
  case GPIO_INTERNAL0_Pin:
    if (shimmerStatus.isSensing)
    {

      //EXG_gatherDataStart();
      __NOP();
      __NOP();
      __NOP();
      //EXG_dataReadyChip2();
    }
    break; //EXG2
  case BT_RTS_Pin:
    BtUart_rtsIntCheck();
    break;
  case BT_CONNECTION_Pin:
    BtUart_connectIntCheck();
    break;
  case DOCK_DETECT_Pin:
    DockUart_interruptCheck();
    break;
  case USER_BTN_N_Pin:
    GPIO_userButtonCheck();
    break;
  case SD_DETECT_N_Pin:
    SD_insertedCheck();
    break;
  default:
    break;
  }
}
#endif

uint8_t SD_insertedCheck()
{
  if (HAL_GPIO_ReadPin(SD_DETECT_N_GPIO_Port, SD_DETECT_N_Pin) == GPIO_PIN_RESET)
  { //inserted
    shimmerStatus.isSdInserted = 1;
  }
  else
  {
    shimmerStatus.isSdInserted = 0;
  }
  return shimmerStatus.isSdInserted;
}

void SdPowerOn(void)
{
  HAL_GPIO_WritePin(SW_FLASH_GPIO_Port, SW_FLASH_Pin, GPIO_PIN_SET);
}

void SdPowerOff(void)
{
  HAL_GPIO_WritePin(SW_FLASH_GPIO_Port, SW_FLASH_Pin, GPIO_PIN_RESET);
}

uint8_t isSdPowerOn(void)
{
  return HAL_GPIO_ReadPin(SW_FLASH_GPIO_Port, SW_FLASH_Pin);
}

void setMcuHasSdcardControl(uint8_t state)
{
  HAL_GPIO_WritePin(SW_SD_MCU_DOCK_GPIO_Port, SW_SD_MCU_DOCK_Pin,
      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void vbusPinStateCheck(void)
{
  GPIO_PinState pin = HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin);
  if (pin == GPIO_PIN_SET)
  {
    //Disable GPIO on pin so that USB peripheral can take control
    GPIO_VBUS_init(0);
    //Clear interrupt flag else it triggers multiple times.
    __HAL_GPIO_EXTI_CLEAR_IT(USB_VBUS_Pin);

    MX_USB_OTG_HS_PCD_Init();
#if !USE_USBX
    MX_USB_DEVICE_Init(); //usb pluggedin
#endif
  }
  else if (pin == GPIO_PIN_RESET)
  {
    USB_STATE state = usbPlugInState();
    if (state == USB_CABLE_UNPLUGGED)
    {
#if !USE_USBX
      USBD_DeInit(&hUsbDevice);
#endif
      HAL_PCD_MspDeInit(&hpcd_USB_OTG_HS); //deinit if unplugged
      GPIO_VBUS_init(1);
    }
  }
}

/* USER CODE END 2 */
