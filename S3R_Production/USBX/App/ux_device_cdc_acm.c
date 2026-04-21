/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    ux_device_cdc_acm.c
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
#include "ux_device_cdc_acm.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dcache.h"
#include "usb_otg.h"
#include "ux_dcd_stm32.h"

#include <string.h>
/* USER CODE END Includes */

extern DCACHE_HandleTypeDef hdcache1;

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
usbx_cdc_acm_t usbx_cdc_tx_rx = { NULL, cdc_rx_buffer, 0, APP_RX_DATA_SIZE, 0, 0, UX_STATE_RESET,
  UX_STATE_RESET, usbx_success, usbx_success, 0, 1, 0, 1, 0, 0, NULL, 0 };
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
UX_SLAVE_CLASS_CDC_ACM *cdc_acm = NULL;

/* Bulk endpoint buffers handed to the Synopsys OTG DMA. Two hard
 * requirements from the controller / cache:
 *   1) Transfer buffer pointer must be 4-byte aligned (Synopsys DMA);
 *      misalignment -> AHB bus fault -> HardFault.
 *   2) On STM32U5 with D-cache enabled, bulk endpoints bypass the cache,
 *      so every CPU<->DMA handoff needs cache maintenance. To avoid
 *      false-sharing with adjacent globals on clean/invalidate, pad each
 *      buffer onto its own 32-byte cache line. ALIGN_32BYTES() satisfies
 *      both constraints in one go.
 * APP_TX_DATA_SIZE / APP_RX_DATA_SIZE are already multiples of 32, so the
 * rounded-up maintenance ranges always land on whole cache lines. */
ALIGN_32BYTES(uint8_t cdc_tx_buffer[APP_TX_DATA_SIZE]);
ALIGN_32BYTES(uint8_t cdc_rx_buffer[APP_RX_DATA_SIZE]);
uint8_t cdc_command_buffer[RX_COMMAND_BUFFER_SIZE];

static volatile bool cdc_port_open = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  USBD_CDC_ACM_Activate
 *         This function is called when insertion of a CDC ACM device.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_Activate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Activate */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  cdc_acm = (UX_SLAVE_CLASS_CDC_ACM *) cdc_acm_instance;
  cdc_port_open = false; /* host hasn't opened the serial port yet */
  /* USER CODE END USBD_CDC_ACM_Activate */

  return;
}

/**
 * @brief  USBD_CDC_ACM_Deactivate
 *         This function is called when extraction of a CDC ACM device.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_Deactivate(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_Deactivate */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);
  cdc_acm = UX_NULL;
  cdc_port_open = false;
  /* USER CODE END USBD_CDC_ACM_Deactivate */

  return;
}

/**
 * @brief  USBD_CDC_ACM_ParameterChange
 *         This function is invoked to manage the CDC ACM class requests.
 * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
 * @retval none
 */
VOID USBD_CDC_ACM_ParameterChange(VOID *cdc_acm_instance)
{
  /* USER CODE BEGIN USBD_CDC_ACM_ParameterChange */
  UX_PARAMETER_NOT_USED(cdc_acm_instance);

  if (cdc_acm != UX_NULL)
  {
    ULONG status;
    ULONG line_state = 0U;

    /* Query the CDC ACM line state using the official USBX API
     * instead of accessing internal struct members directly. */
    status = ux_device_class_cdc_acm_ioctl(
        cdc_acm, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE, &line_state);

    if (status == UX_SUCCESS)
    {
      /* DTR line state is indicated by the DTR bit in the line_state. */
      cdc_port_open = ((line_state & UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_DTR) != 0U);
    }
  }
  /* USER CODE END USBD_CDC_ACM_ParameterChange */

  return;
}

/* USER CODE BEGIN 1 */

/* Cache maintenance helpers for the bulk endpoint buffers. Round down the
 * start and up the end to whole 32-byte D-cache lines so partial lines at
 * either end are flushed/invalidated too (same pattern used in
 * ux_device_msc.c). cdc_tx_buffer / cdc_rx_buffer are themselves aligned
 * to 32 bytes and their sizes are multiples of 32, so in practice the
 * round-up is a no-op for the static buffers — the macro is kept for
 * safety in case the sub-region being maintained is arbitrary. */
static inline void cdc_dcache_clean(const void *addr, uint32_t size)
{
  uint32_t start = ((uint32_t) addr) & ~0x1FU;
  uint32_t end   = (((uint32_t) addr) + size + 31U) & ~0x1FU;
  HAL_DCACHE_CleanByAddr(&hdcache1, (uint32_t *) start, end - start);
}

static inline void cdc_dcache_invalidate(const void *addr, uint32_t size)
{
  uint32_t start = ((uint32_t) addr) & ~0x1FU;
  uint32_t end   = (((uint32_t) addr) + size + 31U) & ~0x1FU;
  HAL_DCACHE_InvalidateByAddr(&hdcache1, (uint32_t *) start, end - start);
}

