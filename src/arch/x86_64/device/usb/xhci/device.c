/*
   Copyright 2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/usb/xhci.h>
#include <device/usb/xhci_device.h>
#include <device/usb/xhci_device_ctx.h>
#include <device/usb/xhci_ring.h>
#include <mem/mem.h>                    // pmalloc
#include <device/usb/xhci_mem.h>
#include <device/usb/xhci_trb.h>
#include <std/string.h>                 // memset

#include <log.h>


PUBLIC status_t init_xhci_device_struct(
    xhci_device_t *device,
    uint8_t port_id,
    uint8_t slot_id,
    uint8_t speed,
    uint16_t using_64byte_ctx)
{
    device->port_id          = port_id;
    device->slot_id          = slot_id;
    device->speed            = speed;
    device->using_64byte_ctx = using_64byte_ctx;
    status_t status;
    size_t input_ctx_size = sizeof(xhci_input_ctx32_t);
    if (using_64byte_ctx)
    {
        input_ctx_size = sizeof(xhci_input_ctx64_t);
    }
    status = pmalloc(
        input_ctx_size,
        XHCI_INPUT_CONTROL_CONTEXT_ALIGNMENT,
        XHCI_INPUT_CONTROL_CONTEXT_BOUNDARY,
        &device->dma_input_ctx);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc input ctx.\n");
        return status;
    }
    device->input_ctx = KADDR_P2V(device->dma_input_ctx);
    memset(device->input_ctx,0,input_ctx_size);

    // allocate ep ring
    xhci_transfer_ring_t *ctrl_ep_ring;
    status = pmalloc(
        sizeof(xhci_transfer_ring_t),
        XHCI_TRANSFER_RING_SEGMENTS_ALIGNMENT,
        XHCI_TRANSFER_RING_SEGMENTS_BOUNDARY,
        &ctrl_ep_ring);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc ctrl_ep_ring.\n");
        return status;
    }
    ctrl_ep_ring = KADDR_P2V(ctrl_ep_ring);
    memset(ctrl_ep_ring,0,sizeof(xhci_transfer_ring_t));
    device->ctrl_ep_ring = ctrl_ep_ring;
    ctrl_ep_ring->ring.trb_count = XHCI_TRANSFER_RING_COUNT;
    ctrl_ep_ring->ring.cycle_bit = 1;
    ctrl_ep_ring->ring.dequeue   = 0;
    ctrl_ep_ring->ring.enqueue   = 0;
    ctrl_ep_ring->doorbell_id    = slot_id;

    xhci_trb_t *transfer_ring;
    status = pmalloc(
        sizeof(ctrl_ep_ring->ring.trbs[0]) * XHCI_TRANSFER_RING_COUNT,
        XHCI_TRANSFER_RING_SEGMENTS_ALIGNMENT,
        XHCI_TRANSFER_RING_SEGMENTS_BOUNDARY,
        &transfer_ring);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc transfer ring.\n");
        return status;
    }
    ctrl_ep_ring->ring.trbs_paddr = (phy_addr_t)transfer_ring;
    ctrl_ep_ring->ring.trbs       = KADDR_P2V(transfer_ring);
    memset(
        ctrl_ep_ring->ring.trbs,
        0,
        sizeof(ctrl_ep_ring->ring.trbs[0]) * XHCI_TRANSFER_RING_COUNT);

    // Make Linking trb
    uint8_t cycle_bit = ctrl_ep_ring->ring.cycle_bit;

    ctrl_ep_ring->ring.trbs[ctrl_ep_ring->ring.trb_count - 1].addr  =
            (uint64_t)transfer_ring;

    ctrl_ep_ring->ring.trbs[ctrl_ep_ring->ring.trb_count - 1].flags =
            SET_FIELD(TRB_3_TC_BIT | cycle_bit,TRB_3_TYPE,TRB_TYPE_LINK);

    return K_SUCCESS;
}

PUBLIC xhci_transfer_ring_t* xhci_get_device_ctrl_ep_transfer_ring(
    xhci_device_t *device)
{
    return device->ctrl_ep_ring;
}

