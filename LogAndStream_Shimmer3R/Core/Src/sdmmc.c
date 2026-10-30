/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    sdmmc.c
 * @brief   This file provides code for the configuration
 *          of the SDMMC instances.
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
#include "sdmmc.h"

/* USER CODE BEGIN 0 */
#include "bsp_driver_sd.h"
#include "gpio.h"
#include "stdio.h"
#include "ux_device_msc.h"
extern STATTypeDef stat;
volatile sdOwner_t currentSdOwner = OWNER_IDLE;
volatile uint8_t sdTransferDone = 0;
/* Set by HAL_SD_ErrorCallback, which releases every wait loop by raising the
 * SAME completion flags a success raises (deliberate anti-deadlock). Without
 * this flag an SD/DMA error is indistinguishable from a completed transfer,
 * so HAL_SD_SharedRead/Write would return HAL_OK over garbage data. */
volatile uint8_t sdTransferError = 0;

/* SDMMC_CK is the 48 MHz kernel clock divided by 2 x CLKDIV, or undivided at
 * CLKDIV 0. Default Speed allows 25 MHz and High Speed 50 MHz, so a card runs
 * at 24 MHz until it has been switched to High Speed, and only then at 48. */
#define SD_CLKDIV_DEFAULT_SPEED 1U
#define SD_CLKDIV_HIGH_SPEED    0U
static uint8_t SD_enterHighSpeed(void);
/* USER CODE END 0 */

SD_HandleTypeDef hsd1;

/* SDMMC1 init function */

void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  //Initialise will fail if an SD card is not detected
  LogAndStream_checkSdInSlot();
  shimmerStatus.sdBadFile = 1;
  if (shimmerStatus.sdInserted)
  {
    /* USER CODE END SDMMC1_Init 1 */
    hsd1.Instance = SDMMC1;
    hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
    hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    /* SD_CLKDIV_DEFAULT_SPEED: see SD_enterHighSpeed() */
    hsd1.Init.ClockDiv = 1;
    if (HAL_SD_Init(&hsd1) != HAL_OK)
    {
      /* Do not call Error_Handler() here — HAL_SD_Init can legitimately
       * fail during a hot-swap (e.g. dock→MCU transition).  Leave
       * sdBadFile = 1 so callers know the card is unusable and allow
       * the system to continue or retry on the next dock/undock cycle. */
      hsd1.Instance = NULL;
    }
    /* USER CODE BEGIN SDMMC1_Init 2 */
    else
    {
      /* 48 MHz once the card is in High Speed. A card that cannot switch
       * stays at 24 MHz in Default Speed, which still works. */
      (void) SD_enterHighSpeed();

      shimmerStatus.sdBadFile = 0;
      shimmerStatus.sdPeripheralInit = 1;
    }
  }
  /* USER CODE END SDMMC1_Init 2 */
}

