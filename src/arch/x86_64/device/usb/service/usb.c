// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2009-2013 Kevin O'Connor <kevin@koconnor.net>
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/usb/hid.h>  // USB_INTERFACE_SUBCLASS_BOOT
#include <device/usb/xhci.h> // xhci_setup
#include <mem/allocator.h>   // kmalloc
#include <mem/page.h>        // VIRT_TO_PHYS,PHYS_TO_VIRT
#include <service.h>         // previous prototype for 'usb_main'
#include <std/string.h>      // memset
#include <task/task.h>       // task_msleep,task_start

PRIVATE void usb_setup(usb_hub_set_t *hub_set)
{
    xhci_setup(hub_set);
    return;
}

PRIVATE int speed_to_ctrl_size(uint8_t speed)
{
    switch (speed)
    {
        case USB_FULLSPEED:
        case USB_LOWSPEED:
            return 8;
        case USB_HIGHSPEED:
            return 64;
        case USB_SUPERSPEED:
            return 512;
    }
    return -1;
};

PUBLIC usb_pipe_t *usb_realloc_pipe(
    usb_device_t              *usb_dev,
    usb_pipe_t                *pipe,
    usb_endpoint_descriptor_t *epdesc
)
{
    return xhci_realloc_pipe(usb_dev, pipe, epdesc);
}

PRIVATE int usb_send_pipe(
    usb_pipe_t *pipe_fl,
    int         dir,
    const void *cmd,
    void       *data,
    int         data_size
)
{
    return xhci_send_pipe(pipe_fl, dir, cmd, data, data_size);
}

PUBLIC usb_pipe_t *
usb_alloc_pipe(usb_device_t *usb_dev, usb_endpoint_descriptor_t *epdesc)
{
    return usb_realloc_pipe(usb_dev, NULL, epdesc);
}

PUBLIC void usb_free_pipe(usb_device_t *usb_dev, usb_pipe_t *pipe)
{
    if (pipe == NULL)
    {
        return;
    }
    usb_realloc_pipe(usb_dev, pipe, NULL);
    return;
}

PUBLIC int usb_send_default_control(
    usb_pipe_t               *pipe,
    const usb_ctrl_request_t *req,
    void                     *data
)
{
    return usb_send_pipe(
        pipe, req->bRequestType & USB_DIR_IN, req, data, req->wLength
    );
}

PUBLIC void usb_add_freelist(usb_pipe_t *pipe)
{
    if (pipe == NULL) return;
    usb_t *cntl    = pipe->ctrl;
    pipe->freenext = cntl->freelist;
    cntl->freelist = pipe;
}

PUBLIC void usb_desc2pipe(
    usb_pipe_t                *pipe,
    usb_device_t              *usb_dev,
    usb_endpoint_descriptor_t *epdesc
)
{
    pipe->ctrl       = usb_dev->hub->ctrl;
    pipe->type       = usb_dev->hub->ctrl->type;
    pipe->ep         = epdesc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
    pipe->dev_addr   = usb_dev->dev_addr;
    pipe->speed      = usb_dev->speed;
    pipe->max_packet = epdesc->wMaxPacketSize;
    pipe->ep_type    = epdesc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
    return;
}

PUBLIC int
usb_get_period(usb_device_t *usb_dev, usb_endpoint_descriptor_t *epdesc)
{
    uint8_t period = epdesc->bInterval;
    int     ret    = 0;
    if (usb_dev->speed != USB_HIGHSPEED)
    {
        if (period >> 8)
        {
            period >>= 8;
            ret += 8;
        }
        if (period >> 4)
        {
            period >>= 4;
            ret += 4;
        }
        if (period >> 2)
        {
            period >>= 2;
            ret += 2;
        }
        if (period >> 1)
        {
            ret += 1;
        }
        return (period <= 0) ? 0 : ret;
    }
    return (period <= 4) ? 0 : period - 4;
}