/**
 * @brief  Transmit a caller-supplied buffer over the CDC-ACM bulk-IN pipe.
 *
 * The Synopsys OTG DMA used by USBX_CDC_ACM requires the transfer buffer
 * pointer to be 4-byte aligned; a misaligned pointer produces an AHB bus
 * fault -> HardFault. Callers pass arbitrary pointers:
 *   - uartRespBuf[] (global, alignment not explicitly asserted)
 *   - char str[] stack locals (e.g. hal_FactoryTest.c), 1-byte aligned
 *   - future callers we cannot audit.
 *
 * To make this call site safe regardless of caller alignment, copy the
 * payload into our own 32-byte-aligned static bounce buffer
 * (cdc_tx_buffer[]) and drive USBX from that. The extra memcpy is
 * negligible at CDC data rates. The same copy also lets us do the
 * D-cache Clean on a known-aligned, known-sized region.
 *
 * Flow-control / hang avoidance
 * -----------------------------
 * Windows exposes a CDC-ACM interface as a COM port as soon as the
 * device enumerates, but the bulk-IN pipe is only drained by the host
 * after an application (PuTTY, our config tool, ...) actually OPENS the
 * COM port and asserts DTR. If nothing has opened the port, sending
 * bulk-IN packets just fills the device's internal TxFIFO and then
 * NAKs forever -> the synchronous busy-wait below would hang until
 * watchdog reset.
 *
 * USBD_CDC_ACM_ParameterChange() tracks host DTR in cdc_port_open;
 * USBX_CDC_ACM_IsPortOpen() exposes it. We refuse to start a transfer
 * when !cdc_port_open and return usbx_error, so the caller can drop
 * the response rather than wedge the main loop.
 *
 * As a second layer of defense we also enforce a wall-clock timeout on
 * the busy-wait. If the OTG completion IRQ does not fire within
 * CDC_TX_CHUNK_TIMEOUT_MS (host disappears, cable pulled with mid-
 * transfer, host-side driver hung, ...), we abort the endpoint to clean
 * the hardware state, reset USBX's internal write state machine, and
 * return usbx_error. Without this the previous code could sit in
 * `while (tx_active) cdc_acm_write_task();` forever.
 */

/* Per-chunk timeout for the CDC TX path, measured from the moment the
 * transfer was queued. This is now a STALL watchdog only: a transfer
 * normally completes in sub-ms, but the host (Windows CDC-ACM driver)
 * is allowed to pause polling for extended periods when its internal
 * read buffer fills up. We do not want to abort healthy-but-paused
 * transfers, so CDC_TX_STALL_TIMEOUT_MS is generous. Set too low we
 * discard good data; set too high we take too long to recover from a
 * genuinely dead pipe.
 *
 * 5 seconds handles: Windows selective-suspend / app context switches /
 * host-side CDC driver warm-up after port open (the observed 5-10 s
 * first-reply delay). A genuinely dead pipe (cable pulled, host
 * crashed) is already detected by the (cdc_acm == NULL ||
 * state != CONFIGURED) guard in cdc_acm_write_task(), which fires
 * within a single poll. */
#define CDC_TX_STALL_TIMEOUT_MS 5000U

/* How long USBX_CDC_ACM_Transmit() is allowed to drive the engine
 * synchronously before it returns and lets the main loop finish the
 * transfer asynchronously. Kept small to preserve main-loop
 * responsiveness while still letting tiny responses complete before
 * the caller returns (so most call sites observe sync-like behaviour
 * under normal conditions).
 *
 * If the host is actively polling EP 0x84, a 512-byte packet completes
 * in ~ tens of microseconds, so 20 ms is orders of magnitude above the
 * expected completion time. If completion has not occurred by then,
 * the host is pausing and we return to the main loop. */
#define CDC_TX_SYNC_DRIVE_MS 20U

/* How long USBX_CDC_ACM_Transmit() will wait for a PREVIOUSLY-queued
 * transfer to drain before refusing a new one. Kept small. If we
 * return usbx_busy, the caller can either retry or drop the payload. */
#define CDC_TX_QUEUE_DRAIN_MS 10U

/* Diagnostics. Exposed as volatiles so they can be inspected from a
 * debugger Expressions view without recompiling. They are cumulative
 * across the lifetime of the device. */
