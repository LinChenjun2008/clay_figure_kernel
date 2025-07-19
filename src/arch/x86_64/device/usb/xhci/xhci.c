// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2013 Gerd Hoffmann <kraxel@redhat.com>
 * Copyright (C) 2009-2013 Kevin O'Connor <kevin@koconnor.net>
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/pci.h>      // pci functions
#include <device/usb/xhci.h> // xhci struct
#include <mem/allocator.h>   // pmalloc,pfree
#include <mem/page.h>        // KADDR_V2P,KADDR_P2V
#include <std/string.h>      // memset
#include <task/task.h>       // task_start

PRIVATE int speed_from_xhci(uint8_t speed)
{
    switch (speed)
    {
        case 1:
            return USB_FULLSPEED;
        case 2:
            return USB_LOWSPEED;
        case 3:
            return USB_HIGHSPEED;
        case 4:
            return USB_SUPERSPEED;
        default:
            return -1;
    }
    return -1;
};

PRIVATE int speed_to_xhci(uint8_t speed)
{
    switch (speed)
    {
        case USB_FULLSPEED:
            return 1;
        case USB_LOWSPEED:
            return 2;
        case USB_HIGHSPEED:
            return 3;
        case USB_SUPERSPEED:
            return 4;
        default:
            return 0;
    }
    return 0;
};


PRIVATE void switch_to_xhci(pci_device_t *xhci_dev)
{
    pci_device_t *ehci = pci_dev_match(0x0c, 0x03, 0x20, 0);
    if (ehci == NULL || (pci_dev_config_read(ehci, 0) & 0xffff) != 0x8086)
    {
        // ehci not exist
        return;
    }

    uint32_t superspeed_ports = pci_dev_config_read(xhci_dev, 0xdc); // USB3PRM
    pci_dev_config_write(xhci_dev, 0xd8, superspeed_ports); // USB3_PSSEN
    uint32_t ehci2xhci_ports = pci_dev_config_read(xhci_dev, 0xd4); // XUSB2PRM
    pci_dev_config_write(xhci_dev, 0xd0, ehci2xhci_ports);          // XUSB2PR
    return;
}

PRIVATE status_t xhci_controller_setup(void *mmio_base, xhci_t **xhci_addr)
{
    xhci_t  *xhci;
    status_t status;
    status = pmalloc(sizeof(*xhci), 0, 0, &xhci);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Alloc memory for xhci failed.\n");
        return status;
    }
    xhci = KADDR_P2V(xhci);
    memset(xhci, 0, sizeof(*xhci));
    xhci->mmio_base = mmio_base;
    xhci->cap_regs  = xhci->mmio_base;

    size_t caplength = xhci_read_cap(xhci, XHCI_CAP_CAPLENGTH);
    xhci->opt_regs   = xhci->cap_regs + GET_FIELD(caplength, CAPLENGTH);

    xhci->run_regs      = xhci->cap_regs + xhci_read_cap(xhci, XHCI_CAP_RSTOFF);
    xhci->doorbell_regs = xhci->cap_regs + xhci_read_cap(xhci, XHCI_CAP_DBOFF);

    uint32_t hcs1 = xhci_read_cap(xhci, XHCI_CAP_HCSPARAM1);
    uint32_t hcc1 = xhci_read_cap(xhci, XHCI_CAP_HCCPARAM1);

    xhci->ports     = GET_FIELD(hcs1, HCSP1_MAX_PORTS);
    xhci->slots     = GET_FIELD(hcs1, HCSP1_MAX_SLOTS);
    xhci->xecp      = GET_FIELD(hcc1, HCCP1_XECP) << 2;
    xhci->context64 = GET_FIELD(hcc1, HCCP1_CSZ) ? 1 : 0;
    PR_LOG(
        LOG_INFO,
        "XHCI: mmio base: %p, %d ports, %d slots, %d bytes context.\n",
        mmio_base,
        xhci->ports,
        xhci->slots,
        xhci->context64 ? 64 : 32
    );

    if (xhci->xecp != 0)
    {
        uint32_t offset;
        addr_t   addr = (addr_t)xhci->cap_regs + xhci->xecp;
        do
        {
            xhci_xcap_regs_t *xcap = (xhci_xcap_regs_t *)addr;
            uint32_t          cap  = xcap->cap;
            uint32_t          name;
            uint32_t          ports;
            switch (GET_FIELD(cap, XECP_CAP_ID))
            {
                case 0x02:
                    name          = xcap->data[0];
                    ports         = xcap->data[1];
                    uint8_t major = GET_FIELD(cap, XECP_SUP_MAJOR);
                    uint8_t minor = GET_FIELD(cap, XECP_SUP_MINOR);
                    uint8_t count = (ports >> 8) & 0xff;
                    uint8_t start = (ports >> 0) & 0xff;
                    PR_LOG(
                        LOG_INFO,
                        "xHCI protocol %c%c%c%c"
                        " %x.%02x ,%d ports (offset %d), def %x\n",
                        (name >> 0) & 0xff,
                        (name >> 8) & 0xff,
                        (name >> 16) & 0xff,
                        (name >> 24) & 0xff,
                        major,
                        minor,
                        count,
                        start,
                        ports >> 16
                    );
                    if (strncmp((const char *)&name, "USB ", 4) == 0)
                    {
                        if (major == 2)
                        {
                            xhci->usb2.start = start;
                            xhci->usb2.count = count;
                        }
                        if (major == 3)
                        {
                            xhci->usb3.start = start;
                            xhci->usb3.count = count;
                        }
                    }
                    break;
                default:
                    PR_LOG(
                        LOG_INFO,
                        "XHCI XECP %#02x.\n",
                        GET_FIELD(cap, XECP_CAP_ID)
                    );
                    break;
            }
            offset = GET_FIELD(cap, XECP_NEXT_POINT) << 2;
            addr += offset;
        } while (offset > 0);
    }
    uint32_t pagesize = xhci_read_opt(xhci, XHCI_OPT_PAGESIZE);
    if (XHCI_PAGE_SIZE != pagesize << 12)
    {
        PR_LOG(
            LOG_ERROR, "XHCI Driver not support pagesize: %d.\n", pagesize << 12
        );
        pfree(KADDR_V2P(xhci));
        *xhci_addr = NULL;
        return K_ERROR;
    }
    *xhci_addr = xhci;
    return K_SUCCESS;
}

