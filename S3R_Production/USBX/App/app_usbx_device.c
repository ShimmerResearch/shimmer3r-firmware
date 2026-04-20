/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    app_usbx_device.c
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
#include "app_usbx_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <string.h>

#include "hal_Board.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN UX_Device_Memory_Buffer */

/* USER CODE END UX_Device_Memory_Buffer */
#if defined(__ICCARM__)
#pragma data_alignment = 32
#endif
/* The USBX byte pool backs all endpoint transfer buffers handed to the
 * SDMMC DMA via USBD_STORAGE_Read/Write. Align the pool itself to a full
 * 32-byte D-cache line (STM32U5 DCACHE line = 32 B) so that when we
 * Clean/Invalidate the rounded-up transfer range in the MSC callbacks we
 * cannot accidentally flush a cache line that also holds unrelated data
 * sitting just before/after the pool. The individual sub-allocations
 * inside the pool are still only 8-byte aligned (ThreadX byte pool), which
 * is why the MSC callbacks must also round the maintenance range down/up
 * to the cache line. */
__ALIGN_BEGIN static UCHAR ux_device_byte_pool_buffer[UX_DEVICE_APP_MEM_POOL_SIZE]
    __attribute__((aligned(32))) __ALIGN_END;

static ULONG storage_interface_number;
static ULONG storage_configuration_number;
static ULONG cdc_acm_interface_number;
static ULONG cdc_acm_configuration_number;
static UX_SLAVE_CLASS_STORAGE_PARAMETER storage_parameter;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_parameter;

/* USER CODE BEGIN PV */

static UCHAR vendor_id[] = "Shimmer";
static UCHAR product_id[] = "XXXX";
static UCHAR serial_id[20] = { ' ' };

static volatile bool usbx_initialized = false;

/** Tick value recorded when the USB device stack is started.  Suspend events
 *  received within SUSPEND_GRACE_MS of this timestamp are considered spurious
 *  (hosts – especially Mac USB-C/Thunderbolt ports – may issue multiple
 *  suspend events during HS chirp, PD probing or alternate-mode negotiation)
 *  and are ignored.  Only suspends after the grace window are treated as
 *  genuine cable-unplug events. */
static volatile uint32_t usbInitTick = 0;
#define SUSPEND_GRACE_MS 2000U

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static UINT USBD_ChangeFunction(ULONG Device_State);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
 * @brief  Application USBX Device Initialization.
 * @param  none
 * @retval status
 */

