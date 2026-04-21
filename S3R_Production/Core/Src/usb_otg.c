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
#include "main.h" // for USB_VBUS_Pin/Port

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
  hpcd_USB_OTG_HS.Init.speed = PCD_SPEED_HIGH;
  hpcd_USB_OTG_HS.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
  hpcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
  /* LPM kept DISABLED.  Enabling it on STM32U5 OTG_HS requires the
   * application to service LPM L1 suspend/resume callbacks; with USBX
   * the default path does not, which can cause the link to stay in L1
   * after a host LPM probe.  Mac USB-C xHCI root ports probe LPM during
   * HS enumeration but will happily enumerate a non-LPM device. */
  hpcd_USB_OTG_HS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_HS.Init.use_dedicated_ep1 = DISABLE;
  hpcd_USB_OTG_HS.Init.vbus_sensing_enable = DISABLE;
  /* Synopsys internal OTG DMA ENABLED.
   *
   * With dma_enable = ENABLE the OTG core DMA engine fetches/pushes
   * transfer payload directly from/to the application buffer supplied to
   * HAL_PCD_EP_Transmit/Receive, bypassing the CPU FIFO-copy loop. This
   * is a throughput and IRQ-load win (no more per-packet FIFO copy in
   * the OTG ISR), but imposes hard requirements on every bulk-endpoint
   * buffer handed to the USBX stack:
   *
   *   1) Pointer alignment: the Synopsys DMA requires the transfer
   *      buffer pointer to be 4-byte aligned. Misalignment -> AHB bus
   *      fault -> HardFault. Audited and addressed for every USBX call
   *      site -- see Doc/USBX_DMA_Alignment_And_Cache.md.
   *        * MSC: buffers come from ux_device_byte_pool_buffer[] which
   *          is ALIGN_32BYTES (app_usbx_device.c).
   *        * CDC RX: cdc_rx_buffer[] is ALIGN_32BYTES
   *          (ux_device_cdc_acm.c).
   *        * CDC TX: USBX_CDC_ACM_Transmit() now bounces caller payload
   *          through the ALIGN_32BYTES cdc_tx_buffer[] before handing
   *          it to USBX, so caller pointer alignment is irrelevant.
   *
   *   2) D-cache coherency: OTG internal DMA bypasses the D-cache.
   *      Every CPU<->DMA handoff needs HAL_DCACHE_CleanByAddr before
   *      DMA reads CPU-written data, and HAL_DCACHE_InvalidateByAddr
   *      before CPU reads DMA-written data. Ranges are rounded to
   *      whole 32-byte cache lines at both ends. Audited and in place
   *      for MSC R/W and CDC TX/RX.
   *
   *   3) Memory placement: all buffers live in main SRAM1/2/3, which
   *      the OTG_HS DMA can reach over the AHB bus matrix. No linker
   *      changes required.
   *
   *   4) FIFO sizing (HAL_PCDEx_SetRxFiFo / SetTxFiFo in
   *      app_usbx_device.c::USBX_APP_Device_Init) stays valid; in DMA
   *      mode the FIFOs are still used as the staging area between the
   *      AHB DMA engine and the USB serial interface. Current budget
   *      (RX 0x240 + TX0..4) totals 1008/1024 HS words and is unchanged.
   *
   * STATUS 2026-04-21: DMA mode DISABLED again.
   * ------------------------------------------
   * Extensive bring-up showed that the USBX-in-standalone-mode x
   * OTG-DMA combination has layered pathologies in ST's HAL/USBX that
   * show up as CDC-ACM endpoint wedges after the first host-side pause
   * (Windows selective-suspend / CDC read-queue back-pressure):
   *   - HAL_PCD_EP_Abort -> USB_EPStopXfer does not clear its internal
   *     state cleanly if the host has stopped polling.
   *   - USBX's own _ux_dcd_stm32_transfer_abort leaves the DCD ED
   *     TRANSFER flag set, wedging _ux_dcd_stm32_transfer_run.
   *   - USBX arms ZLP terminators that the Synopsys core parks waiting
   *     to transmit, with no way to recover cleanly.
   * MSC alone works fine in DMA mode (zero-copy from USBD_STORAGE_*
   * into the DMA engine, no abort/recovery ever needed on the bulk-IN
   * side). CDC is the pain point.
   *
   * Slave mode (FIFO copy) works reliably for both CDC and MSC on this
   * platform, and MSC throughput in slave mode is still adequate for
   * SD-card-backup-over-USB use cases. All the alignment / cache /
   * bounce-buffer / DTR-gate / stall-watchdog / fault-register
   * instrumentation stays in place -- it is correct regardless of
   * DMA state, and makes slave-mode CDC more robust too.
   *
   * If DMA mode is re-attempted in future, the known outstanding
   * issues are captured in Doc/USBX_DMA_Alignment_And_Cache.md. */
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
  /* Keep the NoVbusSense variant consistent with the main init. See the
   * large comment above MX_USB_OTG_HS_PCD_Init()'s dma_enable line for
   * the reason this is currently DISABLE (USBX + OTG-DMA CDC wedges).
   * If/when DMA mode is re-attempted for the main path, flip both in
   * lockstep. */
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
    MX_USBX_Device_Init();
  }
}

void USB_deinit(void)
{
  if (USBX_IsInitialised())
  {
    MX_USBX_Device_DeInit();
  }
}

/* USER CODE END 1 */
