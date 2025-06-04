/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/usb/xhci.h> // xhci struct
#include <device/pci.h>      // pci functions
#include <mem/mem.h>         // pmalloc,pfree
#include <std/string.h>      // memset
#include <task/task.h>       // task_start

#include <log.h>

PRIVATE int speed_from_xhci(uint8_t speed)
{
    switch (speed)
    {
        case 1: return USB_FULLSPEED;
        case 2: return USB_LOWSPEED;
        case 3: return USB_HIGHSPEED;
        case 4: return USB_SUPERSPEED;
        default: return -1;
    }
    return -1;
};

PRIVATE void switch_to_xhci(pci_device_t *xhci_dev)
{
    pci_device_t *ehci = pci_dev_match(0x0c,0x03,0x20,0);
    if (ehci == NULL || (pci_dev_config_read(ehci,0) & 0xffff) != 0x8086)
    {
        // ehci not exist
        return;
    }

    uint32_t superspeed_ports = pci_dev_config_read(xhci_dev, 0xdc); // USB3PRM
    pci_dev_config_write(xhci_dev, 0xd8, superspeed_ports); // USB3_PSSEN
    uint32_t ehci2xhci_ports = pci_dev_config_read(xhci_dev, 0xd4); // XUSB2PRM
    pci_dev_config_write(xhci_dev, 0xd0, ehci2xhci_ports); // XUSB2PR
    return;
}

PRIVATE status_t xhci_controller_setup(void *mmio_base,xhci_t **xhci_addr)
{
    xhci_t *xhci;
    status_t status;
    status = pmalloc(sizeof(*xhci),0,0,&xhci);
    if (ERROR(status))
    {
        pr_log("\3 Alloc memory for xhci failed.\n");
        return status;
    }
    xhci = KADDR_P2V(xhci);
    memset(xhci,0,sizeof(*xhci));
    xhci->mmio_base = mmio_base;
    xhci->cap_regs = xhci->mmio_base;

    size_t caplength = xhci_read_cap(xhci,XHCI_CAP_CAPLENGTH);
    xhci->opt_regs = xhci->cap_regs + GET_FIELD(caplength,CAPLENGTH);

    xhci->run_regs = xhci->cap_regs + xhci_read_cap(xhci,XHCI_CAP_RSTOFF);
    xhci->doorbell_regs = xhci->cap_regs + xhci_read_cap(xhci,XHCI_CAP_DBOFF);

    uint32_t hcs1 = xhci_read_cap(xhci,XHCI_CAP_HCSPARAM1);
    uint32_t hcc1  = xhci_read_cap(xhci,XHCI_CAP_HCCPARAM1);

    xhci->ports = GET_FIELD(hcs1,HCSP1_MAX_PORTS);
    xhci->slots = GET_FIELD(hcs1,HCSP1_MAX_SLOTS);
    xhci->xecp  = GET_FIELD(hcc1,HCCP1_XECP) << 2;
    xhci->context64 = GET_FIELD(hcc1,HCCP1_CSZ) ? 1 : 0;
    // xhci->usb.type = ;
    pr_log("XHCI: mmio base: %p, %d ports, %d slots, %d bytes context.\n",
        mmio_base,xhci->ports,xhci->slots,xhci->context64 ? 64 : 32);

    if (xhci->xecp != 0)
    {
        uint32_t offset;
        addr_t addr = (addr_t)xhci->cap_regs + xhci->xecp;
        do
        {
            xhci_xcap_regs_t *xcap = (xhci_xcap_regs_t*)addr;
            uint32_t cap = xcap->cap;
            uint32_t name;
            uint32_t ports;
            switch (GET_FIELD(cap,XECP_CAP_ID))
            {
                case 0x02:
                    name = xcap->data[0];
                    ports = xcap->data[1];
                    uint8_t major = GET_FIELD(cap,XECP_SUP_MAJOR);
                    uint8_t minor = GET_FIELD(cap,XECP_SUP_MINOR);
                    uint8_t count = (ports >> 8) & 0xff;
                    uint8_t start = (ports >> 0) & 0xff;
                    pr_log("xHCI protocol %c%c%c%c"
                        " %x.%02x ,%d ports (offset %d), def %x\n",
                        (name >>  0) & 0xff,
                        (name >>  8) & 0xff,
                        (name >> 16) & 0xff,
                        (name >> 24) & 0xff,
                        major,minor,count,start,ports >> 16);
                    if (strncmp((const char*)&name,"USB ",4) == 0)
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
                    pr_log("XHCI XECP %#02x.\n",GET_FIELD(cap,XECP_CAP_ID));
                    break;
            }
            offset = GET_FIELD(cap,XECP_NEXT_POINT) << 2;
            addr += offset;
        } while (offset > 0);
    }
    uint32_t pagesize = xhci_read_opt(xhci,XHCI_OPT_PAGESIZE);
    if (XHCI_PAGE_SIZE != pagesize << 12)
    {
        pr_log("\3 XHCI Driver not support pagesize: %d.\n",pagesize << 12);
        pfree(KADDR_V2P(xhci));
        *xhci_addr = NULL;
        return K_ERROR;
    }
    *xhci_addr = xhci;
    return K_SUCCESS;
}