UINT MX_USBX_Device_Init(VOID)
{
  UINT ret = UX_SUCCESS;
  UCHAR *device_framework_high_speed;
  UCHAR *device_framework_full_speed;
  ULONG device_framework_hs_length;
  ULONG device_framework_fs_length;
  ULONG string_framework_length;
  ULONG language_id_framework_length;
  UCHAR *string_framework;
  UCHAR *language_id_framework;

  UCHAR *pointer;

  /* USER CODE BEGIN MX_USBX_Device_Init0 */

  const UCHAR *usb_serial;
  size_t usb_serial_len;

  /* Build the product ID for the Disk Drive string based on the Shimmer's MAC ID */
  LogAndStream_buildShimmerMacSuffix((char *) product_id, sizeof(product_id));

  usb_serial = USBD_Get_UsbSerialStringPtr();
  memset(serial_id, ' ', sizeof(serial_id));

  if (usb_serial != UX_NULL)
  {
    usb_serial_len = strlen((const char *) usb_serial);
    if (usb_serial_len > sizeof(serial_id))
    {
      usb_serial_len = sizeof(serial_id);
    }
    memcpy(serial_id, usb_serial, usb_serial_len);
  }

  /* USER CODE END MX_USBX_Device_Init0 */
  pointer = ux_device_byte_pool_buffer;

  /* Initialize USBX Memory.
   *
   * The second argument is the size of the memory region (pool) that USBX
   * manages from 'pointer'. It must match the size of
   * ux_device_byte_pool_buffer[] (UX_DEVICE_APP_MEM_POOL_SIZE), NOT the
   * unrelated USBX_DEVICE_MEMORY_STACK_SIZE (which is a leftover ThreadX
   * stack-size macro in the ST template). Passing the smaller value causes
   * ux_device_stack_initialize() to fail with UX_MEMORY_INSUFFICIENT as
   * soon as UX_SLAVE_REQUEST_DATA_MAX_LENGTH is raised above the default. */
  if (ux_system_initialize(pointer, UX_DEVICE_APP_MEM_POOL_SIZE, UX_NULL, 0) != UX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_SYSTEM_INITIALIZE_ERROR */
    return UX_ERROR;
    /* USER CODE END USBX_SYSTEM_INITIALIZE_ERROR */
  }

  /* Get Device Framework High Speed and get the length */
  device_framework_high_speed
      = USBD_Get_Device_Framework_Speed(USBD_HIGH_SPEED, &device_framework_hs_length);

  /* Get Device Framework Full Speed and get the length */
  device_framework_full_speed
      = USBD_Get_Device_Framework_Speed(USBD_FULL_SPEED, &device_framework_fs_length);

  /* Get String Framework and get the length */
  string_framework = USBD_Get_String_Framework(&string_framework_length);

  /* Get Language Id Framework and get the length */
  language_id_framework = USBD_Get_Language_Id_Framework(&language_id_framework_length);

  /* Install the device portion of USBX */
  if (ux_device_stack_initialize(device_framework_high_speed,
          device_framework_hs_length, device_framework_full_speed,
          device_framework_fs_length, string_framework, string_framework_length,
          language_id_framework, language_id_framework_length, USBD_ChangeFunction)
      != UX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_DEVICE_INITIALIZE_ERROR */
    return UX_ERROR;
    /* USER CODE END USBX_DEVICE_INITIALIZE_ERROR */
  }

  /* Initialize the storage class parameters for the device */
  storage_parameter.ux_slave_class_storage_instance_activate = USBD_STORAGE_Activate;
  storage_parameter.ux_slave_class_storage_instance_deactivate = USBD_STORAGE_Deactivate;

  /* Store the number of LUN in this device storage instance */
  storage_parameter.ux_slave_class_storage_parameter_number_lun = STORAGE_NUMBER_LUN;

  /* Initialize the storage class parameters for reading/writing to the Flash Disk */
  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba
      = USBD_STORAGE_GetMediaLastLba();

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length
      = USBD_STORAGE_GetMediaBlocklength();

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type
      = 0;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag
      = STORAGE_REMOVABLE_FLAG;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read_only_flag
      = STORAGE_READ_ONLY;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read
      = USBD_STORAGE_Read;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write
      = USBD_STORAGE_Write;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_flush
      = USBD_STORAGE_Flush;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status
      = USBD_STORAGE_Status;

  storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_notification
      = USBD_STORAGE_Notification;

  /* USER CODE BEGIN STORAGE_PARAMETER */

  storage_parameter.ux_slave_class_storage_parameter_vendor_id = vendor_id;
  storage_parameter.ux_slave_class_storage_parameter_product_id = product_id;
  //storage_parameter.ux_slave_class_storage_parameter_product_rev = (UCHAR *)STORAGE_PRODUCT_REV;
  storage_parameter.ux_slave_class_storage_parameter_product_serial = serial_id;

  /* USER CODE END STORAGE_PARAMETER */

  /* Get storage configuration number */
  storage_configuration_number = USBD_Get_Configuration_Number(CLASS_TYPE_MSC, 0);

  /* Find storage interface number */
  storage_interface_number = USBD_Get_Interface_Number(CLASS_TYPE_MSC, 0);

  /* Initialize the device storage class */
  if (ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
          storage_configuration_number, storage_interface_number, &storage_parameter)
      != UX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_DEVICE_STORAGE_REGISTER_ERROR */
    return UX_ERROR;
    /* USER CODE END USBX_DEVICE_STORAGE_REGISTER_ERROR */
  }

  /* Initialize the cdc acm class parameters for the device */
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_activate = USBD_CDC_ACM_Activate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_instance_deactivate = USBD_CDC_ACM_Deactivate;
  cdc_acm_parameter.ux_slave_class_cdc_acm_parameter_change = USBD_CDC_ACM_ParameterChange;

  /* USER CODE BEGIN CDC_ACM_PARAMETER */

  /* USER CODE END CDC_ACM_PARAMETER */

  /* Get cdc acm configuration number */
  cdc_acm_configuration_number = USBD_Get_Configuration_Number(CLASS_TYPE_CDC_ACM, 0);

  /* Find cdc acm interface number */
  cdc_acm_interface_number = USBD_Get_Interface_Number(CLASS_TYPE_CDC_ACM, 0);

  /* Initialize the device cdc acm class */
  if (ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
          cdc_acm_configuration_number, cdc_acm_interface_number, &cdc_acm_parameter)
      != UX_SUCCESS)
  {
    /* USER CODE BEGIN USBX_DEVICE_CDC_ACM_REGISTER_ERROR */
    return UX_ERROR;
    /* USER CODE END USBX_DEVICE_CDC_ACM_REGISTER_ERROR */
  }

  /* USER CODE BEGIN MX_USBX_Device_Init1 */
  USBX_APP_Device_Init();

  usbx_initialized = true;
  usbInitTick = HAL_GetTick();
  /* USER CODE END MX_USBX_Device_Init1 */

  return ret;
}

