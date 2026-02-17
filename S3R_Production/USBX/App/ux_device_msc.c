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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BLOCK_START_ADDR         0     /* Block start address      */
#define NUM_OF_BLOCKS            5     /* Total number of blocks   */
#define BUFFER_WORDS_SIZE        ((MMC_BLOCKSIZE * NUM_OF_BLOCKS) >> 2) /* Total data size in bytes */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile uint8_t SD_READ_FLAG = 0;
volatile uint8_t SD_WRITE_FLAG = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#include "hal_board.h"

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
 /* UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(data_pointer);
  UX_PARAMETER_NOT_USED(number_blocks);
  UX_PARAMETER_NOT_USED(lba);
  UX_PARAMETER_NOT_USED(media_status);*/
  /* USER CODE BEGIN USBD_STORAGE_Read */
    uint32_t timeout_ms = 2000;
    uint32_t start_tick = 0;
    uint32_t total_length = number_blocks * 512; // SD_BLOCKSIZE is 512

  #ifdef DMA
    SD_READ_FLAG = 0;
    /* Start the Dma write */
    if (HAL_SD_ReadBlocks_DMA(&hsd1, (uint8_t *) data_pointer, lba, number_blocks) != HAL_OK)
    {
      return UX_ERROR;
    }
    //Wait until DMA transfer complete
    start_tick = HAL_GetTick();
    while (SD_READ_FLAG == 0)
    {
      if ((HAL_GetTick() - start_tick) > timeout_ms)
      {
        return UX_ERROR; //DMA timeout
      }

    }
  #else
    if (HAL_SD_ReadBlocks(&hsd1, (uint8_t *) data_pointer, lba, number_blocks, timeout_ms) != HAL_OK)
    {
      return UX_ERROR;
    }
  #endif
    uint32_t alignedAddr = ((uint32_t)data_pointer) & ~0x1F;
     SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, total_length + ((uint32_t)data_pointer - alignedAddr));
     while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER)
         {
           if ((HAL_GetTick() - start_tick) > timeout_ms) return UX_ERROR;
         }
    status = UX_STATE_NEXT;
    return status;
  /* USER CODE END USBD_STORAGE_Read */
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
  uint32_t timeout_ms = 2000; // Set an appropriate timeout value
  uint32_t start_tick = 0;
  uint32_t total_length = number_blocks * 512;

 /* uint32_t alignedAddr = (uint32_t)data_pointer & ~0x1F;
  uint32_t full_size = (((uint32_t)data_pointer + total_length + 31U) & ~0x1F) - alignedAddr;
     Round up length to 32-byte cache lines
        SCB_CleanDCache_by_Addr((uint32_t *)alignedAddr,
            full_size);
*/
  /* In USBD_STORAGE_Write, before HAL_SD_WriteBlocks_DMA */
  uint32_t alignedAddr = (uint32_t)data_pointer & ~0x1F;
  uint32_t full_size = (((uint32_t)data_pointer + total_length + 31U) & ~0x1F) - alignedAddr;
  SCB_CleanDCache_by_Addr((uint32_t *)alignedAddr, full_size);

#ifdef DMA
  /* Start the Dma write */
  SD_WRITE_FLAG = 0;
  if (HAL_SD_WriteBlocks_DMA(&hsd1, (uint8_t *) data_pointer, lba, number_blocks) != HAL_OK)
  {
    return UX_ERROR;
  }
  //Wait until DMA transfer complete
  start_tick = HAL_GetTick();
  while (SD_WRITE_FLAG == 0)
  {
    if ((HAL_GetTick() - start_tick) > timeout_ms)
    {
      return UX_ERROR; //DMA timeout
    }
  }
#else
  if (HAL_SD_WriteBlocks(&hsd1, (uint8_t *) data_pointer, lba, number_blocks, timeout_ms) != HAL_OK)
  {
    return UX_ERROR;
  }
#endif
  start_tick = HAL_GetTick();
  while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER)
  {
    if ((HAL_GetTick() - start_tick) > timeout_ms) return UX_ERROR;
  }
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

  /* USER CODE BEGIN USBD_STORAGE_Flush
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(number_blocks);
  UX_PARAMETER_NOT_USED(lba);
  UX_PARAMETER_NOT_USED(media_status);
   USER CODE END USBD_STORAGE_Flush */

  if (HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER)
  {
    status = UX_STATE_NEXT;
  }
  else
  {
    status = UX_ERROR;
  }
  /* USER CODE END USBD_STORAGE_Flush */
  status = UX_STATE_NEXT;
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

  /* USER CODE BEGIN USBD_STORAGE_Status
  UX_PARAMETER_NOT_USED(storage_instance);
  UX_PARAMETER_NOT_USED(lun);
  UX_PARAMETER_NOT_USED(media_id);
  UX_PARAMETER_NOT_USED(media_status);*/

  UINT status = UX_SUCCESS;
  HAL_SD_CardCIDTypeDef pCID;
  HAL_SD_CardCSDTypeDef pCSD;

   if (HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER)
     {
       status = UX_SUCCESS;
     }
     else
     {
       status = UX_ERROR;
     }
   HAL_SD_GetCardCID(&hsd1, &pCID);
   HAL_SD_GetCardCSD(&hsd1, &pCSD);
   return status;
   /* USER CODE END USBD_STORAGE_Status */
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
  if ((!shimmerStatus.sdPeripheralInit))
  {
    Board_sd2Mcu();
  }
  HAL_SD_CardInfoTypeDef CardInfo = { 0 };
  HAL_SD_GetCardInfo(&hsd1, &CardInfo);
  LastLba = CardInfo.BlockNbr-1;
  return LastLba;
  /* USER CODE END USBD_STORAGE_GetMediaLastLba */
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
  /* USER CODE BEGIN USBD_STORAGE_GetMediaBlocklength */
  if ((!shimmerStatus.sdPeripheralInit))
  {
    Board_sd2Mcu();
  }

  HAL_SD_CardInfoTypeDef CardInfo = { 0 };
  HAL_SD_GetCardInfo(&hsd1, &CardInfo);
  MediaBlockLen = CardInfo.BlockSize;
  /* USER CODE END USBD_STORAGE_GetMediaBlocklength */

  return MediaBlockLen;
}

/* USER CODE BEGIN 1 */
#ifdef DMA
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