volatile uint32_t cdc_tx_calls               = 0; /* USBX_CDC_ACM_Transmit() entries */
volatile uint32_t cdc_tx_dropped_not_open    = 0; /* dropped because host had not opened the port */
volatile uint32_t cdc_tx_dropped_not_configd = 0; /* dropped because device not CONFIGURED or cdc_acm NULL */
volatile uint32_t cdc_tx_dropped_bad_args    = 0; /* NULL buffer / zero size */
volatile uint32_t cdc_tx_dropped_busy        = 0; /* dropped because a previous transfer was still in flight */
volatile uint32_t cdc_tx_truncated           = 0; /* caller payload > APP_TX_DATA_SIZE: truncated */
volatile uint32_t cdc_tx_chunks_ok           = 0; /* chunks that completed cleanly */
volatile uint32_t cdc_tx_chunks_queued       = 0; /* chunks that outlived the sync window and were finished asynchronously */
volatile uint32_t cdc_tx_chunks_timeout      = 0; /* chunks aborted on the stall watchdog */
volatile uint32_t cdc_tx_chunks_error        = 0; /* chunks aborted by engine error (non-timeout) */
volatile uint32_t cdc_tx_bytes_ok            = 0; /* cumulative bytes successfully handed off */
volatile uint32_t cdc_tx_last_timeout_tick   = 0; /* HAL_GetTick() of most recent timeout */
volatile uint32_t cdc_tx_queued_tick         = 0; /* HAL_GetTick() when the current in-flight transfer was queued */

/* Snapshot of the OTG-HS CDC bulk-IN (EP 0x84) registers taken at the
 * moment of the most recent timeout. Populated by cdc_tx_abort_and_reset()
 * BEFORE we touch anything, so the captured values describe the stuck
 * state. Read them from a debugger Expressions view. Key bits:
 *   - fault_diepctl4:   EPENA (bit 31) | EPDIS (bit 30) | SNAK (bit 27) | CNAK (bit 26)
 *                       Stuck state typically shows EPENA=1 and the
 *                       packet count in DIEPTSIZ4 != 0, meaning the
 *                       core is waiting for the host to send an IN
 *                       token that never comes.
 *   - fault_dieptsiz4:  XFRSIZ (bits 18:0) = bytes remaining,
 *                       PKTCNT (bits 28:19) = packets remaining.
 *   - fault_diepint4:   pending EP-level interrupt flags; XFRC (bit 0)
 *                       should fire on DMA completion but won't if the
 *                       host never polls.
 *   - fault_diepdma4:   current DMA address; should equal the buffer
 *                       we handed to HAL_PCD_EP_Transmit + any bytes
 *                       the core already consumed. */
volatile uint32_t fault_diepctl4   = 0;
volatile uint32_t fault_dieptsiz4  = 0;
volatile uint32_t fault_diepint4   = 0;
volatile uint32_t fault_diepdma4   = 0;
volatile uint32_t fault_gintsts    = 0;
volatile uint32_t fault_dsts       = 0;

/* Abort the CDC bulk-IN transfer in progress and put all state machines
 * back into a clean RESET so the next USBX_CDC_ACM_Transmit() call can
 * start from scratch.
 *
 * CRITICAL ordering issue that bit us in DMA mode
 * -----------------------------------------------
 * In USBX standalone mode, _ux_dcd_stm32_transfer_run() tracks transfer
 * progress via the per-endpoint descriptor flags:
 *     UX_DCD_STM32_ED_STATUS_TRANSFER  -- set when transfer starts
 *     UX_DCD_STM32_ED_STATUS_DONE      -- set by the XFRC IRQ on completion
 *
 * On every subsequent call the DCD transfer_run() does:
 *     if (ed_status & TRANSFER) {
 *         if (ed_status & DONE) { ...clear... return UX_STATE_NEXT; }
 *         return UX_STATE_WAIT;
 *     }
 *     // start new transfer...
 *
 * If the completion IRQ never fires (host stopped draining, bulk-IN
 * NAK flood, cable glitch, ...) and we only call HAL_PCD_EP_Abort, the
 * TRANSFER flag stays set but DONE never gets set. The NEXT call to
 * USBX_CDC_ACM_Transmit() then immediately returns UX_STATE_WAIT
 * without re-arming the hardware -> times out again -> permanent hang.
 *
 * USBX's own _ux_dcd_stm32_transfer_abort() has the same bug (it calls
 * HAL_PCD_EP_Abort + HAL_PCD_EP_Flush and never touches ed_status).
 *
 * We fix it here: walk to the UX_DCD_STM32_ED for EP 0x84 via the
 * documented _stm32_ed_get() helper (ux_dcd_stm32.h), and clear both
 * TRANSFER and DONE. USED / STALLED / TASK_PENDING are preserved so
 * ux_device_class_cdc_acm_write_run() can re-start on the next poll. */
