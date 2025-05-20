/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/pci.h>           // pci_device_t,pci functions
#include <mem/mem.h>              // pmalloc
#include <device/pic.h>           // eoi
#include <intr.h>                 // register_handle
#include <kernel/syscall.h>       // sys_send_recv,inform_intr
#include <lib/list.h>             // OFFSET
#include <service.h>              // TICK_SLEEP
#include <std/string.h>           // strncmp,memset

#include <device/usb/xhci.h>      // xhci_t,xhci_ring_t
#include <device/usb/xhci_regs.h> // xhci registers
#include <device/usb/xhci_mem.h>  // xhci memory alignment and boundary
#include <device/usb/xhci_trb.h>  // xhci_trb_t
#include <device/usb/usb.h>       // process_event

#include <log.h>

PUBLIC xhci_t *xhci_set;

extern volatile uint64_t global_ticks;

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

PRIVATE void xhci_halt(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci,XHCI_OPT_USBCMD);
    usbcmd &= ~USBCMD_RUN;
    xhci_write_opt(xhci,XHCI_OPT_USBCMD,usbcmd);
    while((xhci_read_opt(xhci,XHCI_OPT_USBSTS) & USBSTS_HCH) == 0) continue;
    return;
}

PRIVATE status_t xhci_reset(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci,XHCI_OPT_USBCMD);
    usbcmd |= USBCMD_HCRST;
    xhci_write_opt(xhci,XHCI_OPT_USBCMD,usbcmd);

    while((xhci_read_opt(xhci,XHCI_OPT_USBCMD) & USBCMD_HCRST)) continue;
    while((xhci_read_opt(xhci,XHCI_OPT_USBSTS) & USBSTS_CNR))   continue;
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

// PRIVATE void xhci_ack_intr(xhci_t *xhci,uint8_t intr)
// {
//     uint32_t iman = xhci_read_run(xhci,XHCI_IRS_IMAN(intr));
//     iman |= IMAN_INTR_PENDING;
//     xhci_write_run(xhci,XHCI_IRS_IMAN(intr),iman);

//     xhci_write_opt(xhci,XHCI_OPT_USBSTS,USBSTS_EINT);
//     return;
// }

// PRIVATE void intr_xHCI_handler(intr_stack_t *stack)
// {
//     /// TODO: Process event
//     /// TODO: xhci_ack_intr(0);
//     send_eoi(stack->int_vector);
//     inform_intr(USB_SRV);
//     return;
// }

PRIVATE void xhci_parse_cap_reg(xhci_t *xhci)
{
    xhci->cap_regs = xhci->mmio_base;
    xhci->cap_len  = xhci_read_cap(xhci,XHCI_CAP_CAPLENGTH);

    uint32_t hcsp1 = xhci_read_cap(xhci,XHCI_CAP_HCSPARAM1);
    xhci->max_slots = GET_FIELD(hcsp1,HCSP1_MAX_SLOTS);
    xhci->max_intr  = GET_FIELD(hcsp1,HCSP1_MAX_INTR);
    xhci->max_ports = GET_FIELD(hcsp1,HCSP1_MAX_PORTS);

    uint32_t hcsp2 = xhci_read_cap(xhci,XHCI_CAP_HCSPARAM2);
    xhci->isoc_sched_thre       = GET_FIELD(hcsp2,HCSP2_IST);
    xhci->erst_max              = GET_FIELD(hcsp2,HCSP2_ERST_MAX);
    xhci->max_scratchpad_buffer = GET_HCSP2_MAX_SC_BUF(hcsp2);

    uint32_t hccp1 = xhci_read_cap(xhci,XHCI_CAP_HCCPARAM1);
    xhci->ac64 = GET_FIELD(hccp1,HCCP1_AC64);
    xhci->bnc  = GET_FIELD(hccp1,HCCP1_BNC);
    xhci->csz  = GET_FIELD(hccp1,HCCP1_CSZ);
    xhci->ppc  = GET_FIELD(hccp1,HCCP1_PPC);
    xhci->pind = GET_FIELD(hccp1,HCCP1_PIND);
    xhci->lhrc = GET_FIELD(hccp1,HCCP1_LHRC);
    xhci->xecp_offset = GET_FIELD(hccp1,HCCP1_XECP) << 2;

    xhci->opt_regs      = xhci->cap_regs + xhci->cap_len;
    xhci->doorbell_regs = xhci->cap_regs + xhci_read_cap(xhci,XHCI_CAP_DBOFF);
    xhci->run_regs      = xhci->cap_regs + xhci_read_cap(xhci,XHCI_CAP_RSTOFF);

    // Log
    pr_log("\1 xHCI: Capability registers (%p)\n",xhci->cap_regs);
    pr_log("    Legnth                 %d\n",xhci->cap_len);
    pr_log("    Max Slots              %d\n",xhci->max_slots);
    pr_log("    Max Interrupts         %d\n",xhci->max_intr);
    pr_log("    Max Ports              %d\n",xhci->max_ports);

    pr_log("    IST                    %d\n",xhci->isoc_sched_thre);
    pr_log("    ERST Max               %d\n",xhci->erst_max);
    pr_log("    Max SC buf             %d\n",xhci->max_scratchpad_buffer);

    pr_log("    64-bits addressing     %s\n",xhci->ac64 ? "yes" : "no");
    pr_log("    bandwidth negotiation  %d\n",xhci->bnc);
    pr_log("    64-byte context size   %s\n",xhci->csz ? "yes" : "no");
    pr_log("    Port Power Control     %d\n",xhci->ppc);
    pr_log("    Port Indicators        %d\n",xhci->pind);
    pr_log("    Light Reset Capability %d\n",xhci->lhrc);
    return;
}

