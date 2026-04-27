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
/* The USBX byte pool backs all endpoint transfer buffers handed to the
 * SDMMC DMA via USBD_STORAGE_Read/Write. Align the pool itself to a full
 * 32-byte D-cache line (STM32U5 DCACHE line = 32 B) so that when we
 * Clean/Invalidate the rounded-up transfer range in the MSC callbacks we
 * cannot accidentally flush a cache line that also holds unrelated data
 * sitting just before/after the pool. The individual sub-allocations
 * inside the pool are still only 8-byte aligned (ThreadX byte pool), which
 * is why the MSC callbacks must also round the maintenance range down/up
 * to the cache line.
 *
 * Use the HAL ALIGN_32BYTES() macro (from stm32u5xx_hal_def.h) so the
 * alignment directive works for every supported toolchain (GCC / IAR /
 * ARMCC) without sprinkling compiler-specific #pragma / __attribute__
 * lines here. */
ALIGN_32BYTES(static UCHAR ux_device_byte_pool_buffer[UX_DEVICE_APP_MEM_POOL_SIZE]);

static ULONG storage_interface_number;
static ULONG storage_configuration_number;
static ULONG cdc_acm_interface_number;
static ULONG cdc_acm_configuration_number;
static UX_SLAVE_CLASS_STORAGE_PARAMETER storage_parameter;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER cdc_acm_parameter;

/* USER CODE BEGIN PV */

/* SCSI INQUIRY identity fields.
 *
 * USBX's _ux_device_class_storage_inquiry() does a FIXED-LENGTH memcpy of
 *   - 8 bytes from vendor_id
 *   - 16 bytes from product_id
 *   - 4 bytes from product_rev
 *   - 20 bytes from product_serial  (used by serial-page inquiry)
 * regardless of the actual C string length.  The SCSI SPC standard
 * requires these fields to be ASCII space-padded (0x20), NOT
 * null-terminated.  If the source buffer is shorter than the fixed
 * length, USBX reads out-of-bounds memory into the INQUIRY response;
 * and if any byte in the response is 0x00, strict SCSI parsers (in
 * particular Mac USB-C xHCI + AppleUSBMSC) reject the device and the
 * storage interface never attaches — on the same cable the CDC TTY
 * interface still comes up fine.  USB-A-through-a-TT and Windows are
 * lenient about this and hide the bug.
 *
 * Size these buffers to exactly the widths USBX reads, pre-fill with
 * 0x20, and do NOT leave a NUL terminator. */