static void cdc_tx_abort_and_reset(void)
{
  /* STEP 0: snapshot the OTG-HS IN-EP4 registers BEFORE we touch anything,
   * so debugging the stuck state is possible after the abort has cleaned
   * it up. These go into the fault_* globals above. Only meaningful when
   * the peripheral is actually up. */
  if (hpcd_USB_OTG_HS.Instance != NULL)
  {
    USB_OTG_GlobalTypeDef   *USBx    = hpcd_USB_OTG_HS.Instance;
    uint32_t                 base    = (uint32_t) USBx;
    USB_OTG_INEndpointTypeDef *in_ep4 = (USB_OTG_INEndpointTypeDef *) (base + USB_OTG_IN_ENDPOINT_BASE + (4U * USB_OTG_EP_REG_SIZE));
    USB_OTG_DeviceTypeDef   *dev_reg = (USB_OTG_DeviceTypeDef *) (base + USB_OTG_DEVICE_BASE);

    fault_diepctl4  = in_ep4->DIEPCTL;
    fault_dieptsiz4 = in_ep4->DIEPTSIZ;
    fault_diepint4  = in_ep4->DIEPINT;
    fault_diepdma4  = in_ep4->DIEPDMA;
    fault_gintsts   = USBx->GINTSTS;
    fault_dsts      = dev_reg->DSTS;
  }

  /* Kick the hardware off the endpoint first so no stale DMA descriptor
   * survives. HAL_PCD_EP_Abort disables the IN endpoint via SNAK/EPDIS
   * and flushes the associated TxFIFO. Ignore the return code — even if
   * it fails we still want to wipe the software state. */
  if (hpcd_USB_OTG_HS.Instance != NULL)
  {
    (void) HAL_PCD_EP_Abort(&hpcd_USB_OTG_HS, 0x84U); /* CDC Data IN */
    (void) HAL_PCD_EP_Flush(&hpcd_USB_OTG_HS, 0x84U);

    /* CRITICAL: clear Global IN NAK if it's effective.
     *
     * HAL_PCD_EP_Abort -> USB_EPStopXfer() executes the Synopsys "Global
     * IN NAK dance" on IN endpoints:
     *     1. DCTL.SGINAK = 1        (set global IN NAK)
     *     2. wait for GINTSTS.GINAKEFF
     *     3. DIEPCTL.SNAK|EPDIS = 1 (disable endpoint)
     *     4. wait for DIEPINT.EPDISD
     *     5. flush TxFIFO
     *     6. DCTL.CGINAK = 1        (clear global IN NAK)
     *
     * If step 2 or 4 times out (exactly what happens when the host has
     * stopped polling the endpoint, which is WHY we got here in the
     * first place), USB_EPStopXfer returns HAL_ERROR WITHOUT reaching
     * step 6. GINAKEFF stays latched and the core NAKs EVERY IN token
     * from the host on EVERY IN endpoint from that point on -- CDC and
     * MSC both go dead, Windows gives up after ~60s and tears the
     * composite down.
     *
     * Observed: fault_gintsts = 0x44808428 -> bit 10 GINAKEFF = 1 after
     * the first watchdog-triggered abort. Fix is to unconditionally
     * issue CGINAK here: writing 1 to DCTL.CGINAK when global NAK is
     * not set is harmless (it's self-clearing). */
    {
      USB_OTG_GlobalTypeDef      *USBx    = hpcd_USB_OTG_HS.Instance;
      uint32_t                    base    = (uint32_t) USBx;
      USB_OTG_INEndpointTypeDef  *in_ep4  = (USB_OTG_INEndpointTypeDef *) (base + USB_OTG_IN_ENDPOINT_BASE + (4U * USB_OTG_EP_REG_SIZE));
      USB_OTG_DeviceTypeDef      *dev_reg = (USB_OTG_DeviceTypeDef *) (base + USB_OTG_DEVICE_BASE);

      /* Write-1-to-clear all pending flags on this EP. */
      in_ep4->DIEPINT = 0xFFFFFFFFU;

      /* DCTL.CGINAK (bit 8) = clear global IN NAK. Self-clearing. */
      dev_reg->DCTL |= USB_OTG_DCTL_CGINAK;

      /* Also clear the GINAKEFF latch in GINTSTS (write-1-to-clear). */
      USBx->GINTSTS = USB_OTG_GINTSTS_GINAKEFF;
    }
  }

  /* Clear the DCD endpoint descriptor's in-flight flags. Without this,
   * every future transfer_run() on this EP returns UX_STATE_WAIT because
   * it sees TRANSFER && !DONE. See big comment block above. */
  {
    UX_SLAVE_DCD *dcd = &_ux_system_slave->ux_system_slave_dcd;
    UX_DCD_STM32 *dcd_stm32 = (UX_DCD_STM32 *) dcd->ux_slave_dcd_controller_hardware;
    if (dcd_stm32 != UX_NULL)
    {
      UX_DCD_STM32_ED *ed = _stm32_ed_get(dcd_stm32, 0x84U); /* CDC Data IN */
      if (ed != UX_NULL)
      {
        ed->ux_dcd_stm32_ed_status &= ~(UX_DCD_STM32_ED_STATUS_TRANSFER
            | UX_DCD_STM32_ED_STATUS_DONE);
      }
    }
  }

  /* Reset USBX's per-class write state so its internal
   * UX_DEVICE_CLASS_CDC_ACM_WRITE_WAIT does not come back and try to
   * complete on a freed/stale transfer_request next time. */
  if (cdc_acm != UX_NULL)
  {
    cdc_acm->ux_device_class_cdc_acm_write_state         = UX_STATE_RESET;
    cdc_acm->ux_device_class_cdc_acm_write_status        = UX_TRANSFER_NO_ANSWER;
    cdc_acm->ux_device_class_cdc_acm_write_actual_length = 0;
    cdc_acm->ux_device_class_cdc_acm_write_requested_length = 0;
  }

  /* Reset our wrapper state machine. */
  usbx_cdc_tx_rx.tx_active       = 0;
  usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
  usbx_cdc_tx_rx.tx_count        = 0;
  usbx_cdc_tx_rx.tx_pending      = 0;
  usbx_cdc_tx_rx.tx_scheduled    = 0;
  usbx_cdc_tx_rx.tx_result       = usbx_error;

  /* IMPORTANT: clear the stall-watchdog timestamp. Without this, if the
   * next transfer gets queued and the watchdog re-evaluates before the
   * new queue_tick stamp is written, it could see a stale timestamp and
   * abort the fresh transfer immediately. The new queue path always
   * writes cdc_tx_queued_tick, so clearing here is defensive but the
   * correct "idle" marker. */
  cdc_tx_queued_tick = 0U;
}

