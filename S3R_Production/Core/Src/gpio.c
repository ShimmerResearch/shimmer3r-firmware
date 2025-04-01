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

#include <TaskList/shimmer_taskList.h>
#include <log_and_stream_externs.h>

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

uint64_t GPIO_tsLastRelease = 0, GPIO_tsRelease = 0;

/* USER CODE END 1 */

/** Configure pins
     PC14-OSC32_IN (PC14)   ------> RCC_OSC32_IN
     PB3 (JTDO/TRACESWO)   ------> DEBUG_JTDO-SWO
     PA13 (JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
     PA14 (JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
     PC15-OSC32_OUT (PC15)   ------> RCC_OSC32_OUT
     PH0-OSC_IN (PH0)   ------> RCC_OSC_IN
     PH1-OSC_OUT (PH1)   ------> RCC_OSC_OUT
     PA2   ------> USART2_TX
     PA3   ------> USART2_RX
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD,
      CS_BMP390_Pin | SW_FLASH_Pin | SW_SENSE_Pin | CS_LIS2DW12_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, CS_LIS3MDL_Pin | SW_SENSE_IO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, CS_HIGH_G_Pin | SW_MIC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC,
      SW_BT_Pin | CS_LSM6DSV_Pin | CS_ADS7028_Pin | DETECT_N_Pin | SW_SD_MCU_DOCK_Pin,
      GPIO_PIN_RESET);

  /*Configure GPIO pins : PE1 PE0 */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PG12 PG10 PG6 PG7
                           PG8 PG4 PG2 PG3
                           PG5 PG1 PG0 */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_10 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8
      | GPIO_PIN_4 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_1 | GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_BMP390_Pin SW_FLASH_Pin SW_SENSE_Pin CS_LIS2DW12_Pin */
  GPIO_InitStruct.Pin = CS_BMP390_Pin | SW_FLASH_Pin | SW_SENSE_Pin | CS_LIS2DW12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : GPIO_INTERNAL2_Pin GPIO_INTERNAL1_Pin BT_CYSPP_Pin */
  GPIO_InitStruct.Pin = GPIO_INTERNAL2_Pin | GPIO_INTERNAL1_Pin | BT_CYSPP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PA15 PA7 PA4 PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_15 | GPIO_PIN_7 | GPIO_PIN_4 | GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LIS3MDL_DRDY_Pin BT_HOST_WAKE_Pin LIS2MDL_DRDY_Pin
     LSM6DSV_INT1_Pin BMP390_INT_Pin GPIO_EXTERNAL_Pin */
  GPIO_InitStruct.Pin = LIS3MDL_DRDY_Pin | BT_HOST_WAKE_Pin | LIS2MDL_DRDY_Pin
      | LSM6DSV_INT1_Pin | BMP390_INT_Pin | GPIO_EXTERNAL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_LIS3MDL_Pin SW_SENSE_IO_Pin */
  GPIO_InitStruct.Pin = CS_LIS3MDL_Pin | SW_SENSE_IO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : GPIO_INTERNAL0_Pin */
  GPIO_InitStruct.Pin = GPIO_INTERNAL0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIO_INTERNAL0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_DETECT_N_Pin */
  GPIO_InitStruct.Pin = SD_DETECT_N_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SD_DETECT_N_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : CHG_STAT1_Pin CHG_STAT2_Pin */
  GPIO_InitStruct.Pin = CHG_STAT1_Pin | CHG_STAT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_HIGH_G_Pin SW_MIC_Pin */
  GPIO_InitStruct.Pin = CS_HIGH_G_Pin | SW_MIC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PF2 PF5 PF11 PF12
                           PF13 */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : SW_BT_Pin CS_LSM6DSV_Pin CS_ADS7028_Pin DETECT_N_Pin
                           SW_SD_MCU_DOCK_Pin */
  GPIO_InitStruct.Pin = SW_BT_Pin | CS_LSM6DSV_Pin | CS_ADS7028_Pin
      | DETECT_N_Pin | SW_SD_MCU_DOCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PC1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : DOCK_DETECT_Pin */
  GPIO_InitStruct.Pin = DOCK_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DOCK_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BT_CONNECTION_Pin */
  GPIO_InitStruct.Pin = BT_CONNECTION_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BT_CONNECTION_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB2 PB1 PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USER_BTN_Pin */
  GPIO_InitStruct.Pin = USER_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(USER_BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LIS2DW12_INT1_Pin */
  GPIO_InitStruct.Pin = LIS2DW12_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LIS2DW12_INT1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI6_IRQn);
}