PRIVATE status_t xhci_halt(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci, XHCI_OPT_USBCMD);
    if (!(usbcmd & XHCI_CMD_RS))
    {
        return K_SUCCESS;
    }
    usbcmd &= ~XHCI_CMD_RS;
    xhci_write_opt(xhci, XHCI_OPT_USBCMD, usbcmd);
    int timeout = 200;
    while ((xhci_read_opt(xhci, XHCI_OPT_USBSTS) & XHCI_STS_HCH) == 0)
    {
        if (--timeout == 0)
        {
            PR_LOG(LOG_ERROR, "Host controller halt timeout.\n");
            return K_TIMEOUT;
        }
        task_msleep(1);
    };
    return K_SUCCESS;
}

PRIVATE status_t xhci_reset(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci, XHCI_OPT_USBCMD);
    usbcmd |= XHCI_CMD_HCRST;
    xhci_write_opt(xhci, XHCI_OPT_USBCMD, usbcmd);

    int timeout = 1000;
    while ((xhci_read_opt(xhci, XHCI_OPT_USBCMD) & XHCI_CMD_HCRST) ||
           (xhci_read_opt(xhci, XHCI_OPT_USBSTS) & XHCI_STS_CNR))
    {
        if (--timeout == 0)
        {
            PR_LOG(LOG_ERROR, "Host controller reset timeout.\n");
            return K_TIMEOUT;
        }
        task_msleep(1);
    }
    task_msleep(50);
    if (xhci_read_opt(xhci, XHCI_OPT_USBCMD) != 0) return K_ERROR;
    if (xhci_read_opt(xhci, XHCI_OPT_DNCTRL) != 0) return K_ERROR;
    if (xhci_read_opt(xhci, XHCI_OPT_CRCR_LO) != 0) return K_ERROR;
    if (xhci_read_opt(xhci, XHCI_OPT_CRCR_HI) != 0) return K_ERROR;
    if (xhci_read_opt(xhci, XHCI_OPT_DCBAAP_LO) != 0) return K_ERROR;
    if (xhci_read_opt(xhci, XHCI_OPT_DCBAAP_HI) != 0) return K_ERROR;
    if (xhci_read_opt(xhci, XHCI_OPT_CONFIG) != 0) return K_ERROR;
    return K_SUCCESS;
}

PRIVATE status_t xhci_run(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci, XHCI_OPT_USBCMD);
    usbcmd |= XHCI_CMD_RS;
    xhci_write_opt(xhci, XHCI_OPT_USBCMD, usbcmd);
    int timeout = 1000;
    while ((xhci_read_opt(xhci, XHCI_OPT_USBSTS) & XHCI_STS_HCH) == 1)
    {
        if (--timeout == 0)
        {
            PR_LOG(LOG_ERROR, "xhci start timeout.\n");
            return K_TIMEOUT;
        }
        task_msleep(1);
    };

    // Verify CNR bit clear
    uint32_t usbsts;
    usbsts = xhci_read_opt(xhci, XHCI_OPT_USBSTS);
    if (usbsts & XHCI_STS_CNR)
    {
        // Control not redy.
        return K_ERROR;
    }
    return K_SUCCESS;
}

// XHCI Hub opt

PRIVATE int xhci_hub_detect(usb_hub_t *hub, uint32_t port)
{
    xhci_t  *xhci = CONTAINER_OF(xhci_t, usb, hub->ctrl);
    uint32_t portsc;
    portsc = xhci_read_opt(xhci, XHCI_OPT_PORTSC(port));
    return (portsc & XHCI_PORTSC_CCS) ? 1 : 0;
}