/* OTG-HS completion-interrupt diagnostic. HAL_PCD_DataInStageCallback is
 * called from the OTG IRQ when DIEPINT.XFRC fires for an IN endpoint;
 * USBX's ux_dcd_stm32_callback.c provides the real handler. We override
 * it here as a *wrapper* that bumps per-endpoint counters so we can tell
 * at a glance whether the XFRC interrupt is firing at all for EP 0x84.
 *
 * Observed symptom: a queued transfer sits with DIEPCTL.EPENA=1 and
 * NAKSTS=0 for 5 seconds until the stall watchdog aborts it. Two very
 * different mechanisms can produce that:
 *   (a) Host stopped polling the IN endpoint entirely. XFRC never
 *       fires because the data is never transmitted on the wire.
 *   (b) Host IS polling, packets ARE transmitted and ACKed, but the
 *       IRQ path is broken and we never observe the completion.
 *
 * Counting IRQ entries for EP 0x84 separates (a) from (b):
 *   - (a): cdc_in_xfrc_count stays flat during the 5 s stall.
 *   - (b): cdc_in_xfrc_count increments but our state machine never
 *          clears tx_active -- i.e., USBX's HAL_PCD_DataInStageCallback
 *          is being bypassed or the ED lookup is returning a different
 *          endpoint.
 * USBX's own callback is a strong symbol, so wrapping here without
 * modifying USBX source needs a link-time trick. The simpler path is
 * to just instrument via a peripheral-register pre/post snapshot in
 * the main loop; exposing the hook here would force editing the
 * middleware. Skipping for now; see the note at the top of
 * USBX_CDC_ACM_Transmit() for the A/B test we actually need. */
volatile uint32_t cdc_in_xfrc_count = 0; /* reserved for future instrumentation */