PRIVATE void xhci_parse_xecp_reg(xhci_t *xhci)
{
    uint32_t offset;
    addr_t addr = (addr_t)xhci->cap_regs + xhci->xecp_offset;
    do
    {
        xhci_xcap_regs_t *xcap = (void*)addr;
        uint32_t cap = xcap->cap;
        uint32_t name;
        uint32_t ports;
        offset = GET_FIELD(cap,XECP_NEXT_POINT) << 2;
        addr += offset;

        // Legacy Support
        if (GET_FIELD(cap,XECP_CAP_ID) == 0x01)
        {
            if (GET_FIELD(cap,XECP_CAP_SPEC) & 1)
            {
                pr_log(" xHCI is bios owner. switch to this system.\n");
                cap |=  (1 << 24); // HC OS owned.
                cap &= ~(1 << 16); // HC Bios owned.
                xcap->cap = cap;
            }
        }

        // Supported Protocol
        if (GET_FIELD(cap,XECP_CAP_ID) == 0x02)
        {
            name = xcap->data[0];
            ports = xcap->data[1];
            uint8_t major = GET_FIELD(cap,XECP_SUP_MAJOR);
            uint8_t minor = GET_FIELD(cap,XECP_SUP_MINOR);
            uint8_t count = (ports >> 8) & 0xff;
            uint8_t start = (ports >> 0) & 0xff;
            pr_log(" xHCI protocol %c%c%c%c"
                   " %x.%02x ,%d ports (offset %d), def %x\n",
                    (name >>  0) & 0xff,
                    (name >>  8) & 0xff,
                    (name >> 16) & 0xff,
                    (name >> 24) & 0xff,
                    major,minor,count,start,ports >> 16);
            if (strncmp((const char*)&name,"USB ",4) != 0)
            {
                continue;
            }
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
    } while (offset > 0);
    return;
}

PRIVATE status_t xhci_setup_dcbaa(xhci_t *xhci)
{
    size_t dcbaa_size = sizeof(addr_t) * (xhci->max_slots + 1);
    uint64_t *dcbaa;
    uint64_t *dcbaa_vaddr;
    status_t status;
    status = pmalloc(dcbaa_size,
                     XHCI_DEVICE_CONTEXT_ALIGNMENT,
                     XHCI_DEVICE_CONTEXT_BOUNDARY,
                     &dcbaa);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc memory for DCBAA.\n");
        return status;
    }
    xhci->dcbaa = KADDR_P2V(dcbaa);

    status = pmalloc(dcbaa_size,0,0,&dcbaa_vaddr);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc memory for DCBAA vaddr.\n");
        return status;
    }
    xhci->dcbaa_vaddr = KADDR_P2V(dcbaa_vaddr);

    if (xhci->max_scratchpad_buffer > 0)
    {
        uint64_t *sc_buffer_array;
        status = pmalloc(
            xhci->max_scratchpad_buffer * sizeof(uint64_t),
            XHCI_DEVICE_CONTEXT_ALIGNMENT,
            XHCI_DEVICE_CONTEXT_BOUNDARY,
            &sc_buffer_array);
        if (ERROR(status))
        {
            pr_log("Faild to alloc memory for sc_buffer.\n");
            return status;
        }

        // Create Scratchpad pages
        uint8_t i;
        for (i = 0;i < xhci->max_scratchpad_buffer;i++)
        {
            void *sc;
            status = pmalloc(
                XHCI_PG_SZIE,
                XHCI_SCRATCHPAD_BUFFER_ARRAY_ALIGNMENT,
                XHCI_SCRATCHPAD_BUFFER_ARRAY_BOUNDARY,
                &sc);
            if (ERROR(status))
            {
                pr_log("\3 Failed to alloc sc buffer page.\n");
                return status;
            }
            ((uint64_t*)KADDR_P2V(sc_buffer_array))[i] = (uint64_t)sc;
        }
        xhci->dcbaa[0] = (uint64_t)sc_buffer_array;
        xhci->dcbaa_vaddr[0] = (uint64_t)KADDR_P2V(sc_buffer_array);
    }

    // Set DCBAA in the operational registers
    uint32_t dcbaa_lo = (phy_addr_t)dcbaa & 0xffffffff;
    uint32_t dcbaa_hi = (phy_addr_t)dcbaa >> 32;
    xhci_write_opt(xhci,XHCI_OPT_DCBAAP_LO,dcbaa_lo);
    xhci_write_opt(xhci,XHCI_OPT_DCBAAP_HI,dcbaa_hi);
    return K_SUCCESS;
}

