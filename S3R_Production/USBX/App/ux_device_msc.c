/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    ux_device_msc.c
 * @author  MCD Application Team
 * @brief   USBX Device applicative file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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
#include "ux_device_msc.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dcache.h"
#include "hal_Board.h"
#include "sdmmc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BLOCK_START_ADDR  0 /* Block start address      */
#define NUM_OF_BLOCKS     5 /* Total number of blocks   */
#define BUFFER_WORDS_SIZE   /* USER CODE END PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile uint8_t SD_READ_FLAG = 0;
volatile uint8_t SD_WRITE_FLAG = 0;
extern DCACHE_HandleTypeDef hdcache1;

/* Set true at the end of a write DMA, cleared once the card has been
 * observed to return to the TRANSFER state. While this is true the card is
 * (or may still be) in its internal PROG phase committing data to NAND and
 * cannot accept a new read/write command. By deferring the PROG-state
 * poll from the end of the previous write to the start of the NEXT
 * read/write/flush, the PROG time overlaps with the USB bulk-OUT phase
 * of the next chunk (host streaming the next 32/64 KB into our buffer
 * while the card still programs the previous chunk), instead of being
 * pure dead time. On cheap class-10 cards this is a ~2-3x MSC write
 * throughput improvement. */
static volatile bool sd_card_may_be_programming = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* Wait (with timeout) for the card to return to the TRANSFER state, but
 * only if the previous operation might have left it programming. Returns
 * UX_SUCCESS if the card is ready (or was never busy), UX_ERROR on timeout.
 * Clears the may-be-programming flag on success. Called at the start of
 * Read / Write / Flush / Status so the previous write's PROG phase
 * overlaps with the current transfer's setup. */
static UINT USBD_STORAGE_WaitCardReady(uint32_t timeout_ms)
{
  if (!sd_card_may_be_programming)
  {
    return UX_SUCCESS;
  }

  uint32_t start_tick = HAL_GetTick();
  while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER)
  {
    if ((HAL_GetTick() - start_tick) > timeout_ms)
    {
      return UX_ERROR;
    }
  }
  sd_card_may_be_programming = false;
  return UX_SUCCESS;
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  USBD_STORAGE_Activate
 *         This function is called when insertion of a storage device.
 * @param  storage_instance: Pointer to the storage class instance.
 * @retval none
 */
VOID USBD_STORAGE_Activate(VOID *storage_instance)
{
  /* USER CODE BEGIN USBD_STORAGE_Activate */
  UX_PARAMETER_NOT_USED(storage_instance);
  /* USER CODE END USBD_STORAGE_Activate */

  return;
}

/**
 * @brief  USBD_STORAGE_Deactivate
 *         This function is called when extraction of a storage device.
 * @param  storage_instance: Pointer to the storage class instance.
 * @retval none
 */
VOID USBD_STORAGE_Deactivate(VOID *storage_instance)
{
  /* USER CODE BEGIN USBD_STORAGE_Deactivate  */
  UX_PARAMETER_NOT_USED(storage_instance);
  /* USER CODE END USBD_STORAGE_Deactivate */

  return;
}

/**
 * @brief  USBD_STORAGE_Read
 *         This function is invoked to read from media.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  data_pointer: Address of the buffer to be used for reading or writing.
 * @param  number_blocks: number of sectors to read/write.
 * @param  lba: Logical block address is the sector address to read.
 * @param  media_status: should be filled out exactly like the media status
 *                       callback return value.
 * @retval status
 */