usbx_cdc_acm_result_t USBX_CDC_ACM_Transmit(uint8_t *buffer, uint16_t size)
{
  cdc_tx_calls++;

  if (!cdc_acm || _ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED)
  {
    cdc_tx_dropped_not_configd++;
    usbx_cdc_tx_rx.tx_result = usbx_error;
    return usbx_error;
  }

  if (buffer == NULL || size == 0)
  {
    cdc_tx_dropped_bad_args++;
    usbx_cdc_tx_rx.tx_result = usbx_error;
    return usbx_error;
  }

  /* Host-side flow control. If the COM port has not been opened by any
   * application on the host, Windows / macOS / Linux will not drain the
   * bulk-IN endpoint and the transfer will NAK indefinitely. Drop the
   * payload instead of blocking the main loop until the next reset. */
  if (!USBX_CDC_ACM_IsPortOpen())
  {
    cdc_tx_dropped_not_open++;
    usbx_cdc_tx_rx.tx_result = usbx_error;
    return usbx_error;
  }

  /* Queue-contention handling
   * -------------------------
   * A previous transmit may still be in flight (the main loop's
   * cdc_acm_write_task() has not yet seen completion). This can happen
   * when:
   *   (a) Windows paused polling the bulk-IN endpoint because its
   *       read queue is full (we observed fault_diepctl4 = 0x81098200
   *       -> EPENA=1, NAKSTS=0: device armed, host not polling). The
   *       core is healthy; we just have to wait for the host.
   *   (b) The previous transfer is in its normal (sub-millisecond)
   *       completion window and the main loop has not run since.
   *
   * Try to drain for a short bounded time, then either:
   *   - slot opens up: queue the new chunk.
   *   - in-flight transfer has exceeded the stall watchdog: abort it
   *     and queue the new chunk (data loss for the stuck transfer,
   *     but recovery for all future ones).
   *   - otherwise: return usbx_busy. Caller drops or retries on its
   *     own schedule. */
  if (usbx_cdc_tx_rx.tx_active)
  {
    uint32_t drain_start = HAL_GetTick();
    while (usbx_cdc_tx_rx.tx_active
        && (HAL_GetTick() - drain_start) < CDC_TX_QUEUE_DRAIN_MS)
    {
      cdc_acm_write_task();
    }

    if (usbx_cdc_tx_rx.tx_active)
    {
      /* Transfer is wedged. If the stall watchdog has expired, abort
       * it here so this new transmit can proceed. Otherwise drop. */
      if (cdc_tx_queued_tick != 0U
          && (HAL_GetTick() - cdc_tx_queued_tick) >= CDC_TX_STALL_TIMEOUT_MS)
      {
        cdc_tx_chunks_timeout++;
        cdc_tx_last_timeout_tick = HAL_GetTick();
        cdc_tx_abort_and_reset();
      }
      else
      {
        cdc_tx_dropped_busy++;
        return usbx_busy;
      }
    }
  }

  /* Size handling: the bounce buffer is APP_TX_DATA_SIZE (4 KB) and
   * the new API is one-chunk-per-call. If the caller's payload is
   * bigger than the bounce buffer we truncate rather than implement a
   * chained multi-chunk queue. None of the current callers exceed this
   * (factory test strings, dock response packets). The counter lets
   * future callers notice. */
  uint16_t chunk = (size > (uint16_t) APP_TX_DATA_SIZE)
                       ? (uint16_t) APP_TX_DATA_SIZE
                       : size;
  if (size > (uint16_t) APP_TX_DATA_SIZE)
  {
    cdc_tx_truncated++;
  }

  /* Copy caller's payload into our DMA-safe bounce buffer. */
  memcpy(cdc_tx_buffer, buffer, chunk);

  /* OTG DMA reads bypass the D-cache; clean to push the freshly copied
   * bytes from cache down to SRAM before the DMA engine fetches them. */
  cdc_dcache_clean(cdc_tx_buffer, chunk);

  usbx_cdc_tx_rx.tx_buffer       = cdc_tx_buffer;
  usbx_cdc_tx_rx.tx_length       = chunk;
  usbx_cdc_tx_rx.tx_count        = 0;
  usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
  usbx_cdc_tx_rx.tx_scheduled    = 1;
  usbx_cdc_tx_rx.tx_active       = 1;
  usbx_cdc_tx_rx.tx_pending      = 0;
  usbx_cdc_tx_rx.tx_result       = usbx_success;
  cdc_tx_queued_tick             = HAL_GetTick();

  /* Synchronous-ish fast path: drive the engine briefly so small
   * messages typically complete before we return. Bounded so the main
   * loop is never blocked by a paused host.
   *
   * This is NOT a timeout; it's just how much latency we're willing to
   * spend on behalf of the caller. When this window elapses with the
   * transfer still in flight, we return usbx_success (queued) and the
   * main loop's cdc_acm_write_task() carries it to completion (or the
   * stall watchdog eventually reaps it). */
  uint32_t sync_start = HAL_GetTick();
  while (usbx_cdc_tx_rx.tx_active
      && (HAL_GetTick() - sync_start) < CDC_TX_SYNC_DRIVE_MS)
  {
    cdc_acm_write_task();

    /* Device disappeared mid-transfer: unwind and report error. */
    if (cdc_acm == UX_NULL
        || _ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED)
    {
      cdc_tx_chunks_error++;
      cdc_tx_abort_and_reset();
      return usbx_error;
    }
  }

  if (!usbx_cdc_tx_rx.tx_active)
  {
    /* Completed within the sync window. write_task()'s UX_STATE_NEXT
     * branch already credited chunks_ok / bytes_ok and cleared
     * cdc_tx_queued_tick, so we only need to surface the result. */
    if (usbx_cdc_tx_rx.tx_result != usbx_success)
    {
      cdc_tx_abort_and_reset();
      return usbx_cdc_tx_rx.tx_result;
    }
    return usbx_success;
  }

  /* Still in flight. Main loop will finish it. */
  cdc_tx_chunks_queued++;
  return usbx_success;
}

