/*
   Copyright 2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __XHCI_DEVICE_H__
#define __XHCI_DEVICE_H__

#include <device/usb/xhci_ring.h>

typedef struct xhci_device_s
{
    uint8_t              port_id; // 1-based
    uint8_t              slot_id; // slots index in DCBAA
    uint8_t              speed;   // Speed
    uint8_t              using_64byte_ctx; // 64-byte context size
    void                *input_ctx;
    addr_t               dma_input_ctx;
    xhci_transfer_ring_t *ctrl_ep_ring;
} xhci_device_t;

PUBLIC status_t init_xhci_device_struct(
    xhci_device_t *device,
    uint8_t port_id,
    uint8_t slot_id,
    uint8_t speed,
    uint16_t using_64byte_ctx);

PUBLIC xhci_transfer_ring_t* xhci_get_device_ctrl_ep_transfer_ring(xhci_device_t *device);

#endif