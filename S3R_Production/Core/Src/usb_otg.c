/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    usb_otg.c
 * @brief   This file provides code for the configuration
 *          of the USB_OTG instances.
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
#include "usb_otg.h"

/* USER CODE BEGIN 0 */
#include "app_usbx_device.h"
#include "dcache.h"
#include "main.h"                // for USB_VBUS_Pin/Port
#include "stm32u5xx_ll_system.h" // for LL_SYSCFG_DisableOTGPHY()

/* Currently-selected USB bus speed.  Defaults to Full-Speed; may be
 * overridden at boot via USB_setSpeed() (e.g. from an EEPROM setting)
 * before USB_init() is called.  Both MX_USB_OTG_HS_PCD_Init() and the
 * FIFO allocation in USBX_APP_Device_Init() read this value. */
static USB_Speed_t usb_selected_speed = USB_SPEED_FULL;

static uint32_t USB_getPcdSpeed(void);
static void USB_OTG_HS_PhyTeardown(void);

/* USER CODE END 0 */

PCD_HandleTypeDef hpcd_USB_OTG_HS;

/* USB_OTG_HS init function */

void MX_USB_OTG_HS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_HS_Init 0 */

  /* USER CODE END USB_OTG_HS_Init 0 */

  /* USER CODE BEGIN USB_OTG_HS_Init 1 */

  /* USER CODE END USB_OTG_HS_Init 1 */
  hpcd_USB_OTG_HS.Instance = USB_OTG_HS;
  hpcd_USB_OTG_HS.Init.dev_endpoints = 9;
  hpcd_USB_OTG_HS.Init.speed = USB_getPcdSpeed();
  hpcd_USB_OTG_HS.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
  hpcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.use_dedicated_ep1 = DISABLE;
  hpcd_USB_OTG_HS.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.dma_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_HS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_HS_Init 2 */

  /* USER CODE END USB_OTG_HS_Init 2 */
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *pcdHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (pcdHandle->Instance == USB_OTG_HS)
  {
    /* USER CODE BEGIN USB_OTG_HS_MspInit 0 */

    //https://community.st.com/t5/stm32-mcus-products/stm32u595-timeout-in-usb-corereset/td-p/618743
    /* Enable SYSCFG Clock */
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    //HAL_GPIO_DeInit(USB_VBUS_GPIO_Port, USB_VBUS_Pin);
    /* USER CODE END USB_OTG_HS_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
    PeriphClkInit.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_HSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /** Set the OTG PHY reference clock selection
     */
    HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);

    /* USB_OTG_HS clock enable */
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USBPHYC_CLK_ENABLE();

    /* Enable VDDUSB */
    if (__HAL_RCC_PWR_IS_CLK_DISABLED())
    {
      __HAL_RCC_PWR_CLK_ENABLE();
      HAL_PWREx_EnableVddUSB();

      /*configure VOSR register of USB*/
      HAL_PWREx_EnableUSBHSTranceiverSupply();
      __HAL_RCC_PWR_CLK_DISABLE();
    }
    else
    {
      HAL_PWREx_EnableVddUSB();

      /*configure VOSR register of USB*/
      HAL_PWREx_EnableUSBHSTranceiverSupply();
    }

    /*Configuring the SYSCFG registers OTG_HS PHY*/
    /*OTG_HS PHY enable*/
    HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

    /* USB_OTG_HS interrupt Init */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    /* USER CODE BEGIN USB_OTG_HS_MspInit 1 */
    CRC_setState(CRC_SRC_USB, 1);
    /* USER CODE END USB_OTG_HS_MspInit 1 */
  }
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *pcdHandle)
{

  if (pcdHandle->Instance == USB_OTG_HS)
  {
    /* USER CODE BEGIN USB_OTG_HS_MspDeInit 0 */

    /* Fully power-down the embedded HS PHY and reset the OTG core so
     * that the next HAL_PCD_Init() can take effect at a new speed
     * without needing a power cycle.  See USB_OTG_HS_PhyTeardown(). */
    USB_OTG_HS_PhyTeardown();

    /* USER CODE END USB_OTG_HS_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    __HAL_RCC_USBPHYC_CLK_DISABLE();

    /* USB_OTG_HS interrupt Deinit */
    HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
    /* USER CODE BEGIN USB_OTG_HS_MspDeInit 1 */
    CRC_setState(CRC_SRC_USB, 0);
    /* USER CODE END USB_OTG_HS_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

void USB_setSpeed(USB_Speed_t speed)
{
  usb_selected_speed = speed;
}

USB_Speed_t USB_getSpeed(void)
{
  return usb_selected_speed;
}

/* Map the logical USB_Speed_t selection to the HAL PCD speed constant. */
static uint32_t USB_getPcdSpeed(void)
{
  return (usb_selected_speed == USB_SPEED_FULL) ? PCD_SPEED_FULL : PCD_SPEED_HIGH;
}

/**
 * @brief  Fully power-down the embedded HS PHY and force-reset the OTG
 *         peripheral so that a subsequent HAL_PCD_Init() with a different
 *         speed setting (HS vs FS) takes effect without a power cycle.
 *
 * Just gating the clocks (the CubeMX default) leaves the OTG core
 * registers and the PHY block powered/configured from the previous
 * init.  The next USB_DevInit() writes DCFG.DSPD for the new speed, but
 * the PHY ignores it because it was never reset — the link always
 * re-negotiates at the old speed until VDD is removed.
 *
 * Shared by HAL_PCD_MspDeInit() and HAL_PCD_MspDeInit_NoVbusSense().
 */
static void USB_OTG_HS_PhyTeardown(void)
{
  /* 1. Pulse the peripheral reset line FIRST, while the OTG core + PHY
   *    are still fully clocked and powered.  This is the only step that
   *    actually clears the OTG digital core registers (GUSBCFG,
   *    DCFG.DSPD, GAHBCFG, …) back to their reset values.  On STM32U5
   *    the OTG core and the embedded HS PHY share a single reset bit
   *    (RCC_AHB2RSTR1_OTGRST), so this one macro covers both.
   *
   *    __DSB() ensures the SET_BIT write has reached the RCC register
   *    before we issue the matching CLEAR_BIT — a handful of __NOP()s
   *    happens to work in practice but is not a formal barrier. */
  __HAL_RCC_USB_OTG_HS_FORCE_RESET();
  __DSB();
  __HAL_RCC_USB_OTG_HS_RELEASE_RESET();
  __DSB();

  /* 2. Disable the PHY via SYSCFG before removing its supplies.
   *    HAL only exposes HAL_SYSCFG_EnableOTGPHY(); the matching disable
   *    is only available at the LL layer. */
  LL_SYSCFG_DisableOTGPHY();

  /* 3. Remove HS transceiver supply + VddUSB so the analog PHY block
   *    actually loses state.  Without this, the PHY stays latched on
   *    its previous HS/FS configuration and the next HAL_PCD_Init()
   *    re-programming DCFG.DSPD has no visible effect on the line. */
  if (__HAL_RCC_PWR_IS_CLK_DISABLED())
  {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_DisableUSBHSTranceiverSupply();
    HAL_PWREx_DisableVddUSB();
    __HAL_RCC_PWR_CLK_DISABLE();
  }
  else
  {
    HAL_PWREx_DisableUSBHSTranceiverSupply();
    HAL_PWREx_DisableVddUSB();
  }
}

#if SUPPORT_SR48_6_0
void MX_USB_OTG_HS_PCD_Init_NoVbusSense(void)
{

  /* USER CODE BEGIN USB_OTG_HS_Init 0 */

  /* USER CODE END USB_OTG_HS_Init 0 */

  /* USER CODE BEGIN USB_OTG_HS_Init 1 */

  /* USER CODE END USB_OTG_HS_Init 1 */
  hpcd_USB_OTG_HS.Instance = USB_OTG_HS;
  hpcd_USB_OTG_HS.Init.dev_endpoints = 9;
  hpcd_USB_OTG_HS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_HS.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
  hpcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.use_dedicated_ep1 = DISABLE;
  hpcd_USB_OTG_HS.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.dma_enable = DISABLE;

  hpcd_USB_OTG_HS.MspInitCallback = HAL_PCD_MspInit_NoVbusSense;
  hpcd_USB_OTG_HS.MspDeInitCallback = HAL_PCD_MspDeInit_NoVbusSense;

  if (HAL_PCD_Init(&hpcd_USB_OTG_HS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_HS_Init 2 */

  /* USER CODE END USB_OTG_HS_Init 2 */
}

void HAL_PCD_MspInit_NoVbusSense(PCD_HandleTypeDef *pcdHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (pcdHandle->Instance == USB_OTG_HS)
  {
    /* USER CODE BEGIN USB_OTG_HS_MspInit 0 */

    //https://community.st.com/t5/stm32-mcus-products/stm32u595-timeout-in-usb-corereset/td-p/618743
    /* Enable SYSCFG Clock */
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* USER CODE END USB_OTG_HS_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
    PeriphClkInit.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_HSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /** Set the OTG PHY reference clock selection
     */
    HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);

    /* USB_OTG_HS clock enable */
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USBPHYC_CLK_ENABLE();

    /* Enable VDDUSB */
    if (__HAL_RCC_PWR_IS_CLK_DISABLED())
    {
      __HAL_RCC_PWR_CLK_ENABLE();
      HAL_PWREx_EnableVddUSB();

      /*configure VOSR register of USB*/
      HAL_PWREx_EnableUSBHSTranceiverSupply();
      __HAL_RCC_PWR_CLK_DISABLE();
    }
    else
    {
      HAL_PWREx_EnableVddUSB();

      /*configure VOSR register of USB*/
      HAL_PWREx_EnableUSBHSTranceiverSupply();
    }

    /*Configuring the SYSCFG registers OTG_HS PHY*/
    /*OTG_HS PHY enable*/
    HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

    /* USB_OTG_HS interrupt Init */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    /* USER CODE BEGIN USB_OTG_HS_MspInit 1 */
    CRC_setState(CRC_SRC_USB, 1);
    /* USER CODE END USB_OTG_HS_MspInit 1 */
  }
}

void HAL_PCD_MspDeInit_NoVbusSense(PCD_HandleTypeDef *pcdHandle)
{

  if (pcdHandle->Instance == USB_OTG_HS)
  {
    /* USER CODE BEGIN USB_OTG_HS_MspDeInit 0 */

    /* Fully power-down the embedded HS PHY and reset the OTG core so
     * that the next HAL_PCD_Init() can take effect at a new speed
     * without needing a power cycle.  See USB_OTG_HS_PhyTeardown(). */
    USB_OTG_HS_PhyTeardown();

    /* USER CODE END USB_OTG_HS_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    __HAL_RCC_USBPHYC_CLK_DISABLE();

    /* USB_OTG_HS interrupt Deinit */
    HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
    /* USER CODE BEGIN USB_OTG_HS_MspDeInit 1 */

    //https://community.st.com/t5/stm32-mcus-products/stm32u595-timeout-in-usb-corereset/td-p/618743
    /* Enable SYSCFG Clock */
    __HAL_RCC_SYSCFG_CLK_DISABLE();
    CRC_setState(CRC_SRC_USB, 0);
    /* USER CODE END USB_OTG_HS_MspDeInit 1 */
  }
}
#endif

void USB_init(void)
{
  if (!USBX_IsInitialised())
  {
    DCACHE_invalidate();
    USB_setSpeed(ShimEeprom_getSensorSettingsPage()->usbFullSpeed ? USB_SPEED_FULL : USB_SPEED_HIGH);
    MX_USBX_Device_Init();
  }
}

void USB_deinit(void)
{
  if (USBX_IsInitialised())
  {
    MX_USBX_Device_DeInit();

    /* Allow the embedded HS PHY analog supplies (VddUSB + HS transceiver)
     * to fully discharge after HAL_PCD_MspDeInit() disabled them.
     * Without this delay, a rapid USB_deinit() → USB_init() sequence
     * (e.g. unplug/replug or runtime speed change) may re-enable the
     * supplies before the PHY block has lost state, causing the PHY to
     * retain its previous speed configuration despite DCFG.DSPD being
     * reprogrammed.  50 ms is conservative; the internal LDOs typically
     * decay within ~10 ms. */
    HAL_Delay(50);
  }
}

/* USER CODE END 1 */