PRIVATE int xhci_hub_reset(usb_hub_t *hub, uint32_t port)
{
    xhci_t  *xhci = CONTAINER_OF(xhci_t, usb, hub->ctrl);
    uint32_t portsc;
    portsc = xhci_read_opt(xhci, XHCI_OPT_PORTSC(port));
    if ((portsc & XHCI_PORTSC_CCS) == 0)
    {
        PR_LOG(LOG_ERROR, "Port not connected before reset.\n");
        return -1;
    }
    switch (GET_FIELD(portsc, XHCI_PORTSC_PLS))
    {
        case PLS_U0:
            // A USB3 port - controller automatically performs reset
            break;
        case PLS_POLLING:
            // A USB2 port - perform device reset
            xhci_write_opt(
                xhci, XHCI_OPT_PORTSC(port), portsc | XHCI_PORTSC_PR
            );
            break;
        default:
            PR_LOG(
                LOG_ERROR,
                "Unknow PLS: %d.\n",
                GET_FIELD(portsc, XHCI_PORTSC_PLS)
            );
            return -1;
            break;
    }
    int timeout = 100;
    while (timeout > 0)
    {
        portsc = xhci_read_opt(xhci, XHCI_OPT_PORTSC(port));
        if ((portsc & XHCI_PORTSC_CCS) == 0)
        {
            PR_LOG(LOG_ERROR, "Port not connected.\n");
            return -1;
        }
        if (portsc & XHCI_PORTSC_PED)
        {
            // reset complete
            break;
        }
        if (timeout == 0)
        {
            PR_LOG(LOG_ERROR, "Port reset timeout.\n");
            return K_TIMEOUT;
        }
        timeout--;
        task_msleep(1);
    }
    return speed_from_xhci(GET_FIELD(portsc, XHCI_PORTSC_SPEED));
}

PRIVATE int xhci_hub_portmap(usb_hub_t *hub, uint32_t port)
{
    xhci_t  *xhci  = CONTAINER_OF(xhci_t, usb, hub->ctrl);
    uint32_t pport = port + 1;
    if (port + 1 >= xhci->usb3.start &&
        port + 1 < xhci->usb3.start + xhci->usb3.count)
    {
        pport = port + 2 - xhci->usb3.start;
    }
    if (port + 1 >= xhci->usb2.start &&
        port + 1 < xhci->usb2.start + xhci->usb2.count)
    {
        pport = port + 2 - xhci->usb2.start;
    }
    return pport;
}

PRIVATE int xhci_hub_disconnect(usb_hub_t *hub, uint32_t port)
{
    // turn the port power off
    (void)hub;
    (void)port;
    return 0;
}

PRIVATE usb_hub_op_t xhci_hub_ops = {
    xhci_hub_detect,
    xhci_hub_reset,
    xhci_hub_portmap,
    xhci_hub_disconnect,
};

