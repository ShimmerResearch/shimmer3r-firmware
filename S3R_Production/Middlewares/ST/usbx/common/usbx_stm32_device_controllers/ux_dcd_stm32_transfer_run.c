/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   STM32 Controller Driver                                             */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define UX_SOURCE_CODE
#define UX_DCD_STM32_SOURCE_CODE


/* Include necessary system files.  */

#include "ux_api.h"
#include "ux_dcd_stm32.h"
#include "ux_utility.h"
#include "ux_device_stack.h"

/* Needed for HAL_DCACHE_CleanByAddr / HAL_DCACHE_InvalidateByAddr and
 * DCACHE_HandleTypeDef, used by the cache-maintenance block below. The
 * MX-generated "dcache.h" provides `extern DCACHE_HandleTypeDef hdcache1`
 * but we fall back to an inline extern declaration locally so this file
 * does not depend on the application's Core/Inc layout. */
#include "stm32u5xx_hal.h"


#if defined(UX_DEVICE_STANDALONE)
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                                RELEASE       */
/*                                                                        */
/*    _ux_dcd_stm32_transfer_request                       PORTABLE C     */
/*                                                            6.1.10      */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function will initiate a transfer to a specific endpoint.      */
/*    If the endpoint is IN, the endpoint register will be set to accept  */
/*    the request.                                                        */
/*                                                                        */
/*    If the endpoint is IN, the endpoint FIFO will be filled with the    */
/*    buffer and the endpoint register set.                               */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    dcd_stm32                             Pointer to device controller  */
/*    transfer_request                      Pointer to transfer request   */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    Completion Status                                                   */
/*                                                                        */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    HAL_PCD_EP_Transmit                   Transmit data                 */
/*    HAL_PCD_EP_Receive                    Receive data                  */
/*    _ux_utility_semaphore_get             Get semaphore                 */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    STM32 Controller Driver                                             */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  01-31-2022     Chaoqiong Xiao           Modified comment(s), used ST  */
/*                                            HAL library to drive the    */
/*                                            controller,                 */
/*                                            resulting in version 6.1.10 */
/*                                                                        */
/**************************************************************************/
UINT  _ux_dcd_stm32_transfer_run(UX_DCD_STM32 *dcd_stm32, UX_SLAVE_TRANSFER *transfer_request)
{
UX_INTERRUPT_SAVE_AREA

UX_SLAVE_ENDPOINT       *endpoint;
UX_DCD_STM32_ED         *ed;
ULONG                   ed_status;


    /* Get the pointer to the logical endpoint from the transfer request.  */
    endpoint =  transfer_request -> ux_slave_transfer_request_endpoint;

    /* Get the physical endpoint address in the endpoint container.  */
    ed =  (UX_DCD_STM32_ED *) endpoint -> ux_slave_endpoint_ed;

    UX_DISABLE

    /* Get current ED status.  */
    ed_status = ed -> ux_dcd_stm32_ed_status;

    /* Invalid state.  */
    if (_ux_system_slave -> ux_system_slave_device.ux_slave_device_state == UX_DEVICE_RESET)
    {
        transfer_request -> ux_slave_transfer_request_completion_code = UX_TRANSFER_BUS_RESET;
        UX_RESTORE
        return(UX_STATE_EXIT);
    }

    /* ED stalled.  */
    if (ed_status & UX_DCD_STM32_ED_STATUS_STALLED)
    {
        transfer_request -> ux_slave_transfer_request_completion_code = UX_TRANSFER_STALLED;
        UX_RESTORE
        return(UX_STATE_NEXT);
    }

    /* ED transfer in progress.  */
    if (ed_status & UX_DCD_STM32_ED_STATUS_TRANSFER)
    {
        if (ed_status & UX_DCD_STM32_ED_STATUS_DONE)
        {

            /* Keep used, stall and task pending bits.  */
            ed -> ux_dcd_stm32_ed_status &= (UX_DCD_STM32_ED_STATUS_USED |
                                        UX_DCD_STM32_ED_STATUS_STALLED |
                                        UX_DCD_STM32_ED_STATUS_TASK_PENDING);
            UX_RESTORE
            return(UX_STATE_NEXT);
        }
        UX_RESTORE
        return(UX_STATE_WAIT);
    }


    /* Start transfer.  */
    ed -> ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_TRANSFER;

    /* D-cache maintenance around the OTG DMA hand-off.
     * ------------------------------------------------
     * In DMA mode (hpcd.Init.dma_enable = ENABLE) the Synopsys OTG core
     * fetches/pushes the transfer payload over the AHB bus directly
     * from/to transfer_request->ux_slave_transfer_request_data_pointer,
     * bypassing the CPU D-cache. The HAL's PCD DMA path does NOT perform
     * cache maintenance, so unless the application does it here the DMA
     * will see stale SRAM and the host will receive zeros / time out.
     *
     * This matters more for classes that copy payload into a USBX-owned
     * endpoint buffer via CPU memcpy (e.g. CDC-ACM, HID) than for
     * zero-copy classes like MSC where the app already owns the buffer
     * and does its own maintenance. Putting the maintenance here covers
     * BOTH paths in one place (MSC's own clean becomes a cheap no-op on
     * already-clean lines).
     *
     * Rules:
     *   - IN (device -> host, DATA_OUT from the app's perspective):
     *       CPU wrote last, DMA will read -> Clean before DMA start.
     *   - OUT (host -> device, DATA_IN from the app's perspective):
     *       DMA will write, CPU will read -> Invalidate before DMA
     *       start. (Invalidate-before is safe for aligned buffers on
     *       M33 when the CPU does not access the buffer during DMA.)
     *
     * The maintenance range is rounded down-at-start and up-at-end to
     * the 32-byte D-cache line so any partial cache line at either end
     * is flushed/invalidated too. The USBX byte pool is ALIGN_32BYTES
     * (app_usbx_device.c) so the rounding never touches memory outside
     * the pool. */
    {
        uint32_t dma_addr = (uint32_t)transfer_request->ux_slave_transfer_request_data_pointer;
        uint32_t dma_len  = (uint32_t)transfer_request->ux_slave_transfer_request_requested_length;
        if (dma_len != 0U && dma_addr != 0U)
        {
            uint32_t aligned_start = dma_addr & ~0x1FU;
            uint32_t aligned_end   = (dma_addr + dma_len + 31U) & ~0x1FU;
            uint32_t aligned_size  = aligned_end - aligned_start;
            extern DCACHE_HandleTypeDef hdcache1;

            if (transfer_request->ux_slave_transfer_request_phase == UX_TRANSFER_PHASE_DATA_OUT)
            {
                /* TX to host: CPU wrote last -> Clean. */
                HAL_DCACHE_CleanByAddr(&hdcache1, (uint32_t *)aligned_start, aligned_size);
            }
            else
            {
                /* RX from host: DMA will write -> Invalidate. */
                HAL_DCACHE_InvalidateByAddr(&hdcache1, (uint32_t *)aligned_start, aligned_size);
            }
        }
    }

    /* Check for transfer direction.  Is this a IN endpoint ? */
    if (transfer_request -> ux_slave_transfer_request_phase == UX_TRANSFER_PHASE_DATA_OUT)
    {

        /* Transmit data.  */
        HAL_PCD_EP_Transmit(dcd_stm32 -> pcd_handle,
                            endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
                            transfer_request->ux_slave_transfer_request_data_pointer,
                            transfer_request->ux_slave_transfer_request_requested_length);
    }
    else
    {

        /* We have a request for a SETUP or OUT Endpoint.  */
        /* Receive data.  */
        HAL_PCD_EP_Receive(dcd_stm32 -> pcd_handle,
                            endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
                            transfer_request->ux_slave_transfer_request_data_pointer,
                            transfer_request->ux_slave_transfer_request_requested_length);
    }

    /* Return to caller with WAIT.  */
    UX_RESTORE
    return(UX_STATE_WAIT);
}
#endif /* defined(UX_DEVICE_STANDALONE) */