PRIVATE status_t xhci_setup_command_ring(xhci_t *xhci)
{
    xhci->command_ring.ring.trb_count = XHCI_COMMAND_RING_TRB_COUNT;
    xhci->command_ring.ring.cycle_bit = 1;
    xhci->command_ring.ring.enqueue   = 0;
    size_t ring_size =   sizeof(xhci->command_ring.ring.trbs[0])
                       * xhci->command_ring.ring.trb_count;
    void *cr;
    status_t status;
    status = pmalloc(
        ring_size,
        XHCI_COMMAND_RING_SEGMENTS_ALIGNMENT,
        XHCI_COMMAND_RING_SEGMENTS_BOUNDARY,
        &cr);
    if (ERROR(status))
    {
        pr_log("\3 Failed alloc memory for CR.\n");
        return status;
    }
    xhci->command_ring.ring.trbs       = KADDR_P2V(cr);
    xhci->command_ring.ring.trbs_paddr = (phy_addr_t)cr;
    // Set link trb
    uint8_t cycle_bit = xhci->command_ring.ring.cycle_bit;
    xhci->command_ring.ring.trbs[xhci->command_ring.ring.trb_count - 1].addr  =
            (uint64_t)cr;
    xhci->command_ring.ring.trbs[xhci->command_ring.ring.trb_count - 1].flags =
            SET_FIELD(TRB_3_TC_BIT | cycle_bit,TRB_3_TYPE,TRB_TYPE_LINK);
    // write CRCR
    uint32_t crcr_lo = ((phy_addr_t)cr & 0xffffffff) | cycle_bit;
    uint32_t crcr_hi = (phy_addr_t)cr >> 32;
    xhci_write_opt(xhci,XHCI_OPT_CRCR_LO,crcr_lo);
    xhci_write_opt(xhci,XHCI_OPT_CRCR_HI,crcr_hi);
    return K_SUCCESS;
}