PRIVATE status_t configure_xhci(xhci_t *xhci, usb_hub_t *hub)
{
    PR_LOG(LOG_INFO, "Configure xhci: %p.\n", xhci);
    status_t status;
    void    *devs = NULL, *eseg = NULL, *evts = NULL, *cmds = NULL;
    void    *cmds_evt = NULL, *port_evt = NULL, *xfer_evt = NULL;

    size_t pc_evt_size = XHCI_RING_ITEMS * sizeof(usb_port_connecton_event_t);
    status = pmalloc(sizeof(*xhci->devs) * (xhci->slots + 1), 64, 0, &devs);
    if (ERROR(status))
    {
        goto fail;
    }

    status = pmalloc(sizeof(*xhci->eseg), 64, 0, &eseg);
    if (ERROR(status))
    {
        goto fail;
    }

    status = pmalloc(XHCI_RING_SIZE, 64, 65536, &evts);
    if (ERROR(status))
    {
        goto fail;
    }

    status = pmalloc(XHCI_RING_SIZE, 64, 65536, &cmds);
    if (ERROR(status))
    {
        goto fail;
    }

    status = pmalloc(XHCI_RING_SIZE, 0, 0, &cmds_evt);
    if (ERROR(status))
    {
        goto fail;
    }

    status = pmalloc(pc_evt_size, 0, 0, &port_evt);
    if (ERROR(status))
    {
        goto fail;
    }
    status = pmalloc(XHCI_RING_SIZE, 0, 0, &xfer_evt);
    if (ERROR(status))
    {
        goto fail;
    }
    xhci->devs      = KADDR_P2V(devs);
    xhci->eseg      = KADDR_P2V(eseg);
    xhci->evts.trbs = KADDR_P2V(evts);
    xhci->cmds.trbs = KADDR_P2V(cmds);

    memset(xhci->devs, 0, sizeof(*xhci->devs) * (xhci->slots + 1));
    memset(xhci->eseg, 0, sizeof(*xhci->eseg));
    memset(xhci->evts.trbs, 0, XHCI_RING_SIZE);
    memset(xhci->cmds.trbs, 0, XHCI_RING_SIZE);

    memset(KADDR_P2V(cmds_evt), 0, XHCI_RING_SIZE);
    memset(KADDR_P2V(port_evt), 0, pc_evt_size);
    memset(KADDR_P2V(xfer_evt), 0, XHCI_RING_SIZE);

    init_fifo(
        &xhci->cmds_evts,
        KADDR_P2V(cmds_evt),
        sizeof(xhci_trb_t),
        XHCI_RING_ITEMS
    );

    init_fifo(
        &xhci->port_evts,
        KADDR_P2V(port_evt),
        sizeof(usb_port_connecton_event_t),
        XHCI_RING_ITEMS
    );

    init_fifo(
        &xhci->xfer_evts,
        KADDR_P2V(xfer_evt),
        sizeof(xhci_trb_t),
        XHCI_RING_ITEMS
    );


    if (ERROR(xhci_halt(xhci)))
    {
        PR_LOG(LOG_ERROR, "Failed to halt xhci controller.\n");
        goto fail;
    }
    if (ERROR(xhci_reset(xhci)))
    {
        PR_LOG(LOG_ERROR, "Failed to reset xhci controller.\n");
        goto fail;
    }

    xhci_write_opt(xhci, XHCI_OPT_CONFIG, xhci->slots);

    // Write DCBAA
    uint32_t dcbaap_lo = (uint64_t)devs & 0xffffffff;
    uint32_t dcbaap_hi = (uint64_t)devs >> 32;
    xhci_write_opt(xhci, XHCI_OPT_DCBAAP_LO, dcbaap_lo);
    xhci_write_opt(xhci, XHCI_OPT_DCBAAP_HI, dcbaap_hi);

    // Write ERSTBA
    xhci->eseg->ptr_lo = (uint64_t)evts & 0xffffffff;
    xhci->eseg->ptr_hi = (uint64_t)evts >> 32;
    xhci->eseg->size   = XHCI_RING_ITEMS;

    xhci_write_run(xhci, XHCI_IRS_ERSTSZ(0), ERSTSZ_SET(1));

    uint32_t erstba_lo = (uint64_t)eseg & 0xffffffff;
    uint32_t erstba_hi = (uint64_t)eseg >> 32;
    xhci_write_run(xhci, XHCI_IRS_ERSTBA_LO(0), erstba_lo);
    xhci_write_run(xhci, XHCI_IRS_ERSTBA_HI(0), erstba_hi);

    // Write ERDP
    xhci->evts.trb_count = XHCI_RING_ITEMS;
    xhci->evts.cs        = 1;
    xhci->evts.enqueue   = 0;
    xhci->evts.dequeue   = 0;
    uint32_t erdp_lo     = (uint64_t)evts & 0xffffffff;
    uint32_t erdp_hi     = (uint64_t)evts >> 32;
    xhci_write_run(xhci, XHCI_IRS_ERDP_LO(0), erdp_lo);
    xhci_write_run(xhci, XHCI_IRS_ERDP_HI(0), erdp_hi);

    // Write CRCR
    xhci->cmds.trb_count = XHCI_RING_ITEMS;
    xhci->cmds.cs        = 1;
    xhci->cmds.enqueue   = 0;
    xhci->cmds.dequeue   = 0;
    uint32_t crcr_lo     = (uint64_t)cmds & 0xffffffff;
    uint32_t crcr_hi     = (uint64_t)cmds >> 32;
    xhci_write_opt(xhci, XHCI_OPT_CRCR_LO, crcr_lo | xhci->cmds.cs);
    xhci_write_opt(xhci, XHCI_OPT_CRCR_HI, crcr_hi);

    uint32_t hcs2 = xhci_read_cap(xhci, XHCI_CAP_HCSPARAM2);
    int      spb  = GET_HCSP2_MAX_SC_BUF(hcs2);
    if (spb != 0)
    {
        uint64_t *spba;
        status = pmalloc(sizeof(uint64_t) * spb, 64, 4096, &spba);
        if (ERROR(status))
        {
            PR_LOG(LOG_ERROR, "Failed to alloc spba.\n");
            goto fail;
        }
        int i;
        for (i = 0; i < spb; i++)
        {
            void *pad = NULL;
            status =
                pmalloc(XHCI_PAGE_SIZE, XHCI_PAGE_SIZE, XHCI_PAGE_SIZE, &pad);
            if (ERROR(status))
            {
                PR_LOG(LOG_ERROR, "Failed to alloc sctatch pad buf.\n");
                while (--i >= 0)
                {
                    pfree(((uint64_t **)KADDR_P2V(spba))[i]);
                }
                pfree(spba);
                goto fail;
            }
            ((uint64_t **)KADDR_P2V(spba))[i] = pad;
        }
        xhci->devs[0].ptr = (uint64_t)spba;
    }
    xhci_run(xhci);

    task_msleep(XHCI_TIME_POSTPOWER);

    memset(hub, 0, sizeof(*hub));
    hub->ctrl      = &xhci->usb;
    hub->portcount = xhci->ports;
    hub->op        = &xhci_hub_ops;
    return K_SUCCESS;

fail:
    pfree(xfer_evt);
    pfree(port_evt);
    pfree(cmds_evt);

    pfree(cmds);
    pfree(evts);
    pfree(eseg);
    pfree(devs);

    return K_ERROR;
}

PUBLIC status_t xhci_setup(usb_hub_set_t *hub_set)
{
    int number_of_xhci;
    number_of_xhci = pci_dev_count(0x0c, 0x03, 0x30);
    status_t status =
        pmalloc(sizeof(*hub_set->hubs) * number_of_xhci, 0, 0, &hub_set->hubs);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Failed to alloc hub_set.\n");
        return status;
    }
    hub_set->hubs  = KADDR_P2V(hub_set->hubs);
    hub_set->count = number_of_xhci;

    pci_device_t *pci;
    int           i;
    for (i = 0; i < number_of_xhci; i++)
    {
        pci              = pci_dev_match(0x0c, 0x03, 0x30, i);
        addr_t mmio_base = pci_dev_read_bar(pci, 0);
        page_map(
            (uint64_t *)KERNEL_PAGE_DIR_TABLE_POS,
            (void *)mmio_base,
            (void *)KADDR_P2V(mmio_base)
        );
        if ((pci_dev_config_read(pci, 0) & 0xffff) == 0x8086)
        {
            switch_to_xhci(pci);
        }
        xhci_t *xhci = NULL;
        xhci_controller_setup(KADDR_P2V(mmio_base), &xhci);
        xhci->usb.pci = pci;

        configure_xhci(xhci, &hub_set->hubs[i]);
    }
    return K_SUCCESS;
}