UINT USBD_STORAGE_Read(VOID *storage_instance,
    ULONG lun,
    UCHAR *data_pointer,
    ULONG number_blocks,
    ULONG lba,
    ULONG *media_status)
{
  UINT status = UX_SUCCESS;
  /* USER CODE BEGIN USBD_STORAGE_Read */
  uint32_t timeout_ms = 2000;
  uint32_t total_length = number_blocks * 512; //SD_BLOCKSIZE is 512

  if (hsd1.Instance == NULL)
  {
    return UX_ERROR;
  }

  /* If a previous write left the card in PROG, wait for it to finish now.
   * This overlaps the card's internal NAND programming with the USB OUT
   * phase that just delivered this read request (host had to wait for the
   * previous write's CSW anyway), instead of burning it at the end of the
   * previous write callback. */
  if (USBD_STORAGE_WaitCardReady(timeout_ms) != UX_SUCCESS)
  {
    return UX_ERROR;
  }

#ifdef USBX_MSC_DMA
  /* Kick the SDMMC DMA. HAL_SD_SharedRead internally waits for the DMA
   * completion ISR (sdTransferDone) before returning, so we do NOT need
   * a second wait on SD_READ_FLAG here — that was pure duplicated spin. */
  if (HAL_SD_SharedRead(OWNER_USB, (uint8_t *) data_pointer, lba, number_blocks) != HAL_OK)
  {
    return UX_ERROR;
  }
#else
  if (HAL_SD_ReadBlocks(&hsd1, (uint8_t *) data_pointer, lba, number_blocks, timeout_ms) != HAL_OK)
  {
    return UX_ERROR;
  }
#endif

  /* D-cache invalidate for the region the SDMMC DMA just wrote into.
   * Round the start down AND the end up to the 32-byte cache-line boundary,
   * otherwise any partial cache line at either end can hold stale data that
   * masks the DMA-fresh bytes the host is about to read back.
   * (This mirrors the Clean range used in USBD_STORAGE_Write.) */
  uint32_t alignedAddr = ((uint32_t) data_pointer) & ~0x1FU;
  uint32_t full_size = (((uint32_t) data_pointer + total_length + 31U) & ~0x1FU) - alignedAddr;
  HAL_DCACHE_InvalidateByAddr(&hdcache1, (uint32_t *) alignedAddr, full_size);

  /* Reads do NOT put the card into PROG state, so no post-transfer
   * TRANSFER-state poll is needed here. */

  status = UX_STATE_NEXT;
  /* USER CODE END USBD_STORAGE_Read */
  return status;
}

/**
 * @brief  USBD_STORAGE_Write
 *         This function is invoked to write in media.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  data_pointer: Address of the buffer to be used for reading or writing.
 * @param  number_blocks: number of sectors to read/write.
 * @param  lba: Logical block address is the sector address to read.
 * @param  media_status: should be filled out exactly like the media status
 *                       callback return value.
 * @retval status
 */
UINT USBD_STORAGE_Write(VOID *storage_instance,
    ULONG lun,
    UCHAR *data_pointer,
    ULONG number_blocks,
    ULONG lba,
    ULONG *media_status)
{
  /* USER CODE BEGIN USBD_STORAGE_Write */
  UINT status = UX_SUCCESS;
  uint32_t timeout_ms = 2000; //Set an appropriate timeout value
  uint32_t total_length = number_blocks * 512;

  if (hsd1.Instance == NULL)
  {
    return UX_ERROR;
  }

  /* If the previous write left the card in PROG, wait for it to finish now.
   * The host has already streamed this chunk's data into our buffer over
   * USB while the card was programming, so most (often all) of that PROG
   * time has already elapsed -> this wait is usually short or zero. */
  if (USBD_STORAGE_WaitCardReady(timeout_ms) != UX_SUCCESS)
  {
    return UX_ERROR;
  }

  /* D-cache maintenance for the host-filled buffer before the SDMMC DMA
   * reads from it.
   *
   * In OTG_HS CPU-FIFO (slave) mode — the current configuration; see the
   * comment in MX_USB_OTG_HS_PCD_Init — the CPU copies the host bulk-OUT
   * packets out of the Synopsys FIFO and into this buffer via normal
   * store instructions, which go through the D-cache.  The SDMMC DMA
   * that is about to read this buffer bypasses the cache, so we MUST
   * Clean the cache lines covering the buffer first, otherwise the
   * SDMMC DMA would read stale pre-USB bytes from SRAM.
   *
   * (If we ever re-enable OTG DMA mode — hpcd_USB_OTG_HS.Init.dma_enable
   * = ENABLE — this must flip to Invalidate, because the USB DMA would
   * then write directly to SRAM bypassing cache and Clean would
   * overwrite the USB-fresh bytes with stale cached data.)
   *
   * Round the range to whole 32-byte cache lines at BOTH ends so no
   * dirty partial line at either boundary is left un-flushed. */
  uint32_t alignedAddr = (uint32_t) data_pointer & ~0x1F;
  uint32_t full_size = (((uint32_t) data_pointer + total_length + 31U) & ~0x1F) - alignedAddr;
  HAL_DCACHE_CleanByAddr(&hdcache1, (uint32_t *) alignedAddr, full_size);

#ifdef USBX_MSC_DMA
  /* Kick the SDMMC DMA. HAL_SD_SharedWrite internally waits for the DMA
   * completion ISR (sdTransferDone) before returning, so we do NOT need
   * a second wait on SD_WRITE_FLAG here. */
  if (HAL_SD_SharedWrite(OWNER_USB, (uint8_t *) data_pointer, lba, number_blocks) != HAL_OK)
  {
    return UX_ERROR;
  }
#else
  if (HAL_SD_WriteBlocks(&hsd1, (uint8_t *) data_pointer, lba, number_blocks, timeout_ms) != HAL_OK)
  {
    return UX_ERROR;
  }
#endif

  /* DMA complete means the card has received the data; the card will now
   * spend tens of ms in PROG committing it to NAND. We DO NOT wait for
   * that to finish here — returning UX_STATE_NEXT immediately lets USBX
   * send CSW to the host so the next bulk-OUT can start flowing in
   * parallel with the PROG phase. The next Read/Write/Flush/Status will
   * wait for TRANSFER state via USBD_STORAGE_WaitCardReady before issuing
   * a new command. */
  sd_card_may_be_programming = true;

  status = UX_STATE_NEXT;
  /* USER CODE END USBD_STORAGE_Write */

  return status;
}