/**
 * @brief  _ux_utility_interrupt_disable
 *         USB utility interrupt disable.
 * @param  none
 * @retval none
 */
ALIGN_TYPE _ux_utility_interrupt_disable(VOID)
{
  UINT interrupt_save;
  /* USER CODE BEGIN _ux_utility_interrupt_disable */
  interrupt_save = __get_PRIMASK();
  __disable_irq();
  /* USER CODE END _ux_utility_interrupt_disable */

  return interrupt_save;
}

/**
 * @brief  _ux_utility_interrupt_restore
 *         USB utility interrupt restore.
 * @param  flags
 * @retval none
 */
VOID _ux_utility_interrupt_restore(ALIGN_TYPE flags)
{

  /* USER CODE BEGIN _ux_utility_interrupt_restore */
  __set_PRIMASK(flags);
  /* USER CODE END _ux_utility_interrupt_restore */
}

/**
 * @brief  _ux_utility_time_get
 *         Get Time Tick for host timing.
 * @param  none
 * @retval time tick
 */
ULONG _ux_utility_time_get(VOID)
{
  ULONG time_tick = 0U;

  /* USER CODE BEGIN _ux_utility_time_get */

  time_tick = (ULONG) HAL_GetTick();

  /* USER CODE END _ux_utility_time_get */

  return time_tick;
}

/**
 * @brief  USBD_ChangeFunction
 *         This function is called when the device state changes.
 * @param  Device_State: USB Device State
 * @retval status
 */
static UINT USBD_ChangeFunction(ULONG Device_State)
{
  UINT status = UX_SUCCESS;

  /* USER CODE BEGIN USBD_ChangeFunction0 */

  /* USER CODE END USBD_ChangeFunction0 */

  switch (Device_State)
  {
  case UX_DEVICE_ATTACHED:

    /* USER CODE BEGIN UX_DEVICE_ATTACHED */

    /* USER CODE END UX_DEVICE_ATTACHED */

    break;

  case UX_DEVICE_REMOVED:

    /* USER CODE BEGIN UX_DEVICE_REMOVED */

    /* USER CODE END UX_DEVICE_REMOVED */

    break;

  case UX_DCD_STM32_DEVICE_CONNECTED:

    /* USER CODE BEGIN UX_DCD_STM32_DEVICE_CONNECTED */

    /* USER CODE END UX_DCD_STM32_DEVICE_CONNECTED */

    break;

  case UX_DCD_STM32_DEVICE_DISCONNECTED:

    /* USER CODE BEGIN UX_DCD_STM32_DEVICE_DISCONNECTED */

    /* USER CODE END UX_DCD_STM32_DEVICE_DISCONNECTED */

    break;

  case UX_DCD_STM32_DEVICE_SUSPENDED:

    /* USER CODE BEGIN UX_DCD_STM32_DEVICE_SUSPENDED */

    /* Hosts (especially Mac USB-C / Thunderbolt ports) may issue multiple
     * spurious suspend events during enumeration, HS chirp, or USB-PD / alt-
     * mode probing.  Ignore all suspend events that arrive within the grace
     * window after USB init.
     *
     * After the grace window, treat suspend as a cable-unplug indicator
     * (because VBUS sensing inside the OTG peripheral is not usable on this
     * hardware — see SCH voltage-divider note). As an extra safety check,
     * confirm that the VBUS GPIO has actually gone low before triggering
     * the dock/USB state change; a genuine host-initiated selective suspend
     * will still have VBUS high and should be ignored here. */
    if ((HAL_GetTick() - usbInitTick) >= SUSPEND_GRACE_MS)
    {
      if (!Board_isUsbPluggedIn())
      {
        ShimTask_setDockOrUsbStateChange();
      }
    }

    /* USER CODE END UX_DCD_STM32_DEVICE_SUSPENDED */

    break;

  case UX_DCD_STM32_DEVICE_RESUMED:

    /* USER CODE BEGIN UX_DCD_STM32_DEVICE_RESUMED */

    /* USER CODE END UX_DCD_STM32_DEVICE_RESUMED */

    break;

  case UX_DCD_STM32_SOF_RECEIVED:

    /* USER CODE BEGIN UX_DCD_STM32_SOF_RECEIVED */

    /* USER CODE END UX_DCD_STM32_SOF_RECEIVED */

    break;

  default:

    /* USER CODE BEGIN DEFAULT */

    /* USER CODE END DEFAULT */

    break;
  }

  /* USER CODE BEGIN USBD_ChangeFunction1 */

  /* USER CODE END USBD_ChangeFunction1 */

  return status;
}