VOID cdc_acm_write_task(VOID)
{
  if (!usbx_cdc_tx_rx.tx_active)
  {
    return; //nothing to do
  }

  /* Defensive re-check inside the engine body. If the USB cable is
   * pulled, the host re-enumerates, or Windows issues a BOT reset
   * during the transfer, USBD_CDC_ACM_Deactivate() sets cdc_acm to
   * UX_NULL and the device state drops out of UX_DEVICE_CONFIGURED.
   * Calling ux_device_class_cdc_acm_write_run() with cdc_acm == NULL
   * dereferences NULL at the "not configured" bail-out path
   * (write_run.c line 137 writes cdc_acm->write_state) and
   * HardFaults. Bail cleanly instead. */
  if (cdc_acm == UX_NULL
      || _ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED)
  {
    cdc_tx_chunks_error++;
    cdc_tx_abort_and_reset();
    return;
  }

  /* Stall watchdog. If the host has stopped polling the bulk-IN
   * endpoint (observed symptom: DIEPCTL.EPENA=1, NAKSTS=0 for
   * many seconds), abort so the next transmit can fire a fresh
   * transfer. We ONLY check in this async/main-loop path; the
   * synchronous path in USBX_CDC_ACM_Transmit() never waits this
   * long so it never trips here. */
  if (cdc_tx_queued_tick != 0U
      && (HAL_GetTick() - cdc_tx_queued_tick) >= CDC_TX_STALL_TIMEOUT_MS)
  {
    cdc_tx_chunks_timeout++;
    cdc_tx_last_timeout_tick = HAL_GetTick();
    cdc_tx_abort_and_reset();
    return;
  }

  ULONG actual_length = 0;
  UINT retVal;
  switch (usbx_cdc_tx_rx.tx_engine_state)
  {
  case UX_STATE_RESET:
    if (usbx_cdc_tx_rx.tx_scheduled || usbx_cdc_tx_rx.tx_pending)
    {
      usbx_cdc_tx_rx.tx_engine_state = UX_STATE_WAIT;
    }
    else
    {
      usbx_cdc_tx_rx.tx_active = 0;
    }
    break;
  case UX_STATE_WAIT:
    if (usbx_cdc_tx_rx.tx_scheduled || usbx_cdc_tx_rx.tx_pending)
    {
      //Call USBX to transmit remaining bytes
      retVal = ux_device_class_cdc_acm_write_run(cdc_acm,
          usbx_cdc_tx_rx.tx_buffer + usbx_cdc_tx_rx.tx_count,
          usbx_cdc_tx_rx.tx_length - usbx_cdc_tx_rx.tx_count, &actual_length);
      //Handle error
      if (retVal < UX_STATE_IDLE)
      {
        //Fatal error: stop everything
        usbx_cdc_tx_rx.tx_result = usbx_error;
        usbx_cdc_tx_rx.tx_active = 0;
        usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
        usbx_cdc_tx_rx.tx_count = 0;
        usbx_cdc_tx_rx.tx_pending = 0;
        usbx_cdc_tx_rx.tx_scheduled = 0;
        cdc_tx_queued_tick = 0U;
        cdc_tx_chunks_error++;
      }
      //Handle successful completion
      else if (retVal == UX_STATE_NEXT)
      {
        /* Transmission fully completed. This path runs for every
         * transfer: the sync-drive loop in USBX_CDC_ACM_Transmit()
         * detects completion via tx_active dropping to 0, and the
         * main-loop async path (when the transfer outlived the sync
         * window) relies on the counter bumps here. Increment the
         * counters unconditionally but only credit bytes_ok for the
         * async case -- the sync path already credits it on return.
         *
         * To keep the accounting single-sourced and avoid double-
         * counting on small messages, we fold both cases into one
         * block by clearing cdc_tx_queued_tick here (async path) and
         * letting USBX_CDC_ACM_Transmit() check tx_active to decide
         * whether it owns the accounting. */
        if (cdc_tx_queued_tick != 0U)
        {
          /* Async completion: caller already returned. Credit stats. */
          cdc_tx_chunks_ok++;
          cdc_tx_bytes_ok += usbx_cdc_tx_rx.tx_length;
          cdc_tx_queued_tick = 0U;
        }
        usbx_cdc_tx_rx.tx_result = usbx_success;
        usbx_cdc_tx_rx.tx_active = 0;
        usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
        usbx_cdc_tx_rx.tx_count = 0;
        usbx_cdc_tx_rx.tx_pending = 0;
        usbx_cdc_tx_rx.tx_scheduled = 0;
      }
      else if (retVal == UX_STATE_WAIT)
      { //Partial transmission: update count and mark pending
        usbx_cdc_tx_rx.tx_count += actual_length;
        usbx_cdc_tx_rx.tx_pending = 1;
      }
      else
      {
        //Nothing to send, go idle
        usbx_cdc_tx_rx.tx_active = 0;
        usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
        cdc_tx_queued_tick = 0U;
      }
    }
    break;
  default: //Unknown state; reset
    usbx_cdc_tx_rx.tx_engine_state = UX_STATE_RESET;
    break;
  }
}