PRIVATE status_t xhci_configure_opt_reg(xhci_t *xhci)
{
    uint64_t pagesize = xhci_read_opt(xhci,XHCI_OPT_PAGESIZE);
    xhci->pagesize = (pagesize & 0xffff) << 12;

    // Enable device notifications
    xhci_write_opt(xhci,XHCI_OPT_DNCTRL,0xffff);

    // Configure USBCONFIG field
    xhci_write_opt(xhci,XHCI_OPT_CONFIG,xhci->max_slots);

    // Setup DCBAA
    xhci_setup_dcbaa(xhci);

    // Setup command ring and write CRCR
    xhci_setup_command_ring(xhci);

    pr_log("\1 xHCI Operational Registers (%p)\n",xhci->opt_regs);
    pr_log("    USBCMD    %08x\n",xhci_read_opt(xhci,XHCI_OPT_USBCMD));
    pr_log("    USBSTS    %08x\n",xhci_read_opt(xhci,XHCI_OPT_USBSTS));
    pr_log("    PAGESIZE  %08x\n",xhci_read_opt(xhci,XHCI_OPT_PAGESIZE));
    pr_log("    DNCTRL    %08x\n",xhci_read_opt(xhci,XHCI_OPT_DNCTRL));
    pr_log("    CRCR LO   %08x\n",xhci_read_opt(xhci,XHCI_OPT_CRCR_LO));
    pr_log("    CRCR HI   %08x\n",xhci_read_opt(xhci,XHCI_OPT_CRCR_HI));
    pr_log("    DCBAAP LO %08x\n",xhci_read_opt(xhci,XHCI_OPT_DCBAAP_LO));
    pr_log("    DCBAAP HI %08x\n",xhci_read_opt(xhci,XHCI_OPT_DCBAAP_HI));
    pr_log("    CONFIG    %08x\n",xhci_read_opt(xhci,XHCI_OPT_CONFIG));

    return K_SUCCESS;
}

PRIVATE status_t xhci_setup_event_ring(xhci_t *xhci)
{
    xhci->event_ring.ring.trb_count     = XHCI_EVENT_RING_TRB_COUNT;
    xhci->event_ring.ring.cycle_bit     = 1;
    xhci->event_ring.ring.dequeue       = 0;
    xhci->event_ring.erst_count         = 1;

    size_t ring_size =   sizeof(xhci->event_ring.ring.trbs[0])
                       * xhci->event_ring.ring.trb_count;
    size_t erst_size =   sizeof(xhci_erst_t) * xhci->event_ring.erst_count;
    void *er;
    status_t status;
    status = pmalloc(
        ring_size,
        XHCI_EVENT_RING_SEGMENTS_ALIGNMENT,
        XHCI_EVENT_RING_SEGMENTS_BOUNDARY,
        &er);
    if (ERROR(status))
    {
        pr_log("\3 Failed alloc memory for ER.\n");
        return status;
    }
    xhci->event_ring.ring.trbs       = KADDR_P2V(er);
    xhci->event_ring.ring.trbs_paddr = (phy_addr_t)er;

    void *erst;
    status = pmalloc(
        erst_size,
        XHCI_EVENT_RING_SEGMENT_TABLE_ALIGNMENT,
        XHCI_EVENT_RING_SEGMENT_TABLE_BOUNDARY,
        &erst);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc memory for ERST.\n");
        return status;
    }
    xhci->event_ring.erst = KADDR_P2V(erst);

    xhci_erst_t erst_entry =
    {
        (xhci_trb_t*)xhci->event_ring.ring.trbs_paddr,
        xhci->event_ring.ring.trb_count,
        0,
    };
    xhci->event_ring.erst[0] = erst_entry;

    xhci_write_run(xhci,XHCI_IRS_ERSTSZ(0),ERSTSZ_SET(1));

    // ERDP
    uint32_t erdp_lo = (uint64_t)erst_entry.rs_addr & 0xffffffff;
    uint32_t erdp_hi = (uint64_t)erst_entry.rs_addr >> 32;
    xhci_write_run(xhci,XHCI_IRS_ERDP_LO(0),erdp_lo);
    xhci_write_run(xhci,XHCI_IRS_ERDP_HI(0),erdp_hi);

    // ERSTBA
    xhci_write_run(xhci,XHCI_IRS_ERSTBA_LO(0),(phy_addr_t)erst & 0xffffffff);
    xhci_write_run(xhci,XHCI_IRS_ERSTBA_HI(0),((phy_addr_t)erst) >> 32);
    return K_SUCCESS;
}

PRIVATE status_t xhci_configure_run_reg(xhci_t *xhci)
{
    // // Enable interrupts
    // uint32_t iman = xhci_read_run(xhci,XHCI_IRS_IMAN(0));
    // iman |= IMAN_INTR_ENABLE;
    // xhci_write_run(xhci,XHCI_IRS_IMAN(0),iman);
    status_t status = xhci_setup_event_ring(xhci);
    if (ERROR(status))
    {
        pr_log("\3 Failed to setup event ring.\n");
        return status;
    }
    // xhci_ack_intr(xhci,0);
    return K_SUCCESS;
}