PUBLIC void xhci_process_events(xhci_t *xhci)
{
    uint16_t     dequeue = xhci->evts.dequeue;
    uint8_t      cs      = xhci->evts.cs;
    xhci_ring_t *evts    = &xhci->evts;
    xhci_trb_t  *trb;
    trb_type_t   type;
    uint32_t     ctrl;
    while (1)
    {
        trb  = &evts->trbs[dequeue];
        ctrl = trb->control;
        if ((ctrl & TRB_C) != cs)
        {
            return;
        }
        type = TRB_TYPE(ctrl);
        switch (type)
        {
            case ER_TRANSFER:
                fifo_write(&xhci->xfer_evts, trb);
                break;
            case ER_COMMAND_COMPLETE:
                fifo_write(&xhci->cmds_evts, trb);
                break;

            case ER_PORT_STATUS_CHANGE:
                uint32_t port   = ((trb->ptr >> 24) & 0xff) - 1;
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
                break;

            default:
                PR_LOG(LOG_WARN, "unknow trb type: %d.\n", type);
                break;
        }
        dequeue++;
        if (dequeue == evts->trb_count)
        {
            dequeue  = 0;
            cs       = !cs;
            evts->cs = cs;
        }
        evts->dequeue = dequeue;
        uint64_t addr = (uint64_t)KADDR_V2P(&evts->trbs[dequeue]);

        uint32_t erdp_lo = (addr & 0xffffffff) | (1 << 3); // clear EHB
        uint32_t erdp_hi = addr >> 32;
        xhci_write_run(xhci, XHCI_IRS_ERDP_LO(0), erdp_lo);
        xhci_write_run(xhci, XHCI_IRS_ERDP_HI(0), erdp_hi);
    }
    return;
}

PRIVATE void xhci_trb_queue(xhci_ring_t *ring, xhci_trb_t *trb)
{
    trb->control |= ring->cs;
    ring->trbs[ring->enqueue] = *trb;
    ring->enqueue++;
    if (ring->enqueue == ring->trb_count - 1)
    {
        uint32_t control = 0;
        control          = (TRB_LK_TC | ring->cs) | (TR_LINK << 10);
        ring->trbs[ring->enqueue].control = control;
        ring->enqueue                     = 0;
        ring->cs                          = !ring->cs;
    }
    return;
}

PRIVATE void xhci_doorbell_ring(xhci_t *xhci, uint8_t slot, uint8_t endpoint)
{
    xhci_write_doorbell(
        xhci,
        XHCI_DOORBELL(slot),
        XHCI_DOORBELL_TARGET(endpoint) | XHCI_DOORBELL_STREAMID(0)
    );
    return;
}

PRIVATE trb_comp_code_t
xhci_event_wait(fifo_t *fifo, xhci_trb_t *trb, int timeout)
{
    while (fifo_empty(fifo))
    {
        task_msleep(1);
        if (--timeout == 0)
        {
            PR_LOG(LOG_ERROR, "Process command timeout.\n");
            return CC_INVALID;
        }
    }
    // Completed
    fifo_read(fifo, trb);
    trb_comp_code_t ret = (trb->status >> 24) & 0xff;
    if (ret != CC_SUCCESS)
    {
        PR_LOG(LOG_ERROR, "Command Error: %d.\n", ret);
    }
    return ret;
}

PRIVATE trb_comp_code_t xhci_cmd_submit(xhci_t *xhci, xhci_trb_t *trb)
{
    xhci_trb_queue(&xhci->cmds, trb);
    xhci_doorbell_ring(xhci, 0, 0);

    // Wait Host Controller Process the command.
    return xhci_event_wait(&xhci->cmds_evts, trb, 200);
}

PRIVATE int xhci_cmd_enable_slot(xhci_t *xhci)
{
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.control        = CR_ENABLE_SLOT << 10;
    trb_comp_code_t cc = xhci_cmd_submit(xhci, &trb);
    if (cc != CC_SUCCESS)
    {
        return -1;
    }
    return (trb.control >> 24) & 0xff;
}

PRIVATE int xhci_cmd_disable_slot(xhci_t *xhci, uint32_t slot_id)
{
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.control = CR_DISABLE_SLOT << 10 | slot_id << 24;
    return xhci_cmd_submit(xhci, &trb);
}

PRIVATE trb_comp_code_t
xhci_cmd_address_device(xhci_t *xhci, uint32_t slot_id, xhci_in_ctx_t *in_ctx)
{
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.ptr     = (uint64_t)in_ctx;
    trb.control = CR_ADDRESS_DEVICE << 10 | slot_id << 24;
    return xhci_cmd_submit(xhci, &trb);
}

PRIVATE trb_comp_code_t xhci_cmd_configure_endpoint(
    xhci_t        *xhci,
    uint32_t       slot_id,
    xhci_in_ctx_t *in_ctx
)
{
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.ptr     = (uint64_t)in_ctx;
    trb.control = CR_CONFIGURE_ENDPOINT << 10 | slot_id << 24;
    return xhci_cmd_submit(xhci, &trb);
}