PRIVATE status_t xhci_halt(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci,XHCI_OPT_USBCMD);
    if (!(usbcmd & XHCI_CMD_RS))
    {
        return K_SUCCESS;
    }
    usbcmd &= ~XHCI_CMD_RS;
    xhci_write_opt(xhci,XHCI_OPT_USBCMD,usbcmd);
    int timeout = 200;
    while((xhci_read_opt(xhci,XHCI_OPT_USBSTS) & XHCI_STS_HCH) == 0)
    {
        if (--timeout == 0)
        {
            pr_log("\3 Host controller halt timeout.\n");
            return K_TIMEOUT;
        }
        task_msleep(1);
    };
    return K_SUCCESS;
}

PRIVATE status_t xhci_reset(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci,XHCI_OPT_USBCMD);
    usbcmd |= XHCI_CMD_HCRST;
    xhci_write_opt(xhci,XHCI_OPT_USBCMD,usbcmd);

    int timeout = 1000;
    while((xhci_read_opt(xhci,XHCI_OPT_USBCMD) & XHCI_CMD_HCRST) ||
          (xhci_read_opt(xhci,XHCI_OPT_USBSTS) & XHCI_STS_CNR))
    {
        if (--timeout == 0)
        {
            pr_log("\3 Host controller reset timeout.\n");
            return K_TIMEOUT;
        }
        task_msleep(1);
    }
    task_msleep(50);
    if (xhci_read_opt(xhci,XHCI_OPT_USBCMD) != 0)
    {
        return K_ERROR;
    }
    if (xhci_read_opt(xhci,XHCI_OPT_DNCTRL) != 0)
    {
        return K_ERROR;
    }
    if (xhci_read_opt(xhci,XHCI_OPT_CRCR_LO) != 0)
    {
        return K_ERROR;
    }
    if (xhci_read_opt(xhci,XHCI_OPT_CRCR_HI) != 0)
    {
        return K_ERROR;
    }
    if (xhci_read_opt(xhci,XHCI_OPT_DCBAAP_LO) != 0)
    {
        return K_ERROR;
    }
    if (xhci_read_opt(xhci,XHCI_OPT_DCBAAP_HI) != 0)
    {
        return K_ERROR;
    }
    if (xhci_read_opt(xhci,XHCI_OPT_CONFIG) != 0)
    {
        return K_ERROR;
    }
    return K_SUCCESS;
}

PRIVATE status_t xhci_run(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci,XHCI_OPT_USBCMD);
    usbcmd |= XHCI_CMD_RS;
    xhci_write_opt(xhci,XHCI_OPT_USBCMD,usbcmd);
    int timeout = 1000;
    while((xhci_read_opt(xhci,XHCI_OPT_USBSTS) & XHCI_STS_HCH) == 1)
    {
        if (--timeout == 0)
        {
            pr_log("\3 xhci start timeout.\n");
            return K_TIMEOUT;
        }
        task_msleep(1);
    };

    // Verify CNR bit clear
    uint32_t usbsts;
    usbsts = xhci_read_opt(xhci,XHCI_OPT_USBSTS);
    if (usbsts & XHCI_STS_CNR)
    {
        // Control not redy.
        return K_ERROR;
    }
    return K_SUCCESS;
}

// XHCI Hub opt