VOID cdc_acm_read_task(VOID)
{
  if (!cdc_acm || _ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED)
  {
    usbx_cdc_tx_rx.rx_result = usbx_error;
    return;
  }
  ULONG actual_length = 0;
  UINT status;
  switch (usbx_cdc_tx_rx.rx_engine_state)
  {
  case UX_STATE_RESET:
    if (usbx_cdc_tx_rx.rx_scheduled || usbx_cdc_tx_rx.rx_pending)
    {
      /* About to hand cdc_rx_buffer[] to OTG DMA. OTG DMA writes bypass
       * the D-cache, so invalidate the destination region first — any
       * dirty cache line (e.g. left over from the memset() performed at
       * the end of the previous receive cycle) could otherwise evict
       * stale bytes on top of DMA-written data and the CPU would read
       * corrupted payload. */
      cdc_dcache_invalidate(usbx_cdc_tx_rx.rx_buffer,
          usbx_cdc_tx_rx.rx_length - usbx_cdc_tx_rx.rx_count);
      usbx_cdc_tx_rx.rx_engine_state = UX_STATE_WAIT;
    }
    break;
  case UX_STATE_WAIT:
    status = ux_device_class_cdc_acm_read_run(cdc_acm,
        usbx_cdc_tx_rx.rx_buffer + usbx_cdc_tx_rx.rx_count,
        usbx_cdc_tx_rx.rx_length - usbx_cdc_tx_rx.rx_count, &actual_length);
    if (actual_length > 0U)
    {
      /* OTG DMA writes bypass the D-cache. Invalidate the range the DMA
       * just filled so the subsequent CPU read (memcpy into
       * cdc_command_buffer below) observes the freshly received bytes
       * instead of a stale cache line. */
      cdc_dcache_invalidate(usbx_cdc_tx_rx.rx_buffer + usbx_cdc_tx_rx.rx_count,
          actual_length);
    }
    usbx_cdc_tx_rx.rx_count += actual_length;
    if (status < UX_STATE_IDLE)
    {
      //Fatal error: stop everything
      usbx_cdc_tx_rx.rx_result = usbx_error;
      usbx_cdc_tx_rx.rx_count = 0;                     //discard current buffer
      usbx_cdc_tx_rx.rx_engine_state = UX_STATE_RESET; //reset RX state machine
      usbx_cdc_tx_rx.rx_pending = 0;
      usbx_cdc_tx_rx.rx_scheduled = 1; //keep engine active
    }
    else if (status == UX_STATE_NEXT)
    {
      //one full usb packet received
      usbx_cdc_tx_rx.rx_result = usbx_success;
      usbx_cdc_tx_rx.rx_engine_state = UX_STATE_RESET;
      //usbx_cdc_tx_rx.rx_count        += actual_length; // or process data here
      usbx_cdc_tx_rx.rx_pending = 0;
      usbx_cdc_tx_rx.rx_command_buffer = cdc_command_buffer;
      usbx_cdc_tx_rx.rx_command_length += usbx_cdc_tx_rx.rx_count;
      memcpy(usbx_cdc_tx_rx.rx_command_buffer, usbx_cdc_tx_rx.rx_buffer,
          usbx_cdc_tx_rx.rx_command_length);
      memset(usbx_cdc_tx_rx.rx_buffer, 0, usbx_cdc_tx_rx.rx_count);
      usbx_cdc_tx_rx.rx_count = 0;
      ShimTask_set(TASK_USB_PROCESS_CMD);
    }
    else if (status == UX_STATE_WAIT)
    {
      //Partial reception: mark pending
      usbx_cdc_tx_rx.rx_pending = 1;
    }
    //else still waiting, do nothing
    break;
  default: //Unknown state; reset
    usbx_cdc_tx_rx.rx_engine_state = UX_STATE_RESET;
    break;
  }
}

usbx_cdc_acm_result_t USBX_CDC_ACM_Receive_To_RxBuf(VOID)
{
  return USBX_CDC_ACM_Receive(usbx_cdc_tx_rx.rx_command_buffer, usbx_cdc_tx_rx.rx_command_length);
}

VOID USBX_CDC_ACM_RxBuf_Reset(VOID)
{
  memset(usbx_cdc_tx_rx.rx_command_buffer, 0, usbx_cdc_tx_rx.rx_command_length);
  usbx_cdc_tx_rx.rx_command_length = 0;
}

usbx_cdc_acm_result_t USBX_CDC_ACM_Receive(uint8_t *buffer, uint16_t size)
{
  for (uint32_t i = 0; i < size; i++)
  {
    ShimDock_rxCallback(buffer[i]);
  }
  return usbx_success;
}

/* Reports the COM port open state (configured + class active + DTR set) */
bool USBX_CDC_ACM_IsPortOpen(void)
{
  if (cdc_acm == UX_NULL)
  {
    return false;
  }
  if (_ux_system_slave->ux_system_slave_device.ux_slave_device_state != UX_DEVICE_CONFIGURED)
  {
    return false;
  }
  return cdc_port_open;
}

/* USER CODE END 1 */
