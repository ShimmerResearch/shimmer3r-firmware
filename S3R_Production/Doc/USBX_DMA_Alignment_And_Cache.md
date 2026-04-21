# USBX Device – DMA alignment, D-cache maintenance, VBUS/resume & regression matrix

Scope: STM32U5A5 (Shimmer3r), Synopsys OTG_HS in internal-DMA mode, ThreadX/USBX
device stack with MSC + CDC-ACM composite.

## 1. Alignment audit of USBX call sites

The Synopsys OTG DMA fetches/pushes from/to the transfer buffer pointer
given to `ux_device_class_*_write_run()` / `ux_device_class_*_read_run()`.
The controller requires **4-byte alignment** of that pointer. Misalignment
raises an AHB bus fault → HardFault.

On STM32U5 the D-cache line is **32 bytes**; any buffer also handed to a
DMA that bypasses the cache must additionally be sized/aligned to whole
cache lines to avoid false-sharing on Clean/Invalidate.

| Call site | Buffer | Alignment | Notes |
|-----------|--------|-----------|-------|
| MSC – `USBD_STORAGE_Read/Write` via USBX byte pool | `ux_device_byte_pool_buffer[]` | `ALIGN_32BYTES` (`app_usbx_device.c`) | Already safe. Sub-allocations are 8-byte aligned from ThreadX byte pool; MSC callbacks round the DCACHE maintenance range to whole 32-byte lines. |
| MSC – `USBD_STORAGE_Read/Write` `data_pointer` | Pointer supplied by USBX from the pool | inherited | OK because the pool is aligned and MSC rounds the maintenance range. |
| CDC – bulk IN (`USBX_CDC_ACM_Transmit`) | **`cdc_tx_buffer[]` (bounce buffer)** | `ALIGN_32BYTES` | Caller pointer is no longer handed to USBX. See §3. |
| CDC – bulk OUT (`cdc_acm_read_task`) | `cdc_rx_buffer[]` | `ALIGN_32BYTES` | Updated in `ux_device_cdc_acm.c`. |
| CDC – rx_command_buffer (`cdc_command_buffer[]`) | CPU-only (memcpy target) | N/A | Never touched by DMA. |
| Control transfers / string descriptors | USBX internal pool | aligned | Served from `ux_device_byte_pool_buffer[]`. |

Any future bulk EP buffer added to USBX must be either (a) allocated from
the USBX byte pool, or (b) declared with `ALIGN_32BYTES(...)` and copied
to/from caller memory via a bounce.

## 2. MSC

No action needed – `ux_device_byte_pool_buffer[]` is `ALIGN_32BYTES` and
`USBD_STORAGE_Read` / `USBD_STORAGE_Write` already perform the correct
cache maintenance pair (Clean before write-DMA, Invalidate after
read-DMA) with the range rounded down-and-up to a 32-byte cache line.

## 3. CDC RX

`cdc_rx_buffer[APP_RX_DATA_SIZE]` in `ux_device_cdc_acm.c` is now declared
`ALIGN_32BYTES(...)`. `APP_RX_DATA_SIZE = 4 KiB` is already a multiple of
32, so maintenance ranges always cover whole cache lines.

## 4. CDC TX – bounce buffer

`USBX_CDC_ACM_Transmit(uint8_t *buffer, uint16_t size)` historically
passed the caller's pointer straight through to
`ux_device_class_cdc_acm_write_run()`. Known callers include:

* `uartRespBuf[]` (global in `shimmer_dock_usart.c`) – alignment not
  asserted by source.
* `hal_FactoryTest.c::ShimFactoryTest_sendReportImpl` – passes a
  `char str[]` **stack local**; the ABI only guarantees 1-byte alignment
  for `char[]`, so this WILL occasionally misalign and fault.
* Any future caller we cannot audit.

Fix: `USBX_CDC_ACM_Transmit()` now copies the caller payload (in chunks of
`APP_TX_DATA_SIZE`) into the already-existing 32-byte-aligned
`cdc_tx_buffer[]`, performs a D-cache Clean over the copied range, and
drives USBX from the bounce. The extra `memcpy` cost is negligible
compared to USB HS wire time.

## 5. Control transfers / string descriptors

Allocated from `ux_device_byte_pool_buffer[]`, which is `ALIGN_32BYTES`.
No action needed.

## 6. D-cache maintenance audit (Clean vs. Invalidate)