void HAL_SD_MspInit(SD_HandleTypeDef *sdHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };
  if (sdHandle->Instance == SDMMC1)
  {
    /* USER CODE BEGIN SDMMC1_MspInit 0 */

    /* USER CODE END SDMMC1_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SDMMC | RCC_PERIPHCLK_CLK48;
    PeriphClkInit.IclkClockSelection = RCC_CLK48CLKSOURCE_HSI48;
    PeriphClkInit.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_CLK48;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* SDMMC1 clock enable */
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**SDMMC1 GPIO Configuration
    PD2     ------> SDMMC1_CMD
    PC11     ------> SDMMC1_D3
    PC12     ------> SDMMC1_CK
    PC10     ------> SDMMC1_D2
    PC9     ------> SDMMC1_D1
    PC8     ------> SDMMC1_D0
    */
    GPIO_InitStruct.Pin = SDMMC1_CMD_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(SDMMC1_CMD_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SDMMC1_D3_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(SDMMC1_D3_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SDMMC1_CK_Pin | SDMMC1_D2_Pin | SDMMC1_D1_Pin | SDMMC1_D0_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* SDMMC1 interrupt Init */
    HAL_NVIC_SetPriority(SDMMC1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
    /* USER CODE BEGIN SDMMC1_MspInit 1 */

    /* USER CODE END SDMMC1_MspInit 1 */
  }
}

void HAL_SD_MspDeInit(SD_HandleTypeDef *sdHandle)
{

  if (sdHandle->Instance == SDMMC1)
  {
    /* USER CODE BEGIN SDMMC1_MspDeInit 0 */

    /* USER CODE END SDMMC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SDMMC1_CLK_DISABLE();

    /**SDMMC1 GPIO Configuration
    PD2     ------> SDMMC1_CMD
    PC11     ------> SDMMC1_D3
    PC12     ------> SDMMC1_CK
    PC10     ------> SDMMC1_D2
    PC9     ------> SDMMC1_D1
    PC8     ------> SDMMC1_D0
    */
    HAL_GPIO_DeInit(SDMMC1_CMD_GPIO_Port, SDMMC1_CMD_Pin);

    HAL_GPIO_DeInit(GPIOC,
        SDMMC1_D3_Pin | SDMMC1_CK_Pin | SDMMC1_D2_Pin | SDMMC1_D1_Pin | SDMMC1_D0_Pin);

    /* SDMMC1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
    /* USER CODE BEGIN SDMMC1_MspDeInit 1 */

    /* USER CODE END SDMMC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* Switches an initialised card to High Speed, and only then raises SDMMC_CK
 * to 48 MHz. Returns 1 if the bus is now at 48 MHz.
 *
 * HAL_SD_Init() identifies the card and sets the clock from hsd1.Init.ClockDiv
 * while the card is still in Default Speed (25 MHz max). Running that at
 * 48 MHz (DEV-706's ClockDiv 0) was out of spec, and a SanDisk Ultra
 * SDSQUA4-032G failed CMD16 with CMD_CRC_FAIL there, so it never initialised.
 * FatFs re-identifies the card through BSP_SD_Init() on its first file access
 * after boot, which puts it back in Default Speed, so that path comes through
 * here as well. */
static uint8_t SD_enterHighSpeed(void)
{
  HAL_SD_CardStatusTypeDef cardStatus;

  /* HAL_OK alone does not mean the card switched: an SDSC card
   * (CARD_NORMAL_SPEED) gets HAL_OK and stays in Default Speed. */
  if (HAL_SD_ConfigSpeedBusOperation(&hsd1, SDMMC_SPEED_MODE_HIGH) != HAL_OK
      || hsd1.SdCard.CardSpeed == CARD_NORMAL_SPEED)
  {
    return 0;
  }

  MODIFY_REG(hsd1.Instance->CLKCR, SDMMC_CLKCR_CLKDIV, SD_CLKDIV_HIGH_SPEED);

  /* The HAL's SD_SwitchSpeed() reports a data timeout as success, so read
   * the 64-byte SD status at the new clock before relying on it. */
  if (HAL_SD_GetCardStatus(&hsd1, &cardStatus) != HAL_OK)
  {
    MODIFY_REG(hsd1.Instance->CLKCR, SDMMC_CLKCR_CLKDIV, SD_CLKDIV_DEFAULT_SPEED);
    return 0;
  }
  return 1;
}

/* Overrides the weak CubeMX BSP_SD_Init() in FATFS/Target/bsp_driver_sd.c,
 * which FatFs calls from disk_initialize() on its first file access after
 * boot (with one volume, FATFS_LinkDriver() links once and the driver's
 * is_initialized latch is never reset). When the card fails to initialise,
 * MX_SDMMC1_SD_Init() leaves
 * hsd1.Instance NULL; running HAL_SD_Init() on that handle reads SDMMC
 * registers through the NULL pointer and HardFaults, so a card the firmware
 * cannot initialise stopped the unit booting. Report it as not ready instead,
 * and the file operation fails with FR_NOT_READY. */
uint8_t BSP_SD_Init(void)
{
  uint8_t sd_state = MSD_OK;

  if (hsd1.Instance == NULL)
  {
    return MSD_ERROR;
  }
  if (BSP_SD_IsDetected() != SD_PRESENT)
  {
    return MSD_ERROR_SD_NOT_PRESENT;
  }
  sd_state = HAL_SD_Init(&hsd1);
  if (sd_state == MSD_OK)
  {
    if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK)
    {
      sd_state = MSD_ERROR;
    }
    else
    {
      (void) SD_enterHighSpeed();
    }
  }

  return sd_state;
}

void mmc1DeInit(void)
{
  if (shimmerStatus.sdPeripheralInit)
  {
    HAL_SD_DeInit(&hsd1);
    shimmerStatus.sdPeripheralInit = 0;
  }
}

void printSdCardInfo(char *outputStr)
{
  HAL_SD_CardCIDTypeDef pCID;
  HAL_SD_GetCardCID(&hsd1, &pCID);

  //TODO decide if we want to print CSD info too - currently just prints CID (which has the manufacturer date)
  //HAL_SD_CardCSDTypeDef pCSD;
  //HAL_SD_GetCardCSD(&hsd1, &pCSD);

  sprintf(outputStr, "Manufacturer: ");
  switch (pCID.ManufacturerID)
  {
  case 0x000001:
    sprintf(outputStr + strlen(outputStr), "Panasonic");
    break;
  case 0x000002:
    sprintf(outputStr + strlen(outputStr), "Toshiba");
    break;
  case 0x000003:
    sprintf(outputStr + strlen(outputStr), "SanDisk");
    break;
  case 0x00001b:
    sprintf(outputStr + strlen(outputStr), "Samsung");
    break;
  case 0x00001d:
    sprintf(outputStr + strlen(outputStr), "AData");
    break;
  case 0x000027:
    sprintf(outputStr + strlen(outputStr), "Phison");
    break;
  case 0x000028:
    sprintf(outputStr + strlen(outputStr), "Lexar");
    break;
  case 0x000031:
    sprintf(outputStr + strlen(outputStr), "Silicon Power");
    break;
  case 0x000041:
    sprintf(outputStr + strlen(outputStr), "Kingston");
    break;
  case 0x000074:
    sprintf(outputStr + strlen(outputStr), "Transcend");
    break;
  case 0x000076:
    sprintf(outputStr + strlen(outputStr), "Patriot");
    break;
  case 0x000082:
  case 0x00009c:
    sprintf(outputStr + strlen(outputStr), "Sony");
    break;
  default:
    break;
  }

  printSdCardSize(outputStr + strlen(outputStr));
  sprintf(outputStr + strlen(outputStr), ", Manufacture Date=%d-%02d",
      (pCID.ManufactDate >> 4) + 2000, pCID.ManufactDate & 0x0F);

  sprintf(outputStr + strlen(outputStr), "\r\n");
}

/**
 * Formats the SD card size as a string with a "GB" suffix (e.g., "7.45GB").
 * Writes the result to outputStr.
 */
void printSdCardSize(char *outputStr)
{
  sprintf(outputStr, "%.2lfGB",
      (hsd1.SdCard.BlockSize * ((float) hsd1.SdCard.BlockNbr / 1024 / 1024 / 1024)));
}

HAL_StatusTypeDef
HAL_SD_SharedWrite(sdOwner_t requester, uint8_t *pData, uint32_t addr, uint32_t blocks)
{
  uint32_t startTick = HAL_GetTick();
  uint32_t timeOut_ms = 1000;

  //0. No card: a failed init leaves hsd1.Instance NULL, which the HAL
  //would dereference
  if (hsd1.Instance == NULL)
  {
    return HAL_ERROR;
  }

  //1. Hardware/Mutex Busy Check
  //If someone else owns it, OR the HAL handle is busy, we wait.
  while (currentSdOwner != OWNER_IDLE || hsd1.State == HAL_SD_STATE_BUSY)
  {
    if ((HAL_GetTick() - startTick) > timeOut_ms)
    {
      return HAL_TIMEOUT; //Now it only timeouts if someone ELSE stays stuck
    }
  }

  //2. Atomic Lock
  //We "Claim" the card for this requester
  currentSdOwner = requester;
  sdTransferDone = 0;
  sdTransferError = 0;
  if (currentSdOwner == OWNER_FATFS)
  {
    WriteStatus = 0;
  }
  //3. Trigger the Hardware
  HAL_StatusTypeDef status = HAL_SD_WriteBlocks_DMA(&hsd1, pData, addr, blocks);

  if (status != HAL_OK)
  {
    currentSdOwner = OWNER_IDLE; //Release immediately if hardware failed to start
    return status;
  }

  //4. Wait for DMA Callback (The interrupt sets transfer_done = 1)
  while (sdTransferDone == 0)
  {
    if ((HAL_GetTick() - startTick) > timeOut_ms)
    {
      currentSdOwner = OWNER_IDLE;
      return HAL_TIMEOUT;
    }
  }
  //5. Surface an errored transfer: the error callback sets sdTransferDone
  //too, so the flag alone cannot be trusted as success
  if (sdTransferError)
  {
    currentSdOwner = OWNER_IDLE;
    return HAL_ERROR;
  }
  //6. Release for the next requester
  currentSdOwner = OWNER_IDLE;
  return HAL_OK;
}

HAL_StatusTypeDef
HAL_SD_SharedRead(sdOwner_t requester, uint8_t *pData, uint32_t addr, uint32_t blocks)
{
  uint32_t startTick = HAL_GetTick();
  uint32_t timeOut_ms = 1000;

  //0. No card: a failed init leaves hsd1.Instance NULL, which the HAL
  //would dereference
  if (hsd1.Instance == NULL)
  {
    return HAL_ERROR;
  }

  //1. Hardware/Mutex Busy Check
  //If someone else owns it, OR the HAL handle is busy, we wait.
  while (currentSdOwner != OWNER_IDLE || hsd1.State == HAL_SD_STATE_BUSY)
  {
    if ((HAL_GetTick() - startTick) > timeOut_ms)
    {
      return HAL_TIMEOUT; //Now it only timeouts if someone ELSE stays stuck
    }
  }

  //2. Atomic Lock
  //We "Claim" the card for this requester
  currentSdOwner = requester;
  sdTransferDone = 0;
  sdTransferError = 0;
  if (currentSdOwner == OWNER_FATFS)
  {
    ReadStatus = 0;
  }

  //3. Trigger the Hardware
  HAL_StatusTypeDef status = HAL_SD_ReadBlocks_DMA(&hsd1, pData, addr, blocks);

  if (status != HAL_OK)
  {
    currentSdOwner = OWNER_IDLE; //Release immediately if hardware failed to start
    return status;
  }

  //4. Wait for DMA Callback (The interrupt sets transfer_done = 1)
  while (sdTransferDone == 0)
  {
    if ((HAL_GetTick() - startTick) > timeOut_ms)
    {
      currentSdOwner = OWNER_IDLE;
      return HAL_TIMEOUT;
    }
  }
  //5. Surface an errored transfer: the error callback sets sdTransferDone
  //too, so the flag alone cannot be trusted as success
  if (sdTransferError)
  {
    currentSdOwner = OWNER_IDLE;
    return HAL_ERROR;
  }
  //6. Release for the next requester
  currentSdOwner = OWNER_IDLE;
  return HAL_OK;
}

/* CMD13 (the card-state query behind BSP_SD_GetCardState) is itself a command
 * on the shared SD bus: issued while another owner's DMA command is in flight
 * it can corrupt or hang that transfer. Readiness polls must therefore claim
 * the same mutex the read/write wrappers use, one status command per call.
 * Returns SD_TRANSFER_OK / SD_TRANSFER_BUSY; a mutex-wait timeout reports
 * SD_TRANSFER_BUSY and leaves retry policy to the caller's own timeout loop,
 * and so does a card that failed to initialise (hsd1.Instance NULL), without
 * touching the bus. */
uint8_t HAL_SD_SharedGetCardState(sdOwner_t requester)
{
  uint32_t startTick = HAL_GetTick();
  uint32_t timeOut_ms = 1000;
  uint8_t cardState;

  if (hsd1.Instance == NULL)
  {
    return SD_TRANSFER_BUSY;
  }

  while (currentSdOwner != OWNER_IDLE || hsd1.State == HAL_SD_STATE_BUSY)
  {
    if ((HAL_GetTick() - startTick) > timeOut_ms)
    {
      return SD_TRANSFER_BUSY;
    }
  }
  currentSdOwner = requester;
  cardState = BSP_SD_GetCardState();
  currentSdOwner = OWNER_IDLE;
  return cardState;
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd1)
{
  if (currentSdOwner == OWNER_USB)
  {
#ifdef USBX_MSC_DMA
    USDB_ReadCpltCallback(hsd1); //Signal the USB stack
#endif
  }
  else if (currentSdOwner == OWNER_FATFS)
  {
    BSP_SD_ReadCpltCallback(); //Signal the FatFs stack
  }
  sdTransferDone = 1;
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd1)
{
  if (currentSdOwner == OWNER_USB)
  {
#ifdef USBX_MSC_DMA
    USBD_WriteCpltCallback(hsd1);
#endif
  }
  else if (currentSdOwner == OWNER_FATFS)
  {
    BSP_SD_WriteCpltCallback(); //Signal the FatFs stack
  }
  sdTransferDone = 1;
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
  //Record the failure FIRST, then set the completion flags: waiters check
  //sdTransferError only after their flag releases them
  sdTransferError = 1;
  //Set ALL flags to 1 to ensure no thread stays stuck in a while loop
  SD_WRITE_FLAG = 1;
  SD_READ_FLAG = 1;
  WriteStatus = 1;
  ReadStatus = 1;
  sdTransferDone = 1;
  /* Deliberately NOT releasing currentSdOwner here: the requester releases it
   * on every exit path of HAL_SD_SharedRead/Write. Releasing from the ISR let
   * a concurrent claimant take the mutex while the failing requester was
   * still finishing, after which the requester's own release would clobber
   * the new owner's claim. */
}

/* USER CODE END 1 */