PUBLIC status_t xhci_setup()
{
    uint32_t number_of_xhci = pci_dev_count(0x0c,0x03,0x30);
    if (number_of_xhci == 0)
    {
        pr_log("\3xhci_dev not found.\n");
        return K_NOT_FOUND;
    }

    // register_handle(IRQ_XHCI,intr_xHCI_handler);

    addr_t addr;
    status_t status;
    status = pmalloc(sizeof(xhci_set[0]) * number_of_xhci,0,0,&addr);
    if (ERROR(status))
    {
        pr_log("\3 Can not alloc memory for xHCI set.\n");
        return status;
    }
    xhci_set = KADDR_P2V(addr);
    // init for each xhci
    uint32_t i;
    for (i = 0;i < number_of_xhci;i++)
    {
        xhci_t *xhci = &xhci_set[i];
        memset(xhci,0,sizeof(*xhci));

        pci_device_t *device = pci_dev_match(0x0c,0x03,0x30,i);
        if ((pci_dev_config_read(device,0) & 0xffff) == 0x8086)
        {
            switch_to_xhci(device);
        }

        // Initialization MMIO map
        addr_t xhci_mmio_base = pci_dev_read_bar(device,0);
        pr_log("\1 xhci mmio base: %p\n",xhci_mmio_base);

        page_map(
            (uint64_t*)KERNEL_PAGE_DIR_TABLE_POS,
            (void*)xhci_mmio_base,
            (void*)KADDR_P2V(xhci_mmio_base));

        xhci->mmio_base = KADDR_P2V(xhci_mmio_base);

        // Parse xHCI Capability Register space
        xhci_parse_cap_reg(xhci);
        xhci_parse_xecp_reg(xhci);

        status = pmalloc(sizeof(*xhci->connected_devices) * xhci->max_slots,
                         0,
                         0,
                         &addr);
        if (ERROR(status))
        {
            pr_log("\3 Faild to alloc memory for xhci->connected_devices.\n");
            return status;
        }
        xhci->connected_devices = (xhci_device_t**)KADDR_P2V(addr);
        int slot_id;
        for (slot_id = 0;slot_id < xhci->max_slots;slot_id++)
        {
            xhci->connected_devices[i] = NULL;
        }
        // Helt and Reset Host Controller
        xhci_halt(xhci);
        status = xhci_reset(xhci);
        if (ERROR(status))
        {
            pr_log("\3 Failed to reset host controller.\n");
        }

        xhci_configure_opt_reg(xhci);
        xhci_configure_run_reg(xhci);

        void *pc_data;
        size_t pc_event_size = sizeof(xhci_port_connection_event_t);
        status = pmalloc(pc_event_size * 64,0,0,&pc_data);
        if (ERROR(status))
        {
            pr_log("\3 Out of memory.\n");
        }
        pc_data = KADDR_P2V(pc_data);
        init_fifo(&xhci->port_connection_events,pc_data,pc_event_size,64);

        void *psc_data; // port status change events
        void *cc_data;  // command completion events
        void *tc_data;  // transfer completion events
        size_t item_sz = sizeof(xhci_trb_t);
        size_t fifo_sz = xhci->event_ring.ring.trb_count;
        status = pmalloc(item_sz * fifo_sz,0,0,&psc_data);
        if (ERROR(status))
        {
            pr_log("\3 Out of memory.\n");
        }
        psc_data = KADDR_P2V(psc_data);
        init_fifo(&xhci->port_status_change_events,psc_data,item_sz,fifo_sz);

        status = pmalloc(item_sz * 64,0,0,&cc_data);
        if (ERROR(status))
        {
            pr_log("\3 Out of memory.\n");
        }
        cc_data = KADDR_P2V(cc_data);
        init_fifo(&xhci->command_completion_events,cc_data,item_sz,fifo_sz);

        status = pmalloc(item_sz * 64,0,0,&tc_data);
        if (ERROR(status))
        {
            pr_log("\3 Out of memory.\n");
        }
        tc_data = KADDR_P2V(tc_data);
        init_fifo(&xhci->transfer_completion_events,tc_data,item_sz,fifo_sz);
    }
    return K_SUCCESS;
}