PRIVATE trb_comp_code_t
xhci_cmd_evaluate_context(xhci_t *xhci, uint32_t slot_id, xhci_in_ctx_t *in_ctx)
{
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.ptr     = (uint64_t)in_ctx;
    trb.control = CR_EVALUATE_CONTEXT << 10 | slot_id << 24;
    return xhci_cmd_submit(xhci, &trb);
}

/**
 * @brief 分配input context
 * @param usb_dev
 * @param max_epid
 * @return input context (vitrual address)
 */
PRIVATE xhci_in_ctx_t *xhci_alloc_in_ctx(usb_device_t *usb_dev, int max_epid)
{
    xhci_t        *xhci = CONTAINER_OF(xhci_t, usb, usb_dev->hub->ctrl);
    size_t         size = (sizeof(xhci_in_ctx_t) * 33) << xhci->context64;
    xhci_in_ctx_t *in_ctx;
    status_t       status = pmalloc(size, 64, 4096, &in_ctx);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Failed to alloc in_ctx.\n");
        return NULL;
    }
    in_ctx = KADDR_P2V(in_ctx);
    memset(in_ctx, 0, size);

    xhci_slot_ctx_t *slot_ctx = (void *)&in_ctx[1 << xhci->context64];
    slot_ctx->ctx[0] |= max_epid << 27;
    slot_ctx->ctx[0] |= speed_to_xhci(usb_dev->speed) << 20;

    // Set High-Speed hub flags.
    usb_device_t *hub_dev = usb_dev->hub->usb_dev;
    if (hub_dev != NULL)
    {
        if (usb_dev->speed == USB_LOWSPEED || usb_dev->speed == USB_FULLSPEED)
        {
            xhci_pipe_t *hpipe;
            hpipe = CONTAINER_OF(xhci_pipe_t, pipe, hub_dev->defpipe);
            if (hub_dev->speed == USB_HIGHSPEED)
            {
                slot_ctx->ctx[2] |= hpipe->slot_id;
                slot_ctx->ctx[2] |= (usb_dev->port + 1) << 8;
            }
            else
            {
                xhci_slot_ctx_t *hslot;
                hslot            = KADDR_P2V(xhci->devs[hpipe->slot_id].ptr);
                slot_ctx->ctx[2] = hslot->ctx[2];
            }
        }
        uint32_t route = 0;
        while (usb_dev->hub->usb_dev)
        {
            route <<= 4;
            route |= (usb_dev->port + 1) & 0xf;
            usb_dev = usb_dev->hub->usb_dev;
        }
        slot_ctx->ctx[0] |= route;
    }
    slot_ctx->ctx[1] |= (usb_dev->port + 1) << 16;
    return in_ctx;
}

PRIVATE int xhci_configure_hub(usb_hub_t *hub)
{
    xhci_t      *xhci = CONTAINER_OF(xhci_t, usb, hub->ctrl);
    xhci_pipe_t *pipe = CONTAINER_OF(xhci_pipe_t, pipe, hub->usb_dev->defpipe);
    xhci_slot_ctx_t *hd_slot = (void *)KADDR_P2V(xhci->devs[pipe->slot_id].ptr);
    if ((hd_slot->ctx[3] >> 27) == 3)
    {
        // Already configured
        return 0;
    }
    xhci_in_ctx_t *in_ctx = xhci_alloc_in_ctx(hub->usb_dev, 1);
    if (in_ctx == NULL)
    {
        return -1;
    }
    in_ctx->add           = 0x01;
    xhci_slot_ctx_t *slot = (void *)&in_ctx[1 << xhci->context64];
    slot->ctx[0] |= 1 << 26;
    slot->ctx[1] |= hub->portcount << 24;
    // configure endpoint
    trb_comp_code_t cc;
    cc = xhci_cmd_configure_endpoint(xhci, pipe->slot_id, KADDR_V2P(in_ctx));
    pfree(KADDR_V2P(in_ctx));
    if (cc != CC_SUCCESS)
    {
        PR_LOG(LOG_ERROR, "Failed to configure ep.\n");
        return -1;
    }
    return 0;
}

/**
 * @brief
 * @param usb_dev
 * @param epdesc
 * @return pipe (virtual address)
 */