/**
 * @brief  USBD_STORAGE_Flush
 *         This function is invoked to flush media.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  number_blocks: number of sectors to read/write.
 * @param  lba: Logical block address is the sector address to read.
 * @param  media_status: should be filled out exactly like the media status
 *                       callback return value.
 * @retval status
 */
UINT USBD_STORAGE_Flush(VOID *storage_instance, ULONG lun, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_STORAGE_Flush */
  if (hsd1.Instance == NULL)
  {
    return UX_ERROR;
  }

  /* Flush MUST ensure previously written data is durable. If there is a
   * pending PROG phase from the last write, wait for it to complete here
   * before reporting success to the host. */
  if (USBD_STORAGE_WaitCardReady(2000) != UX_SUCCESS)
  {
    return UX_ERROR;
  }

  status = UX_STATE_NEXT;
  /* USER CODE END USBD_STORAGE_Flush */
  return status;
}

/**
 * @brief  USBD_STORAGE_Status
 *         This function is invoked to obtain the status of the device.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  media_id: is not currently used.
 * @param  media_status: should be filled out exactly like the media status
 *                       callback return value.
 * @retval status
 */
UINT USBD_STORAGE_Status(VOID *storage_instance, ULONG lun, ULONG media_id, ULONG *media_status)
{
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_STORAGE_Status */
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(media_id);
  UX_PARAMETER_NOT_USED(media_status);

  if (hsd1.Instance == NULL)
  {
    return UX_ERROR;
  }

  /* SCSI Test Unit Ready / equivalent poll from the host.  This is called
   * from the MSC class thread at the head of every CBW that isn't a
   * read/write.  It MUST be fast and MUST NOT issue any SD-bus traffic
   * of its own, for two reasons:
   *
   *  1. On a shared-SD design (see HAL_SD_SharedRead/Write in sdmmc.c)
   *     issuing CMD13 via HAL_SD_GetCardState() here while another
   *     owner (FatFs / sensing task) has an SDMMC DMA transfer in
   *     flight will collide with that transfer, corrupt status, or
   *     hang this callback waiting on a busy peripheral.  That hang
   *     directly stalls the MSC bulk-OUT endpoint because the USBX
   *     task cannot re-arm the endpoint until we return.  On direct
   *     USB-C->USB-C HS links the Mac xHCI host gives up after ~30 s
   *     of NAKs and triggers a BOT reset (endpoint 0x02 timeout,
   *     fConsecutiveResetCount++).
   *
   *  2. Semantically TUR only needs to report "medium present / not
   *     present / becoming ready".  If the card is actually bad, the
   *     next Read/Write callback will fail and the CSW will carry a
   *     FAILED status anyway — which is the correct SCSI way to
   *     surface a transient error.
   *
   * So report READY unconditionally whenever the peripheral is
   * initialised.  The previous implementation's use of
   * HAL_SD_GetCardState + a may_be_programming hint is moved out; the
   * real PROG-state wait already happens at the head of Read/Write/
   * Flush via USBD_STORAGE_WaitCardReady, which is where it belongs. */
  status = UX_SUCCESS;

  /* USER CODE END USBD_STORAGE_Status */

  return status;
}