PRIVATE status_t xhci_start_host_controller(xhci_t *xhci)
{
    uint32_t usbcmd;
    usbcmd = xhci_read_opt(xhci,XHCI_OPT_USBCMD);
    usbcmd |= USBCMD_RUN;
    // usbcmd |= USBCMD_INTE;
    usbcmd |= USBCMD_HSEE;
    xhci_write_opt(xhci,XHCI_OPT_USBCMD,usbcmd);

    // wait for start up.
    while(xhci_read_opt(xhci,XHCI_OPT_USBSTS) & USBSTS_HCH) continue;

    // Verify CNR bit clear
    uint32_t usbsts;
    usbsts = xhci_read_opt(xhci,XHCI_OPT_USBSTS);
    if (usbsts & USBSTS_CNR)
    {
        // Control not redy.
        return K_ERROR;
    }
    return K_SUCCESS;
}

PUBLIC status_t xhci_start()
{
    uint32_t number_of_xhci = pci_dev_count(0x0c,0x03,0x30);
    if (number_of_xhci == 0)
    {
        pr_log("\3xhci_dev not found.\n");
        return K_NOT_FOUND;
    }
    status_t status;
    uint32_t i;
    for (i = 0;i < number_of_xhci;i++)
    {
        xhci_t *xhci = &xhci_set[i];
        status = xhci_start_host_controller(xhci);
        if (ERROR(status))
        {
            pr_log("\3 %s Control not ready: %d.\n",__func__,i);
        }
    }
    return K_SUCCESS;
}

PUBLIC void xhci_trb_enqueue(xhci_ring_t *ring,xhci_trb_t *trb)
{
    trb->flags |= ring->cycle_bit;
    ring->trbs[ring->enqueue] = *trb;
    ring->enqueue++;
    if (ring->enqueue == ring->trb_count - 1)
    {
        uint32_t flags = 0;
        flags  = SET_FIELD(
            TRB_3_TC_BIT | ring->cycle_bit,
            TRB_3_TYPE,
            TRB_TYPE_LINK);
        ring->trbs[ring->enqueue].flags = flags;
        ring->enqueue   = 0;
        ring->cycle_bit = !ring->cycle_bit;
    }
    return;
}

PRIVATE void xhci_doorbell_ring(xhci_t *xhci,uint8_t slot,uint8_t endpoint)
{
    xhci_write_doorbell(
        xhci,
        XHCI_DOORBELL(slot),
        XHCI_DOORBELL_TARGET(endpoint) | XHCI_DOORBELL_STREAMID(0));
    return;
}

PUBLIC status_t xhci_submit_command(xhci_t *xhci,xhci_trb_t *trb)
{
    xhci_trb_enqueue(&xhci->command_ring.ring,trb);
    xhci_doorbell_ring(xhci,0,0);

    // Wait Host Controller Process the command.
    while (fifo_empty(&xhci->command_completion_events)) continue;
    // Completed
    fifo_read(&xhci->command_completion_events,trb);
    if (GET_FIELD(trb->status,TRB_2_COMP_CODE) != COMP_SUCCESS)
    {
        pr_log("\3 Command Error %d.\n",GET_FIELD(trb->status,TRB_2_COMP_CODE));
        return K_ERROR;
    }
    return K_SUCCESS;
}

PUBLIC status_t xhci_start_ctrl_ep_transfer(
    xhci_t *xhci,
    xhci_transfer_ring_t *ring,
    xhci_trb_t *trb)
{
    xhci_doorbell_ring(xhci,ring->doorbell_id,1); // 1: control endpoint ring.
    // Wait Host Controller Process the command.
    while (fifo_empty(&xhci->transfer_completion_events)) continue;
    // Completed
    fifo_read(&xhci->transfer_completion_events,trb);
    if (GET_FIELD(trb->status,TRB_2_COMP_CODE) != COMP_SUCCESS)
    {
        pr_log("\3 Command Error %d.\n",GET_FIELD(trb->status,TRB_2_COMP_CODE));
        return K_ERROR;
    }
    return K_SUCCESS;
}