/* USER CODE BEGIN 2 */

void GPIO_usbVbusIntInit(uint8_t state)
{
  if (state)
  {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = USB_VBUS_Pin;
#if SUPPORT_SR48_6_0
    /* SR48-6-0 patch for dock detection */
    if (ShimBrd_isBoardSr48_6_0())
    {
      GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    }
    else
    {
      GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    }
#else
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
#endif
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USB_VBUS_GPIO_Port, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(INT_LINE_USB_VBUS, 0, 0);
    HAL_NVIC_EnableIRQ(INT_LINE_USB_VBUS);
  }
  else
  {
    HAL_GPIO_DeInit(USB_VBUS_GPIO_Port, USB_VBUS_Pin);
  }
}

void GPIO_userButtonCheck()
{
  uint8_t pinState = HAL_GPIO_ReadPin(USER_BTN_GPIO_Port, USER_BTN_Pin);
  if (pinState == GPIO_PIN_SET)
  { //pressed
    shimmerStatus.buttonPressed = 1;
  }
  else
  {
    shimmerStatus.buttonPressed = 0;
    GPIO_tsRelease = RTC_get64();
    if (GPIO_tsRelease - GPIO_tsLastRelease > 3277)
    {
      if (shimmerStatus.sensing == 0)
      {
        shimmerStatus.sdlogCmd = SD_LOG_CMD_STATE_START;
        ShimTask_set(TASK_STARTSENSING);
      }
      else
      {
        shimmerStatus.sdlogCmd = SD_LOG_CMD_STATE_STOP;
        ShimTask_set(TASK_STOPSENSING);
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
#if SUPPORT_SR48_6_0
  /* SR48-6-0 patch for VBUS sense - start */
  case USB_VBUS_Pin:
    if (!ShimBrd_isBoardSr48_6_0())
    {
      if (!(ShimTask_getList() & TASK_USB_SETUP))
      {
        ShimTask_set(TASK_USB_SETUP);
      }
      break;
    }
    /* SR48-6-0 patch for VBUS sense - end */
#else  //SUPPORT_SR48_6_0
  case USB_VBUS_Pin:
    if (!(ShimTask_getList() & TASK_USB_SETUP))
    {
      ShimTask_set(TASK_USB_SETUP);
    }
    break;
#endif //SUPPORT_SR48_6_0
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
    if (shimmerStatus.sensing)
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
    if (shimmerStatus.sensing)
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
    ShimTask_set(TASK_SETUP_DOCK);
    break;
#if SUPPORT_SR48_6_0
    /* SR48-6-0 patch for dock detection - start */
  case SR48_6_0_BOOT0_USER_BTN_Pin:
    if (ShimBrd_isBoardSr48_6_0())
    {
      /* Re-purposing SR48-6-0 BOOT0/USER button interrupt for dock detection*/
      DockUart_interruptCheck();
      ShimTask_set(TASK_SETUP_DOCK);
      break;
    }
    /* SR48-6-0 patch for dock detection - end */
    /* SR48-6-0 patch for VBUS sense - start */
  case USB_VBUS_Pin:
    if (ShimBrd_isBoardSr48_6_0())
    {
      if (!(ShimTask_getList() & TASK_USB_SETUP))
      {
        ShimTask_set(TASK_USB_SETUP);
      }
      break;
    }
    /* SR48-6-0 patch for VBUS sense - end */
#endif //SUPPORT_SR48_6_0
  case USER_BTN_Pin:
    GPIO_userButtonCheck();
    break;
  case SD_DETECT_N_Pin:
    CheckSdInslot();
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
    CheckSdInslot();
    break;
  default:
    break;
  }
}
#endif

uint8_t CheckSdInslot(void)
{
  if (HAL_GPIO_ReadPin(SD_DETECT_N_GPIO_Port, SD_DETECT_N_Pin) == GPIO_PIN_RESET)
  { //inserted
    shimmerStatus.sdInserted = 1;
  }
  else
  {
    shimmerStatus.sdInserted = 0;
  }
  return shimmerStatus.sdInserted;
}

uint8_t isSdPowerOn(void)
{
  return HAL_GPIO_ReadPin(SW_FLASH_GPIO_Port, SW_FLASH_Pin);
}

/*
 * The following pins are utilised differently in each expansion board:
 * - GPIO_ADC_INT_EXP0_Pin
 * - GPIO_ADC_INT_EXP1_Pin
 * - GPIO_ADC_INT_EXP2_Pin
 * - GPIO_ADC_INT_EXP3_Pin
 * - GPIO_INTERNAL0_Pin
 * - GPIO_INTERNAL1_Pin
 * - GPIO_INTERNAL2_Pin
 * */
void gpioInitPerBoard(void)
{
  shimmer_expansion_brd *daughtCardId = ShimBrd_getDaughtCardId();
  if (daughtCardId->exp_brd_id == EXP_BRD_GSR_UNIFIED)
  {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /* GPIO_ADC_INT_EXP0_Pin:
     * GPIO_ADC_INT_EXP1_Pin:
     * PPG ADCs. Also connected to I2C4. Allow code ADC to manage. */

#if SUPPORT_SR48_6_0
    if (ShimBrd_isBoardSr48_6_0())
    {
      /* GPIO_ADC_INT_EXP2_Pin
       * Controls whether I2C4 connected to PPG connector */
      Board_SW_I2C4_ON_PPG(0);
      GPIO_InitStruct.Pin = SR48_6_0_GPIO_ADC_INT_EXP2_Pin;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      HAL_GPIO_Init(SR48_6_0_GPIO_ADC_INT_EXP2_GPIO_Port, &GPIO_InitStruct);
    }
#else
    if (!ShimBrd_isBoardSr48_7_0())
    {
      //TODO GPIO_INTERNAL3? to control I2C on PPG connector
    }
#endif

    /* GPIO_ADC_INT_EXP3_Pin:
     * GSR ADC. Allow code ADC to manage. */

    /* GPIO_INTERNAL0_Pin:
     * GSR Range A0 */
    HAL_GPIO_WritePin(GPIO_INTERNAL0_GPIO_Port, GPIO_INTERNAL0_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_INTERNAL0_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_INTERNAL0_GPIO_Port, &GPIO_InitStruct);

    /* GPIO_INTERNAL1_Pin:
     * GSR Range A1 */
    HAL_GPIO_WritePin(GPIO_INTERNAL1_GPIO_Port, GPIO_INTERNAL1_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_INTERNAL1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_INTERNAL1_GPIO_Port, &GPIO_InitStruct);

    /* GPIO_INTERNAL2_Pin:
     * PPG connector power. 0 = power off, 1 = power on */
    HAL_GPIO_WritePin(GPIO_INTERNAL2_GPIO_Port, GPIO_INTERNAL2_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_INTERNAL2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_INTERNAL2_GPIO_Port, &GPIO_InitStruct);

#if SUPPORT_SR48_6_0
    if (ShimBrd_isBoardSr48_6_0())
    {
      /* SR48-6-0 patch for dock detection - start */

      /* SR48-6-0 S3R prototype has incorrect dock pin handling so we need to
       * re-purpose the dock detect pin as an output to let the dock know when a
       * Shimmer is docked. */
      HAL_NVIC_DisableIRQ(INT_LINE_DOCK_DETECT);
      HAL_GPIO_DeInit(DOCK_DETECT_GPIO_Port, DOCK_DETECT_Pin);

      HAL_GPIO_WritePin(DOCK_DETECT_GPIO_Port, DOCK_DETECT_Pin, GPIO_PIN_SET);
      GPIO_InitStruct.Pin = DOCK_DETECT_Pin;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      HAL_GPIO_Init(DOCK_DETECT_GPIO_Port, &GPIO_InitStruct);

      /* SR48-6-0 doesn't have a DETECT_N pin so deinit */
      HAL_GPIO_DeInit(DETECT_N_GPIO_Port, DETECT_N_Pin);

      /* SR48-6-0 has user button connected to BOOT0 pin PH3 and nothing
       * connected to INT6 so disabling it here and using BOOT0 as an interrupt
       * instead for dock detection. */
      HAL_NVIC_DisableIRQ(INT_LINE_USER_BTN);
      HAL_GPIO_DeInit(USER_BTN_GPIO_Port, USER_BTN_Pin);

      GPIO_InitStruct.Pin = SR48_6_0_BOOT0_USER_BTN_Pin;
      GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      HAL_GPIO_Init(SR48_6_0_BOOT0_USER_BTN_GPIO_Port, &GPIO_InitStruct);

      HAL_NVIC_SetPriority(INT_LINE_SR48_6_0_BOOT0_USER_BTN, 0, 0);
      HAL_NVIC_EnableIRQ(INT_LINE_SR48_6_0_BOOT0_USER_BTN);

      /* SR48-6-0 patch for dock detection - end */

      /* SR48-6-0 has CHG_STAT connections attached to different MCU pins */
      HAL_GPIO_DeInit(CHG_STAT1_GPIO_Port, CHG_STAT1_Pin);
      HAL_GPIO_DeInit(CHG_STAT2_GPIO_Port, CHG_STAT2_Pin);

      GPIO_InitStruct.Pin = SR48_6_0_CHG_STAT1_Pin | SR48_6_0_CHG_STAT2_Pin;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

      /*Configure GPIO pin Output Level */
      HAL_GPIO_WritePin(SR48_6_0_SW_GSR_GPIO_Port, SR48_6_0_SW_GSR_Pin, GPIO_PIN_RESET);

      /*Configure GPIO pins : SW_GSR_Pin */
      GPIO_InitStruct.Pin = SR48_6_0_SW_GSR_Pin;
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

      /* SR48-6-0 uses MCU pins for ADC channels and not the ADS7028 */
      HAL_GPIO_DeInit(CS_ADS7028_GPIO_Port, CS_ADS7028_Pin);
    }
#endif

    GSR_setActiveResistor(HW_RES_40K);
  }
}

void vbusPinStateCheck(void)
{
  GPIO_PinState pin = HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin);
  if (pin == GPIO_PIN_SET)
  {
    if (hUsbDevice.pDesc == NULL)
    {
      //Enable USB peripheral
#if SUPPORT_SR48_6_0
      /* SR48-6-0 patch for VBUS sense - start */
      if (ShimBrd_isBoardSr48_6_0())
      {
        MX_USB_OTG_HS_PCD_Init_NoVbusSense();
      }
      else
      {
        //Disable GPIO interrupt on pin so that USB peripheral can take control
        GPIO_usbVbusIntInit(0);
        //Clear interrupt flag else it triggers multiple times.
        __HAL_GPIO_EXTI_CLEAR_IT(USB_VBUS_Pin);

        MX_USB_OTG_HS_PCD_Init();
      }
      /* SR48-6-0 patch for VBUS sense - end */
#else //SUPPORT_SR48_6_0
      //Disable GPIO interrupt on pin so that USB peripheral can take control
      GPIO_usbVbusIntInit(0);
      //Clear interrupt flag else it triggers multiple times.
      __HAL_GPIO_EXTI_CLEAR_IT(USB_VBUS_Pin);

      MX_USB_OTG_HS_PCD_Init();
#endif //SUPPORT_SR48_6_0
#if !USE_USBX
      MX_USB_DEVICE_Init();
#endif //USE_USBX
    }
  }
  else if (pin == GPIO_PIN_RESET)
  {
#if SUPPORT_SR48_6_0
    /* SR48-6-0 patch for VBUS sense - start */
    if (ShimBrd_isBoardSr48_6_0())
    {
      if (hUsbDevice.pDesc != NULL)
      {
#if !USE_USBX
        USBD_DeInit(&hUsbDevice);
#endif
        HAL_PCD_MspDeInit_NoVbusSense(&hpcd_USB_OTG_HS);
      }
    }
    else
    {
      USB_STATE state = usbPlugInState();
      if (state == USB_CABLE_UNPLUGGED)
      {
        //Disable USB peripheral
#if !USE_USBX
        USBD_DeInit(&hUsbDevice);
#endif //USE_USBX
        HAL_PCD_MspDeInit(&hpcd_USB_OTG_HS);

        //Re-enable GPIO interrupt on pin
        GPIO_usbVbusIntInit(1);
      }
    }
    /* SR48-6-0 patch for VBUS sense - end */
#else //SUPPORT_SR48_6_0
    USB_STATE state = usbPlugInState();
    if (state == USB_CABLE_UNPLUGGED)
    {
      //Disable USB peripheral
#if !USE_USBX
      USBD_DeInit(&hUsbDevice);
#endif //USE_USBX
      HAL_PCD_MspDeInit(&hpcd_USB_OTG_HS);

      //Re-enable GPIO interrupt on pin
      GPIO_usbVbusIntInit(1);
    }
#endif //SUPPORT_SR48_6_0
  }
}

void initBtPins(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  Board_BT_LP_MODE(0);
  Board_BT_CP_ROLE(0);
  /* Hold BT module in reset until we're ready */
  Board_BT_RST_N(0);

  GPIO_InitStruct.Pin = BT_CP_ROLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BT_CP_ROLE_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BT_CYSPP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BT_CYSPP_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BT_RST_Pin | BT_LP_MODE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BT_CONNECTION_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BT_CONNECTION_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BT_HOST_WAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BT_HOST_WAKE_GPIO_Port, &GPIO_InitStruct);
}