// Find the first endpoint of a given type in an interface description.
PUBLIC usb_endpoint_descriptor_t *
usb_find_desc(usb_device_t *usb_dev, int type, int dir)
{
    usb_endpoint_descriptor_t *epdesc = (void *)&usb_dev->iface[1];
    while (1)
    {
        if ((addr_t)epdesc >= (addr_t)usb_dev->iface + usb_dev->imax)
        {
            return NULL;
        }
        if (epdesc->bDescriptorType == USB_DT_INTERFACE)
        {
            return NULL;
        }
        if (epdesc->bDescriptorType == USB_DT_ENDPOINT &&
            (epdesc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == dir &&
            (epdesc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == type)
        {
            return epdesc;
        }
        epdesc = (void *)((uint8_t *)epdesc + epdesc->bLength);
    }
}

PRIVATE int get_device_info8(usb_pipe_t *pipe, usb_device_descriptor_t *dinfo)
{
    usb_ctrl_request_t req;
    req.bRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest     = USB_REQ_GET_DESCRIPTOR;
    req.wValue       = USB_DT_DEVICE << 8;
    req.wIndex       = 0;
    req.wLength      = 8;
    return usb_send_default_control(pipe, &req, dinfo);
}

PRIVATE usb_config_descriptor_t *get_device_config(usb_pipe_t *pipe)
{
    usb_config_descriptor_t config;
    usb_ctrl_request_t      req;
    req.bRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest     = USB_REQ_GET_DESCRIPTOR;
    req.wValue       = USB_DT_CONFIG << 8;
    req.wIndex       = 0;
    req.wLength      = sizeof(config);
    int ret          = usb_send_default_control(pipe, &req, &config);
    if (ret != 0)
    {
        return NULL;
    }
    usb_config_descriptor_t *p_config;
    status_t status = kmalloc(config.wTotalLength, 16, 0, &p_config);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Failed to alloc config.\n");
        return NULL;
    }
    req.wLength = config.wTotalLength;
    ret         = usb_send_default_control(pipe, &req, p_config);
    if (ret != 0 || p_config->wTotalLength != config.wTotalLength)
    {
        kfree(p_config);
        return NULL;
    }
    return p_config;
}

PRIVATE int set_configuration(usb_pipe_t *pipe, uint16_t val)
{
    usb_ctrl_request_t req;
    req.bRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest     = USB_REQ_SET_CONFIGURATION;
    req.wValue       = val;
    req.wIndex       = 0;
    req.wLength      = 0;
    return usb_send_default_control(pipe, &req, NULL);
}

PRIVATE int usb_set_address(usb_device_t *usb_dev)
{
    usb_t *ctrl = usb_dev->hub->ctrl;
    if (ctrl->max_addr >= USB_MAXADDR)
    {
        return -1;
    }
    task_msleep(USB_TIME_RSTRCY);

    // Create a pipe for the default address.
    usb_endpoint_descriptor_t epdesc;
    memset(&epdesc, 0, sizeof(epdesc));
    epdesc.wMaxPacketSize = speed_to_ctrl_size(usb_dev->speed);
    epdesc.bmAttributes   = USB_ENDPOINT_XFER_CONTROL;

    usb_dev->defpipe = usb_alloc_pipe(usb_dev, &epdesc);
    if (usb_dev->defpipe == NULL)
    {
        PR_LOG(LOG_ERROR, "Failed to alloc pipe.\n");
        return -1;
    }
    // Send address command
    usb_ctrl_request_t req;
    req.bRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest     = USB_REQ_SET_ADDRESS;
    req.wValue       = ctrl->max_addr + 1;
    req.wIndex       = 0;
    req.wLength      = 0;
    int ret          = usb_send_default_control(usb_dev->defpipe, &req, NULL);

    if (ret != 0)
    {
        PR_LOG(LOG_ERROR, "failed to send defaule control.\n");
        usb_free_pipe(usb_dev, usb_dev->defpipe);
        return ret;
    }
    task_msleep(USB_TIME_SETADDR_RECOVERY);

    ctrl->max_addr++;
    usb_dev->dev_addr = ctrl->max_addr;
    usb_dev->defpipe  = usb_realloc_pipe(usb_dev, usb_dev->defpipe, &epdesc);
    if (usb_dev->defpipe == NULL)
    {
        return -1;
    }
    return 0;
}

PRIVATE int configure_usb_device(usb_device_t *usb_dev)
{
    // Set the max packet size for endpoint 0 of this device.
    usb_device_descriptor_t dinfo;
    int                     ret = get_device_info8(usb_dev->defpipe, &dinfo);
    if (ret != 0)
    {
        PR_LOG(LOG_ERROR, "Failed to get device info.\n");
        return 0;
    }
    uint16_t max_packet = dinfo.bMaxPacketSize0;
    if (dinfo.bcdUSB >= 0x0300)
    {
        max_packet = 1 << dinfo.bMaxPacketSize0;
    }
    // PR_LOG(
    //     LOG_DEBUG,
    //     "device rev=%04x cls=%02x sub=%02x proto=%02x size=%d\n",
    //     dinfo.bcdUSB,
    //     dinfo.bDeviceClass,
    //     dinfo.bDeviceSubClass,
    //     dinfo.bDeviceProtocol,
    //     max_packet
    // );

    if (max_packet < 8)
    {
        return 0;
    }
    usb_endpoint_descriptor_t epdesc;
    memset(&epdesc, 0, sizeof(epdesc));
    epdesc.wMaxPacketSize = max_packet;
    epdesc.bmAttributes   = USB_ENDPOINT_XFER_CONTROL;
    usb_dev->defpipe = usb_realloc_pipe(usb_dev, usb_dev->defpipe, &epdesc);
    if (usb_dev->defpipe == NULL)
    {
        PR_LOG(LOG_ERROR, "failed to realloc pipe.\n");
        return -1;
    }
    usb_config_descriptor_t *config = get_device_config(usb_dev->defpipe);
    if (config == NULL)
    {
        PR_LOG(LOG_ERROR, "Failed to get config.\n");
        return -1;
    }

    // Determine if a driver exists for this device - only look at the
    // interfaces of the first configuration.
    int      num_iface  = config->bNumInterfaces;
    uint8_t *config_end = (uint8_t *)config + config->wTotalLength;
    usb_interface_descriptor_t *iface = (void *)(&config[1]);
    while (1)
    {
        if (!num_iface || (uint8_t *)iface + iface->bLength > config_end)
        {
            // Not a supported device.
            goto fail;
        }
        if (iface->bDescriptorType == USB_DT_INTERFACE)
        {
            num_iface--;
            if (iface->bInterfaceClass == USB_CLASS_HUB)
            {
                break;
            }
            if (iface->bInterfaceClass == USB_CLASS_MASS_STORAGE)
            {
                if (iface->bInterfaceProtocol == US_PR_BULK)
                {
                    break;
                }
                if (iface->bInterfaceProtocol == US_PR_UAS)
                {
                    break;
                }
            }
            if (iface->bInterfaceClass == USB_CLASS_HID &&
                iface->bInterfaceSubClass == USB_INTERFACE_SUBCLASS_BOOT)
            {
                break;
            }
        }
        iface = (void *)((uint8_t *)iface + iface->bLength);
    }
    // Set the configuration
    ret = set_configuration(usb_dev->defpipe, config->bConfigurationValue);
    if (ret != 0)
    {
        goto fail;
    }

    // Configure driver
    usb_dev->config = config;
    usb_dev->iface  = iface;
    usb_dev->imax = (uint8_t *)config + config->wTotalLength - (uint8_t *)iface;
    if (iface->bInterfaceClass == USB_CLASS_HUB)
    {
        PR_LOG(LOG_DEBUG, "USB HUB.\n");
    }
    else if (iface->bInterfaceClass == USB_CLASS_MASS_STORAGE)
    {
        if (iface->bInterfaceProtocol == US_PR_BULK)
        {
            PR_LOG(LOG_DEBUG, "USB Mass storge (bluk).\n");
        }
        if (iface->bInterfaceProtocol == US_PR_UAS)
        {
            PR_LOG(LOG_DEBUG, "USB Mass storge (uas).\n");
        }
    }
    else
    {
        usb_hid_setup(usb_dev);
    }
    kfree(config);
    return 1;
fail:
    kfree(config);
    return -1;
}

PRIVATE int usb_hub_port_setup(usb_device_t *usb_dev)
{
    usb_hub_t *hub  = usb_dev->hub;
    uint32_t   port = usb_dev->port;

    int ret = hub->op->reset(hub, port);
    if (ret < 0)
    {
        PR_LOG(LOG_ERROR, "Port reset fail.ret=%d\n", ret);
        goto fail;
    }
    usb_dev->speed = ret;
    ret            = usb_set_address(usb_dev);
    if (ret < 0)
    {
        PR_LOG(LOG_ERROR, "usb set address failed.\n");
        hub->op->disconnect(hub, port);
        goto fail;
    }
    // PR_LOG(LOG_INFO, "usb set address success.\n");

    // PR_LOG(LOG_INFO, "Configure the device.\n");
    int count = configure_usb_device(usb_dev);
    if (count < 0)
    {
        goto fail;
    }
    return 0;
    /// TODO:
    if (count == 0)
    {
        hub->op->disconnect(hub, port);
    }
fail:
    kfree(usb_dev);
    return ret;
}

PRIVATE void xhci_event(usb_hub_set_t *hub_set)
{
    size_t i;
    // Reset all ports
    for (i = 0; i < hub_set->count; i++)
    {
        usb_hub_t *hub = &hub_set->hubs[i];
        ;
        uint16_t port;
        for (port = 0; port < hub->portcount; port++)
        {
            if (hub->op->detect(hub, port))
            {
                xhci_t  *xhci   = CONTAINER_OF(xhci_t, usb, hub->ctrl);
                uint32_t portsc = xhci_read_opt(xhci, XHCI_OPT_PORTSC(port));
                if ((portsc & XHCI_PORTSC_CSC))
                {
                    usb_port_connecton_event_t pc_evt;
                    pc_evt.port       = port;
                    pc_evt.is_connect = (portsc & XHCI_PORTSC_CCS) ? 1 : 0;
                    fifo_write(&xhci->port_evts, &pc_evt);
                }
                uint32_t pclear = portsc;
                pclear &= ~(XHCI_PORTSC_PED | XHCI_PORTSC_PR);
                pclear &= ~(XHCI_PORTSC_PLS_MASK << XHCI_PORTSC_PLS_SHIFT);
                pclear |= (1 << XHCI_PORTSC_PLS_SHIFT);
                xhci_write_opt(xhci, XHCI_OPT_PORTSC(port), pclear);
            }
        }
    }

    while (1)
    {
        for (i = 0; i < hub_set->count; i++)
        {
            usb_hub_t *hub = &hub_set->hubs[i];
            ;
            xhci_t *xhci = CONTAINER_OF(xhci_t, usb, hub->ctrl);
            xhci_process_events(xhci);
        }
    }
}

PUBLIC void usb_main(void)
{
    usb_hub_set_t xhci_hubs;
    usb_setup(&xhci_hubs);
    PR_LOG(
        LOG_DEBUG, "Hub set at %p,count=%d.\n", xhci_hubs.hubs, xhci_hubs.count
    );
    task_start(
        "XHCI", DEFAULT_PRIORITY, 4096, xhci_event, (uint64_t)&xhci_hubs
    );
    task_yield();

    while (1)
    {
        size_t i;
        for (i = 0; i < xhci_hubs.count; i++)
        {
            usb_hub_t *hub = &xhci_hubs.hubs[i];
            ;
            xhci_t *xhci = CONTAINER_OF(xhci_t, usb, hub->ctrl);
            if (fifo_empty(&xhci->port_evts))
            {
                continue;
            }
            usb_port_connecton_event_t pc_evt;
            fifo_read(&xhci->port_evts, &pc_evt);
            pr_msg("=====================================================\n");
            if (!pc_evt.is_connect)
            {
                PR_LOG(LOG_INFO, "Port %d disconnected.\n", pc_evt.port + 1);
            }

            if (pc_evt.is_connect)
            {
                usb_device_t *usb_dev;
                status_t      status;
                status = kmalloc(sizeof(*usb_dev), 16, 0, &usb_dev);
                if (ERROR(status))
                {
                    PR_LOG(LOG_ERROR, "Failed to alloc usb dev.\n");
                    continue;
                }
                memset(usb_dev, 0, sizeof(*usb_dev));

                usb_dev->hub  = hub;
                usb_dev->port = pc_evt.port;
                int ret       = usb_hub_port_setup(usb_dev);
                if (ret < 0)
                {
                    PR_LOG(
                        LOG_ERROR, "setup port: %d failed.\n", pc_evt.port + 1
                    );
                }
                else
                {
                    PR_LOG(
                        LOG_INFO, "setup successful: %d.\n", pc_evt.port + 1
                    );
                }
            }
            else
            {
                /// TODO: process port disconnect event
                hub->op->disconnect(hub, pc_evt.port);
            }
            pr_msg("=====================================================\n");
        }
        task_yield();
    }
    return;
}