PRIVATE int xhci_hub_detect(usb_hub_t *hub,uint32_t port)
{
    xhci_t *xhci = CONTAINER_OF(xhci_t,usb,hub->cntl);
    uint32_t portsc;
    portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port));
    return (portsc & XHCI_PORTSC_CCS) ? 1 : 0;
}

PRIVATE int xhci_hub_reset(usb_hub_t *hub,uint32_t port)
{
    xhci_t *xhci = CONTAINER_OF(xhci_t,usb,hub->cntl);
    uint32_t portsc;
    portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port));
    if ((portsc & XHCI_PORTSC_CSC) != 0)
    {
        return -1;
    }
    switch (GET_FIELD(portsc,XHCI_PORTSC_PLS))
    {
    case PLS_U0:
        // A USB3 port - controller automatically performs reset
        break;
    case PLS_POLLING:
        // A USB2 port - perform device reset
        xhci_write_opt(xhci,XHCI_OPT_PORTSC(port),portsc | XHCI_PORTSC_PR);
        break;
    default:
        return -1;
        break;
    }
    int timeout = 100;
    while (timeout > 0)
    {
        portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port));
        if (!(portsc & XHCI_PORTSC_CCS))
        {
            return -1;
        }
        if (portsc & XHCI_PORTSC_PED)
        {
            // reset complete
            break;
        }
        if (timeout == 0)
        {
            return K_TIMEOUT;
        }
        timeout--;
        task_msleep(1);
    }
    return speed_from_xhci(GET_FIELD(portsc,XHCI_PORTSC_SPEED));
}