void initBtInterrupts(void)
{
  HAL_NVIC_SetPriority(INT_LINE_BT_CONNECTION, 15, 0);
  HAL_NVIC_EnableIRQ(INT_LINE_BT_CONNECTION);

  HAL_NVIC_SetPriority(INT_LINE_BT_HOST_WAKE, 15, 0);
  HAL_NVIC_EnableIRQ(INT_LINE_BT_HOST_WAKE);

  HAL_NVIC_SetPriority(INT_LINE_BT_CYSPP, 15, 0);
  HAL_NVIC_EnableIRQ(INT_LINE_BT_CYSPP);
}

void deinitBtPins(void)
{
  HAL_NVIC_DisableIRQ(INT_LINE_BT_CONNECTION);
  HAL_NVIC_DisableIRQ(INT_LINE_BT_HOST_WAKE);
  HAL_NVIC_DisableIRQ(INT_LINE_BT_CYSPP);

  //Default state for all connected Vela IF820 pins when not in use is floating
  HAL_GPIO_DeInit(BT_CP_ROLE_GPIO_Port, BT_CP_ROLE_Pin);
  HAL_GPIO_DeInit(GPIOD, BT_CYSPP_Pin | BT_RST_Pin | BT_LP_MODE_Pin | BT_CONNECTION_Pin);
  HAL_GPIO_DeInit(BT_HOST_WAKE_GPIO_Port, BT_HOST_WAKE_Pin);
}

void setBtPower(uint8_t state)
{
  Board_SW_BT(state);
  shimmerStatus.btPowerOn = state;
}

void initSpi1CsOutputs(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitStruct.Pin = CS_LSM6DSV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  lsm6dsv_unselectDevice();
  adxl371_unselectDevice();
  bmp3_unselectDevice();
}

void deinitSpi1CsOutputs(void)
{
}

/* USER CODE END 2 */