/* USER CODE BEGIN 1 */
VOID USBX_Device_Process(VOID)
{
  ux_device_stack_tasks_run();
}

VOID USBX_APP_Device_Init(VOID)
{
  /* USER CODE BEGIN USB_Device_Init_PreTreatment_0 */

  /* USER CODE END USB_Device_Init_PreTreatment_0 */

  /* initialize the device controller HAL driver */
  MX_USB_OTG_HS_PCD_Init();

  //1. Set Rx FIFO.  Must be at least big enough to hold the largest packet size
  //(512 words for HS) plus some overhead for control transfers and status information.
  //The ST example sets this to 512 words, which is sufficient for HS with 2 bulk OUT EPs, but may need to be increased if more/larger OUT EPs are added.
  // 512 words RX is the ST example default for HS with 2 bulk OUT EPs.
  HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x200); //512 (was 0x100)

  //2. Tx FIFO 0: Control Endpoint (Common)
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, 0x20); //32 words (0x20 = 128 bytes) – 4× EP0 MPS

  //3. Tx FIFO 1: MSC Data IN (Matches USBD_MSC_EPIN_ADDR 0x81)
  /* MSC-biased: 256 words = 2x HS bulk MPS so the core can queue the next IN
   * packet while the previous one is still on the wire, maximising MSC
   * throughput. CDC Data IN (TX4) is sized down accordingly. */
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, 0x100); //256 MSC Data IN (2x HS MPS)

  /* 4. Set FIFO 2 */
  /* Since you use FIFO 3 and 4, FIFO 2 MUST be at least 16 words */
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, 0x10); //16 placeholder (unused IN EP2)

  //4. Tx FIFO 3: CDC Command IN (Matches USBD_CDCACM_EPINCMD_ADDR 0x83)
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 3, 0x20); //32 CDC CMD IN

  //5. Tx FIFO 4: CDC Data IN (Matches USBD_CDCACM_EPIN_ADDR 0x84)
  /* CDC is low-throughput (commands/telemetry); 128 words = ~2x HS MPS is
   * plenty and leaves FIFO budget for the MSC-biased TX1 above. */
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 4, 0x80); //128 CDC Data IN

  /* USER CODE END USB_Device_Init_PreTreatment_1 */

  /* initialize and link controller HAL driver to USBx */
  if (_ux_dcd_stm32_initialize((ULONG) USB_OTG_HS, (ULONG) &hpcd_USB_OTG_HS) != UX_SUCCESS)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN USB_Device_Init_PostTreatment */
  HAL_PCD_Start(&hpcd_USB_OTG_HS);

  /* USER CODE END USB_Device_Init_PostTreatment */
}

VOID USBX_APP_Device_DeInit(VOID)
{
  HAL_StatusTypeDef hal_status;
  UINT usbx_status;

  /* Stop USB peripheral */
  hal_status = HAL_PCD_Stop(&hpcd_USB_OTG_HS);
  if (hal_status != HAL_OK)
  {
    /* In deinit/unplug path, failures should not cause a fatal reset. */
    return;
  }

  /* Deinitialize USB peripheral */
  hal_status = HAL_PCD_DeInit(&hpcd_USB_OTG_HS);
  if (hal_status != HAL_OK)
  {
    /* In deinit/unplug path, failures should not cause a fatal reset. */
    return;
  }

  /* Uninitialize USBX DCD driver */
  usbx_status = _ux_dcd_stm32_uninitialize((ULONG) USB_OTG_HS, (ULONG) &hpcd_USB_OTG_HS);
  if (usbx_status != UX_SUCCESS)
  {
    /* In deinit/unplug path, failures should not cause a fatal reset. */
    return;
  }
}

UINT MX_USBX_Device_DeInit(VOID)
{
  UINT ret = UX_SUCCESS;

  /* 0. Stop USBX device hardware controller (HAL) */
  /* Pull the plug on the hardware last */
  USBX_APP_Device_DeInit();

  /* 1. Unregister CDC ACM class */
  /* Do this first so the stack knows these interfaces are no longer active */
  ux_device_stack_class_unregister(
      _ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

  /* 2. Unregister MSC class */
  ux_device_stack_class_unregister(
      _ux_system_slave_class_storage_name, ux_device_class_storage_entry);

  /* 3. Uninitialize USBX device stack */
  ux_device_stack_uninitialize();

  /* 4. Uninitialize USBX system */
  /* This cleans up the memory pool and internal system resources */
  ux_system_uninitialize();

  usbx_initialized = false;

  return ret;
}

bool USBX_IsInitialised(void)
{
  return usbx_initialized;
}

/* USER CODE END 1 */