PRIVATE int xhci_hub_portmap(usb_hub_t *hub,uint32_t port)
{
    xhci_t *xhci = CONTAINER_OF(xhci_t,usb,hub->cntl);
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

PRIVATE int xhci_hub_disconnect(usb_hub_t *hub,uint32_t port)
{
    // turn the port power off
    (void)hub;
    (void)port;
    return 0;
}

PRIVATE usb_hub_op_t xhci_hub_ops =
{
    xhci_hub_detect,
    xhci_hub_reset,
    xhci_hub_portmap,
    xhci_hub_disconnect,
};

PRIVATE int xhci_check_ports(xhci_t *xhci)
{
    task_msleep(XHCI_TIME_POSTPOWER);
    usb_hub_t hub;
    memset(&hub,0,sizeof(hub));
    hub.cntl      = &xhci->usb;
    hub.portcount = xhci->ports;
    hub.op        = &xhci_hub_ops;
    usb_enumerate(&hub);
    return hub.devcount;
}

PRIVATE void configure_xhci(xhci_t *xhci)
{
    pr_log("\1 Configure xhci: %p.\n",xhci);
    status_t status;
    void *devs,*eseg,*evts,*cmds;

    status = pmalloc(sizeof(*xhci->devs) * (xhci->slots + 1),64,0,&devs);
    if (ERROR(status))
    {
        goto fail;
    }

    status = pmalloc(sizeof(*xhci->eseg),64,0,&eseg);
    if (ERROR(status))
    {
        goto fail;
    }

    status = pmalloc(XHCI_RING_SIZE,64,65536,&evts);
    if (ERROR(status))
    {
        goto fail;
    }

    status = pmalloc(XHCI_RING_SIZE,64,65536,&cmds);
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

    if (ERROR(xhci_halt(xhci)))
    {
        pr_log("\3 Failed to halt xhci controller.\n");
        goto fail;
    }
    if (ERROR(xhci_reset(xhci)))
    {
        pr_log("\3 Failed to reset xhci controller.\n");
        goto fail;
    }

    xhci_write_opt(xhci,XHCI_OPT_CONFIG,xhci->slots);

    // Write DCBAA
    uint32_t dcbaap_lo = (uint64_t)devs & 0xffffffff;
    uint32_t dcbaap_hi = (uint64_t)devs >> 32;
    xhci_write_opt(xhci,XHCI_OPT_DCBAAP_LO,dcbaap_lo);
    xhci_write_opt(xhci,XHCI_OPT_DCBAAP_HI,dcbaap_hi);

    // Write ERSTBA
    xhci->eseg->ptr_lo = (uint64_t)evts & 0xffffffff;
    xhci->eseg->ptr_hi = (uint64_t)evts >> 32;
    xhci->eseg->size   = XHCI_RING_ITEMS;

    xhci_write_run(xhci,XHCI_IRS_ERSTSZ(0),ERSTSZ_SET(1));

    uint32_t erstba_lo = (uint64_t)eseg & 0xffffffff;
    uint32_t erstba_hi = (uint64_t)eseg >> 32;
    xhci_write_run(xhci,XHCI_IRS_ERSTBA_LO(0),erstba_lo);
    xhci_write_run(xhci,XHCI_IRS_ERSTBA_HI(0),erstba_hi);

    // Write ERDP
    xhci->evts.trb_count = XHCI_RING_ITEMS;
    xhci->evts.cs = 1;
    uint32_t erdp_lo = (uint64_t)evts & 0xffffffff;
    uint32_t erdp_hi = (uint64_t)evts >> 32;
    xhci_write_run(xhci,XHCI_IRS_ERDP_LO(0),erdp_lo);
    xhci_write_run(xhci,XHCI_IRS_ERDP_HI(0),erdp_hi);

    // Write CRCR
    xhci->cmds.trb_count = XHCI_RING_ITEMS;
    xhci->cmds.cs = 1;
    uint32_t crcr_lo = (uint64_t)cmds & 0xffffffff;
    uint32_t crcr_hi = (uint64_t)cmds >> 32;
    xhci_write_opt(xhci,XHCI_OPT_CRCR_LO,crcr_lo | xhci->cmds.cs);
    xhci_write_opt(xhci,XHCI_OPT_CRCR_HI,crcr_hi);

    uint32_t hcs2 = xhci_read_cap(xhci,XHCI_CAP_HCSPARAM2);
    int spb = GET_HCSP2_MAX_SC_BUF(hcs2);
    if (spb != 0)
    {
        uint64_t *spba;
        status = pmalloc(sizeof(uint64_t) * spb,64,4096,&spba);
        if (ERROR(status))
        {
            pr_log("\3 Failed to alloc spba.\n");
            goto fail;
        }
        int i;
        for (i = 0;i < spb;i++)
        {
            void *pad = NULL;
            status = pmalloc(XHCI_PAGE_SIZE,XHCI_PAGE_SIZE,XHCI_PAGE_SIZE,&pad);
            if (ERROR(status))
            {
                pr_log("\3 Failed to alloc sctatch pad buf.\n");
                while (--i >= 0)
                {
                    pfree(((uint64_t**)KADDR_P2V(spba))[i]);
                }
                pfree(spba);
                goto fail;
            }
            ((uint64_t**)KADDR_P2V(spba))[i] = pad;
        }
        xhci->devs[0].ptr_lo = (uint64_t)spba & 0xffffffff;
        xhci->devs[0].ptr_hi = (uint64_t)spba >> 32;
    }
    xhci_run(xhci);

    xhci_check_ports(xhci);
    // int count = xhci_check_ports(xhci);
    // if (count)
    // {
    //     return;
    // }
    // while (1) continue;
    return;

fail:
    pfree(cmds);
    pfree(evts);
    pfree(eseg);
    pfree(devs);

    return;
}

PUBLIC status_t xhci_setup(void)
{
    int number_of_xhci;
    number_of_xhci = pci_dev_count(0x0c,0x03,0x30);
    pci_device_t *pci;
    int i;
    for (i = 0;i < number_of_xhci;i++)
    {
        pci = pci_dev_match(0x0c,0x03,0x30,i);
        addr_t mmio_base = pci_dev_read_bar(pci,0);
        page_map(
            (uint64_t*)KERNEL_PAGE_DIR_TABLE_POS,
            (void*)mmio_base,
            (void*)KADDR_P2V(mmio_base));
        if ((pci_dev_config_read(pci,0) & 0xffff) == 0x8086)
        {
            switch_to_xhci(pci);
        }
        xhci_t *xhci = NULL;
        xhci_controller_setup(KADDR_P2V(mmio_base),&xhci);
        xhci->usb.pci = pci;

        // task_start(
        //     "configure xhci",
        //     DEFAULT_PRIORITY,
        //     4096,
        //     configure_xhci,(addr_t)xhci);
        configure_xhci(xhci);
    }
    return K_SUCCESS;
}