/**
 * @brief  USBD_STORAGE_Notification
 *         This function is invoked to obtain the notification of the device.
 * @param  storage_instance : Pointer to the storage class instance.
 * @param  lun: Logical unit number is the command is directed to.
 * @param  media_id: is not currently used.
 * @param  notification_class: specifies the class of notification.
 * @param  media_notification: response for the notification.
 * @param  media_notification_length: length of the response buffer.
 * @retval status
 */
UINT USBD_STORAGE_Notification(VOID *storage_instance,
    ULONG lun,
    ULONG media_id,
    ULONG notification_class,
    UCHAR **media_notification,
    ULONG *media_notification_length)
{
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_STORAGE_Notification */
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(media_id);
  UX_PARAMETER_NOT_USED(notification_class);
  UX_PARAMETER_NOT_USED(media_notification);
  UX_PARAMETER_NOT_USED(media_notification_length);
  /* USER CODE END USBD_STORAGE_Notification */

  return status;
}

/**
 * @brief  USBD_STORAGE_GetMediaLastLba
 *         Get Media last LBA.
 * @param  none
 * @retval last lba
 */
ULONG USBD_STORAGE_GetMediaLastLba(VOID)
{
  ULONG LastLba = 0U;

  /* USER CODE BEGIN USBD_STORAGE_GetMediaLastLba */
  if (hsd1.Instance == NULL)
  {
    /* Return a non-zero default so the MSC stack sees valid geometry during
       class setup. Actual media-not-ready is reported by the Status, Read
       and Write callbacks. */
    return 1U;
  }

  HAL_SD_CardInfoTypeDef CardInfo = { 0 };
  HAL_SD_GetCardInfo(&hsd1, &CardInfo);
  LastLba = CardInfo.BlockNbr - 1;
  /* USER CODE END USBD_STORAGE_GetMediaLastLba */

  return LastLba;
}

/**
 * @brief  USBD_STORAGE_GetMediaBlocklength
 *         Get Media block length.
 * @param  none.
 * @retval block length.
 */
ULONG USBD_STORAGE_GetMediaBlocklength(VOID)
{
  ULONG MediaBlockLen = 0U;

  /* USER CODE BEGIN USBD_STORAGE_GetMediaBlocklength */
  if (hsd1.Instance == NULL)
  {
    /* Return a default block size so the MSC stack sees valid geometry during
       class setup. Actual media-not-ready is reported by the Status, Read
       and Write callbacks. */
    return 512U;
  }

  HAL_SD_CardInfoTypeDef CardInfo = { 0 };
  HAL_SD_GetCardInfo(&hsd1, &CardInfo);
  MediaBlockLen = CardInfo.BlockSize;
  /* USER CODE END USBD_STORAGE_GetMediaBlocklength */

  return MediaBlockLen;
}

/* USER CODE BEGIN 1 */
#ifdef USBX_MSC_DMA
void USBD_WriteCpltCallback(SD_HandleTypeDef *hsd1)
{
  if (hsd1->Instance == SDMMC1)
  {
    SD_WRITE_FLAG = 1;
  }
}

void USDB_ReadCpltCallback(SD_HandleTypeDef *hsd1)
{
  if (hsd1->Instance == SDMMC1)
  {
    SD_READ_FLAG = 1;
  }
}
#endif

/* USER CODE END 1 */