static UCHAR vendor_id[8] = { 'S', 'h', 'i', 'm', 'm', 'e', 'r', ' ' };
static UCHAR product_id[16] = { 'S', 'h', 'i', 'm', 'm', 'e', 'r', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
static UCHAR product_rev[4] = { '1', '.', '0', '0' };
static UCHAR serial_id[20] = { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };

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

  /* Embed the Shimmer's 4-character MAC suffix into the SCSI INQUIRY
   * product_id field.  product_id MUST remain exactly 16 bytes,
   * space-padded, NOT null-terminated (see declaration comment).  Build
   * the suffix into a temporary and copy only the ASCII digits into
   * product_id[8..11], leaving the rest of the field as the pre-filled
   * spaces. */
  {
    char mac_suffix[8];
    size_t suffix_len;

    LogAndStream_buildShimmerMacSuffix(mac_suffix, sizeof(mac_suffix));
    suffix_len = strlen(mac_suffix);
    if (suffix_len > 4U)
    {
      suffix_len = 4U;
    }
    if (suffix_len > 0U)
    {
      /* product_id[0..6] = "Shimmer", [7] = ' ', so put suffix at [8..11] */
      memcpy(&product_id[8], mac_suffix, suffix_len);
    }
  }

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
  /* product_rev MUST be a valid 4-byte ASCII space-padded buffer.  USBX
   * reads 4 bytes unconditionally from this pointer; leaving it NULL (as
   * the previous code did) caused a 4-byte read from address 0 in the
   * INQUIRY response, which strict SCSI parsers (Mac USB-C xHCI) reject. */
  storage_parameter.ux_slave_class_storage_parameter_product_rev = product_rev;
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

  /* Record the init timestamp BEFORE bringing up the PCD / enabling USB
   * interrupts. Otherwise a SUSPENDED event delivered during enumeration
   * (between HAL_PCD_Start and the assignment below) would see
   * usbInitTick == 0 and the (HAL_GetTick() - 0) delta would immediately
   * exceed SUSPEND_GRACE_MS, triggering a false unplug. As a belt-and-
   * braces measure, USBD_ChangeFunction() also treats usbInitTick == 0 as
   * "still within the grace window". Use 1 as a sentinel for the unlikely
   * case that HAL_GetTick() returns 0 right at boot. */
  usbInitTick = HAL_GetTick();
  if (usbInitTick == 0U)
  {
    usbInitTick = 1U;
  }

  USBX_APP_Device_Init();

  usbx_initialized = true;
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
    if ((usbInitTick != 0U) && ((HAL_GetTick() - usbInitTick) >= SUSPEND_GRACE_MS))
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

  /* Initialize the device controller HAL driver.  MX_USB_OTG_HS_PCD_Init()
   * reads USB_getSpeed() internally and configures the peripheral for
   * either HS or FS accordingly. */
  MX_USB_OTG_HS_PCD_Init();

  /* ------------------------------------------------------------------
   * OTG_HS FIFO allocation
   *
   * The STM32U5 OTG_HS peripheral has a shared 1024-word FIFO RAM that
   * is partitioned between one RX FIFO (shared by all OUT endpoints)
   * and up to N TX FIFOs (one per IN endpoint).  The numbers below are
   * in 32-bit words.  Endpoint map (matches the USB descriptors):
   *
   *   EP0  IN/OUT  control      (TX FIFO 0, RX shared)
   *   EP1  IN      MSC bulk-IN       (TX FIFO 1)
   *   EP2  OUT     MSC bulk-OUT      (RX shared)
   *   EP3  IN      CDC notification  (TX FIFO 3)
   *   EP4  IN      CDC bulk-IN       (TX FIFO 4)
   *   EP5  OUT     CDC bulk-OUT      (RX shared)
   *
   * FIFO sizes differ for HS vs FS because the bulk max-packet-size
   * differs (HS = 512 bytes = 128 words, FS = 64 bytes = 16 words) and
   * each TX FIFO must be at least one full MPS.
   * ------------------------------------------------------------------ */
  if (USB_getSpeed() == USB_SPEED_HIGH)
  {
    /* ---------------- High-Speed FIFO layout ---------------------
     * Budget: 1024 words total.  HS bulk MPS = 128 words (512 bytes).
     *
     *   RX  0x230 + TX0 0x20 + TX1 0x100 + TX2 0x10 + TX3 0x10 + TX4 0x80
     *    = 560 + 32 + 256 + 16 + 16 + 128 = 1008 / 1024 words.
     */

    /* RX: shared across EP0 SETUP + MSC-OUT + CDC-OUT.  560 words gives
     * ~4.5x HS bulk MPS of cushion for back-to-back bulk-OUT packets
     * from xHCI hosts (no upstream TT to rate-limit). */
    HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x230); //560 words

    /* TX0 – EP0 control IN.  4x EP0 MPS (64B). */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, 0x20); //32 words

    /* TX1 – MSC bulk-IN.  2x HS MPS so the core can queue the next IN
     * packet while the previous one is still on the wire, maximising
     * MSC throughput. */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, 0x100); //256 words (2x HS MPS)

    /* TX2 – unused IN EP2 placeholder.  Must still be >= 16 words
     * because TX3 and TX4 are populated. */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, 0x10); //16 words

    /* TX3 – CDC notification (interrupt IN, MPS = 8 bytes = 2 words).
     * 16 words is 8x MPS, plenty for SERIAL_STATE / line-state
     * notifications which are tiny and infrequent. */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 3, 0x10); //16 words

    /* TX4 – CDC bulk-IN.  MUST be >= 1x HS bulk MPS (128 words / 512
     * bytes): the Synopsys OTG core requires each TxFIFO to hold at
     * least one full max-packet-size, otherwise the endpoint can hang
     * with ux_device_class_cdc_acm_write_run() stuck in UX_STATE_WAIT
     * (symptom: CDC enumerates and the TTY opens but no bytes come out
     * of the device until the host eventually tears the composite
     * device down ~60 s later).  2x MPS would be nicer for back-to-back
     * throughput but the total budget is 1024 words and MSC is
     * prioritised for large-file-copy performance. */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 4, 0x80); //128 words (1x HS MPS)
  }
  else
  {
    /* ---------------- Full-Speed FIFO layout ---------------------
     * Budget: 1024 words total.  FS bulk/control MPS = 16 words (64
     * bytes), interrupt MPS here = 2 words (8 bytes).  FS needs far
     * less RAM than HS; we still allocate a generous cushion because
     * the budget is plentiful.
     *
     *   RX  0x80 + TX0 0x20 + TX1 0x40 + TX2 0x10 + TX3 0x10 + TX4 0x40
     *    = 128 + 32 + 64 + 16 + 16 + 64 = 320 / 1024 words.
     */

    /* RX: shared across EP0 SETUP + MSC-OUT + CDC-OUT.  128 words is
     * ~8x FS bulk MPS of cushion – easily enough for two back-to-back
     * 64-byte OUT packets plus SETUP overhead. */
    HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_HS, 0x80); //128 words

    /* TX0 – EP0 control IN.  2x EP0 MPS. */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 0, 0x20); //32 words

    /* TX1 – MSC bulk-IN.  4x FS MPS so the core can queue several IN
     * packets ahead of the wire.  At FS the wall-clock throughput is
     * bounded by the 12 Mbps line rate, so over-provisioning beyond
     * ~4x MPS offers no benefit. */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 1, 0x40); //64 words (4x FS MPS)

    /* TX2 – unused IN EP2 placeholder.  Must still be >= 16 words. */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 2, 0x10); //16 words

    /* TX3 – CDC notification (interrupt IN, MPS = 8 bytes = 2 words). */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 3, 0x10); //16 words

    /* TX4 – CDC bulk-IN.  Must be >= 1x FS MPS.  4x MPS gives headroom
     * for back-to-back IN packets without forcing the CDC write path to
     * wait for FIFO drain. */
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_HS, 4, 0x40); //64 words (4x FS MPS)
  }

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
