/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/usb/xhci.h>
#include <device/usb/xhci_device_ctx.h>

PUBLIC xhci_input_ctrl_ctx32_t* xhci_device_get_input_ctrl_ctx(
    xhci_device_t *device)
{
    if (device->using_64byte_ctx)
    {
        xhci_input_ctx64_t *input_ctx = (xhci_input_ctx64_t*)device->input_ctx;
        return (xhci_input_ctrl_ctx32_t*)&input_ctx->control_context;
    }
    xhci_input_ctx32_t *input_ctx = (xhci_input_ctx32_t*)device->input_ctx;
    return &input_ctx->control_context;
}
PUBLIC xhci_slot_ctx32_t* xhci_device_get_input_slot_ctx(xhci_device_t *device)
{
    if (device->using_64byte_ctx)
    {
        xhci_input_ctx64_t *input_ctx = (xhci_input_ctx64_t*)device->input_ctx;
        return (xhci_slot_ctx32_t*)&input_ctx->device_context.slot_context;
    }
    xhci_input_ctx32_t *input_ctx = (xhci_input_ctx32_t*)device->input_ctx;
    return &input_ctx->device_context.slot_context;
}

PUBLIC xhci_endpoint_ctx32_t* xhci_device_get_input_ctrl_ep_ctx(
    xhci_device_t *device)
{
    if (device->using_64byte_ctx)
    {
        xhci_input_ctx64_t *input_ctx = (xhci_input_ctx64_t*)device->input_ctx;
        return  (xhci_endpoint_ctx32_t*)
                &input_ctx->device_context.control_ep_context;
    }
    xhci_input_ctx32_t *input_ctx = (xhci_input_ctx32_t*)device->input_ctx;
    return &input_ctx->device_context.control_ep_context;
}