Rule (AN4839 / ARM):
* **CPU wrote last → DMA reads:** `HAL_DCACHE_CleanByAddr` **before** DMA start.
* **DMA wrote last → CPU reads:** `HAL_DCACHE_InvalidateByAddr` **after** DMA completion (also acceptable before DMA start, provided the buffer is aligned/sized to whole cache lines and the CPU does not touch it during the DMA).

In OTG internal-DMA mode, bulk endpoints bypass the D-cache, so this rule
applies on every bulk transfer.

| Path | CPU wrote last? | Maintenance in code |
|------|-----------------|---------------------|
| MSC write (host→card) | Host wrote the bulk-OUT buffer via DMA, then CPU hands it to SDMMC DMA. **Clean** is the correct choice because after the DMA-OUT completes the core may have speculatively prefetched dirty-looking lines; clean pushes them to SRAM so the SDMMC DMA reads coherent bytes. | `USBD_STORAGE_Write`: `HAL_DCACHE_CleanByAddr` |
| MSC read (card→host) | SDMMC DMA wrote the buffer | `USBD_STORAGE_Read`: `HAL_DCACHE_InvalidateByAddr` |
| CDC TX (device→host) | CPU (`memcpy` into `cdc_tx_buffer`) | `cdc_dcache_clean(cdc_tx_buffer, chunk)` in `USBX_CDC_ACM_Transmit` |
| CDC RX (host→device) | OTG DMA | `cdc_dcache_invalidate(...)` before DMA (prevents dirty eviction) AND after DMA completes with `actual_length > 0` (ensures CPU sees fresh bytes) in `cdc_acm_read_task` |

CDC was previously unprotected; at CDC data rates the symptom is rare
(small transfers, cache lines evict quickly), but strictly the
maintenance is required and is now in place.

## 7. VBUS / resume handling – DMA mode re-verification

DMA mode changes some error-recovery paths (the core DMA engine has its
own transfer-in-flight state that survives individual packet errors).
The following scenarios must be re-verified whenever the USB stack,
OTG interrupt handling, or any bulk buffer is touched:

1. **Cold plug** USB-A → Windows, USB-A → macOS, USB-C → macOS, USB-C → Linux.
2. **Cable unplug during idle** (no transfer active).
3. **Cable unplug mid-transfer** (large file copy in flight).
4. **Cable re-attach after unplug** without power-cycling the device.
5. **Host-initiated BOT reset** (e.g. Windows "Safely Remove Hardware").
6. **macOS Finder "Eject"**.
7. **Selective suspend** from Windows (device must not self-disable if
   VBUS still present — see `SUSPEND_GRACE_MS` + `Board_isUsbPluggedIn()`
   gate in `app_usbx_device.c::USBD_ChangeFunction`).
8. **Mac USB-C / Thunderbolt** spurious suspend during enumeration
   (grace window must suppress).
9. **Bus-powered hub** and **self-powered hub** topologies.
10. **VBUS brown-out** (unpowered hub + high SD write load).

## 8. Regression matrix

Run each row against: MSC-only active, CDC-only active, MSC + CDC
concurrent (CDC TTY traffic while MSC is performing a large copy).

| Host | Port | Topology | Test |
|------|------|----------|------|
| Windows 10/11 | USB-A (2.0) | direct | Large file copy (≥ 1 GiB); verify CRC |
| Windows 10/11 | USB-A (2.0) | powered hub | Large file copy + hot-unplug mid-write |
| Windows 10/11 | USB-C | direct (xHCI) | Enumeration + large copy |
| macOS (Intel) | USB-A via TT | direct | Enumeration + large copy |
| macOS (Apple Silicon) | USB-C | direct (xHCI, no TT) | Enumeration (SCSI INQUIRY strict), large copy, Finder Eject |
| Linux (USB 2) | USB-A | direct | `dd if=… of=/dev/sdX bs=1M` + `sync` |
| Linux (USB 3) | USB-C | direct | Same + `smartctl` inquiry parse |
| Any | Any | bus-powered hub | Full enumeration, short file copy |
| Any | Any | — | CDC TTY echo test while MSC is reading |
| Any | Any | — | CDC TTY echo test while MSC is writing |
| Any | Any | — | Hot-plug during active MSC write |
| Any | Any | — | Boot with cable already inserted |
| Any | Any | — | Boot with cable unplugged, plug after `Init()` |

Pass criterion for each row: no HardFault, no USB stack lockup, MSC CRC
matches source, CDC TTY stream has no dropped/corrupted bytes, and the
CDC/MSC pair re-enumerates cleanly after every unplug/replug.