PRIVATE usb_pipe_t *
xhci_alloc_pipe(usb_device_t *usb_dev, usb_endpoint_descriptor_t *epdesc)
{
    uint8_t      eptype   = epdesc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
    xhci_t      *xhci     = CONTAINER_OF(xhci_t, usb, usb_dev->hub->ctrl);
    xhci_pipe_t *pipe     = NULL;
    uint32_t     epid     = 0;
    void        *pipe_buf = NULL;
    if (epdesc->bEndpointAddress == 0)
    {
        epid = 1;
    }
    else
    {
        epid = (epdesc->bEndpointAddress & 0x0f) * 2;
        epid += (epdesc->bEndpointAddress & USB_DIR_IN) ? 1 : 0;
    }

    status_t status = pmalloc(sizeof(*pipe), 64, 4096, &pipe);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Failed to alloc pipe.\n");
        return NULL;
    }
    pipe = KADDR_P2V(pipe);
    memset(pipe, 0, sizeof(*pipe));

    void *pipe_reqs = NULL;
    status          = pmalloc(XHCI_RING_SIZE, 64, 65536, &pipe_reqs);
    if (ERROR(status))
    {
        PR_LOG(LOG_ERROR, "Failed to alloc pipe reqs.trbs.\n");
        pfree(KADDR_V2P(pipe));
        return NULL;
    }

    usb_desc2pipe(&pipe->pipe, usb_dev, epdesc);
    pipe->epid           = epid;
    pipe->reqs.cs        = 1;
    pipe->reqs.trbs      = KADDR_P2V(pipe_reqs);
    pipe->reqs.enqueue   = 0;
    pipe->reqs.dequeue   = 0;
    pipe->reqs.trb_count = XHCI_RING_ITEMS;
    memset(pipe->reqs.trbs, 0, XHCI_RING_SIZE);

    if (eptype == USB_ENDPOINT_XFER_INT)
    {
        status = pmalloc(pipe->pipe.max_packet, 0, 0, &pipe_buf);
        if (ERROR(status))
        {
            pfree(KADDR_V2P(pipe));
            return NULL;
        }
        pipe->buf = KADDR_P2V(pipe_buf);
    }

    // Allocate input context and initialize endpoint info.
    xhci_in_ctx_t *in_ctx = xhci_alloc_in_ctx(usb_dev, epid);
    if (in_ctx == NULL)
    {
        PR_LOG(LOG_ERROR, "Failed to alloc in_ctx.\n");
        goto fail;
    }
    in_ctx->add       = 0x01 | (1 << epid);
    xhci_ep_ctx_t *ep = (void *)&in_ctx[(pipe->epid + 1) << xhci->context64];
    if (eptype == USB_ENDPOINT_XFER_INT)
    {
        ep->ctx[0] = (usb_get_period(usb_dev, epdesc) + 3) << 16;
    }
    // ep->ctx[1]   |= 3 << 1;
    ep->ctx[1] |= eptype << 3;
    if (epdesc->bEndpointAddress & USB_DIR_IN ||
        eptype == USB_ENDPOINT_XFER_CONTROL)
    {
        ep->ctx[1] |= 1 << 5;
    }
    ep->ctx[1] |= pipe->pipe.max_packet << 16;
    ep->deq_ptr = (uint64_t)KADDR_V2P(&pipe->reqs.trbs[0]);
    ep->deq_ptr |= 1; // dcs
    ep->length = pipe->pipe.max_packet;

    PR_LOG(LOG_DEBUG, "slot id %d, epid %d.\n", pipe->slot_id, pipe->epid);

    if (pipe->epid == 1)
    {
        if (usb_dev->hub->usb_dev != NULL)
        {
            int ret = xhci_configure_hub(usb_dev->hub);
            if (ret != 0)
            {
                goto fail;
            };
        }

        // Enable slot
        size_t size = (sizeof(xhci_slot_ctx_t) * 32) << xhci->context64;
        xhci_slot_ctx_t *dev;
        status = pmalloc(size, 1024 << xhci->context64, 4096, &dev);
        if (ERROR(status))
        {
            goto fail;
        }
        int slot_id = xhci_cmd_enable_slot(xhci);
        if (slot_id < 0)
        {
            PR_LOG(LOG_ERROR, "Failed to enable slot.\n");
            pfree(dev);
            goto fail;
        }
        memset(KADDR_P2V(dev), 0, size);
        xhci->devs[slot_id].ptr = (uint64_t)dev;

        // Address device
        trb_comp_code_t cc =
            xhci_cmd_address_device(xhci, slot_id, KADDR_V2P(in_ctx));
        if (cc != CC_SUCCESS)
        {
            PR_LOG(LOG_ERROR, "Failed to address device: cc = %d.\n", cc);
            cc = xhci_cmd_disable_slot(xhci, slot_id);
            if (cc != CC_SUCCESS)
            {
                PR_LOG(LOG_ERROR, "Failed to disable slot.\n");
                goto fail;
            }
            xhci->devs[slot_id].ptr = 0;
            pfree(dev);
            goto fail;
        }
        pipe->slot_id = slot_id;
    }
    else
    {
        xhci_pipe_t *defpipe =
            CONTAINER_OF(xhci_pipe_t, pipe, usb_dev->defpipe);
        pipe->slot_id = defpipe->slot_id;

        // Send configure command.
        trb_comp_code_t cc =
            xhci_cmd_configure_endpoint(xhci, pipe->slot_id, KADDR_V2P(in_ctx));
        if (cc != CC_SUCCESS)
        {
            PR_LOG(LOG_ERROR, "Failed to configure ep.\n");
            goto fail;
        }
    }
    pfree(KADDR_V2P(in_ctx));
    return &pipe->pipe;
fail:
    pfree(pipe_buf);
    pfree(pipe_reqs);
    pfree(KADDR_V2P(pipe));
    pfree(KADDR_V2P(in_ctx));

    return NULL;
}

PUBLIC usb_pipe_t *xhci_realloc_pipe(
    usb_device_t              *usb_dev,
    usb_pipe_t                *upipe,
    usb_endpoint_descriptor_t *epdesc
)
{
    if (epdesc == NULL)
    {
        usb_add_freelist(upipe);
        return NULL;
    }
    if (upipe == NULL)
    {
        return xhci_alloc_pipe(usb_dev, epdesc);
    }
    uint8_t  eptype         = epdesc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
    uint16_t old_max_packet = upipe->max_packet;
    usb_desc2pipe(upipe, usb_dev, epdesc);
    xhci_pipe_t *pipe = CONTAINER_OF(xhci_pipe_t, pipe, upipe);
    xhci_t      *xhci = CONTAINER_OF(xhci_t, usb, pipe->pipe.ctrl);
    if (eptype != USB_ENDPOINT_XFER_CONTROL ||
        upipe->max_packet == old_max_packet)
    {
        return upipe;
    }

    // maxpacket has changed on control endpoint - update controller.
    xhci_in_ctx_t *in_ctx = xhci_alloc_in_ctx(usb_dev, 1);
    if (in_ctx == NULL)
    {
        return upipe;
    }
    in_ctx->add       = (1 << 1);
    xhci_ep_ctx_t *ep = (void *)&in_ctx[2 << xhci->context64];
    ep->ctx[1] |= pipe->pipe.max_packet << 16;

    // Evaluate context
    trb_comp_code_t cc =
        xhci_cmd_evaluate_context(xhci, pipe->slot_id, KADDR_V2P(in_ctx));
    if (cc != CC_SUCCESS)
    {
        PR_LOG(LOG_ERROR, "Failed to evaluate context.\n");
    }
    pfree(KADDR_V2P(in_ctx));
    return upipe;
}

PRIVATE trb_comp_code_t xhci_xfer_wait(xhci_t *xhci)
{
    xhci_trb_t      trb;
    trb_comp_code_t cc;
    cc = xhci_event_wait(&xhci->xfer_evts, &trb, USB_TIME_COMMAND);
    if (cc != CC_SUCCESS)
    {
        PR_LOG(LOG_ERROR, "xfer failed: %d.\n", cc);
    }
    return cc;
}

/**
 * @brief
 * @param pipe
 * @param dir
 * @param cmd
 * @param data (virtual adderss)
 * @param data_len
 * @return
 */
PRIVATE trb_comp_code_t
xhci_xfer_setup(xhci_pipe_t *pipe, int dir, void *cmd, void *data, int data_len)
{
    xhci_t *xhci = CONTAINER_OF(xhci_t, usb, pipe->pipe.ctrl);

    // Setup stage
    xhci_trb_t setup_trb;
    memset(&setup_trb, 0, sizeof(setup_trb));
    setup_trb.ptr     = *(uint64_t *)cmd;
    setup_trb.status  = USB_CONTROL_SETUP_SIZE;
    setup_trb.control = TRB_TR_IDT;
    setup_trb.control |= (TR_SETUP << 10);
    setup_trb.control |= ((data_len ? (dir ? 3 : 2) : 0)) << 16;
    xhci_trb_queue(&pipe->reqs, &setup_trb);

    // Data stage
    if (data_len != 0)
    {
        xhci_trb_t data_trb;
        memset(&data_trb, 0, sizeof(data_trb));
        data_trb.ptr     = (uint64_t)KADDR_V2P(data);
        data_trb.status  = data_len;
        data_trb.control = (TR_DATA << 10);
        data_trb.control |= (dir ? 1 : 0) << 16;
        xhci_trb_queue(&pipe->reqs, &data_trb);
    }

    // Status stage
    xhci_trb_t status_trb;
    memset(&status_trb, 0, sizeof(status_trb));
    status_trb.ptr     = 0;
    status_trb.control = TRB_TR_IOC;
    status_trb.control |= (TR_STATUS << 10);
    status_trb.control |= (dir ? 0 : 1) << 16;
    xhci_trb_queue(&pipe->reqs, &status_trb);

    xhci_doorbell_ring(xhci, pipe->slot_id, pipe->epid);

    trb_comp_code_t cc = xhci_xfer_wait(xhci);
    if (cc != CC_SUCCESS)
    {
        PR_LOG(LOG_ERROR, "Failed to xfer setup.\n");
        return cc;
    }
    return cc;
}


PRIVATE trb_comp_code_t
xhci_xfer_normal(xhci_pipe_t *pipe, void *data, int data_len)
{
    xhci_t *xhci = CONTAINER_OF(xhci_t, usb, pipe->pipe.ctrl);

    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    trb.ptr = (uint64_t)data;
    trb.status |= data_len;
    trb.control |= TRB_TR_IOC;
    trb.control |= (TR_NORMAL << 10);
    xhci_trb_queue(&pipe->reqs, &trb);

    xhci_doorbell_ring(xhci, pipe->slot_id, pipe->epid);

    return xhci_xfer_wait(xhci);
}

PUBLIC int xhci_send_pipe(
    usb_pipe_t *p,
    int         dir,
    const void *cmd,
    void       *data,
    int         data_len
)
{
    xhci_pipe_t    *pipe = CONTAINER_OF(xhci_pipe_t, pipe, p);
    trb_comp_code_t cc   = CC_SUCCESS;
    if (cmd != NULL)
    {
        const usb_ctrl_request_t *req = cmd;
        if (req->bRequest == USB_REQ_SET_ADDRESS)
        {
            return 0;
        }
        cc = xhci_xfer_setup(pipe, dir, (void *)req, data, data_len);
    }
    else
    {
        cc = xhci_xfer_normal(pipe, data, data_len);
    }
    if (cc != CC_SUCCESS)
    {
        PR_LOG(LOG_ERROR, "xhci send pipe error.\n");
    }
    return 0;
}