/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/pci.h>      // pci_device_t,pci functions
#include <device/usb/xhci.h> // xhci_t,xhci registers
#include <mem/mem.h>         // pmalloc
#include <device/pic.h>      // eoi
#include <intr.h>            // register_handle
#include <kernel/syscall.h>  // sys_send_recv,inform_intr
#include <lib/list.h>        // OFFSET
#include <service.h>         // TICK_SLEEP
#include <std/string.h>      // strncmp,memset

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

PRIVATE void xhci_read_xecp(xhci_t *xhci)
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
        if (GET_FIELD(cap,XECP_CAP_ID) == 0x01)
        {
            if (GET_FIELD(cap,XECP_CAP_SPEC) & 1)
            {
                pr_log("\1 xHCI is bios owner. switch to this system.\n");
                cap |=  (1 << 24); // HC OS owned.
                cap &= ~(1 << 16); // HC Bios owned.
                xcap->cap = cap;
            }
            else
            {
                pr_log("\1 xHCI is OS owned.\n");
            }
        }
        if (GET_FIELD(cap,XECP_CAP_ID) == 0x02)
        {
            name = xcap->data[0];
            ports = xcap->data[1];
            uint8_t major = GET_FIELD(cap,XECP_SUP_MAJOR);
            uint8_t minor = GET_FIELD(cap,XECP_SUP_MINOR);
            uint8_t count = (ports >> 8) & 0xff;
            uint8_t start = (ports >> 0) & 0xff;
            pr_log("\1 xHCI protocol %c%c%c%c"
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

PRIVATE void xhci_reset(xhci_t *xhci)
{
    uint32_t usbcmd = xhci_read_opt(xhci,XHCI_OPT_USBCMD);
    usbcmd |= USBCMD_HCRST;
    xhci_write_opt(xhci,XHCI_OPT_USBCMD,usbcmd);
    // Wait 1ms
    message_t msg;
    msg.type = TICK_SLEEP;
    msg.m3.l1 = 1;
    sys_send_recv(NR_BOTH,TICK,&msg);
    while((xhci_read_opt(xhci,XHCI_OPT_USBCMD) & USBCMD_HCRST)) continue;
    while((xhci_read_opt(xhci,XHCI_OPT_USBSTS) & USBSTS_CNR))   continue;
    return;
}

PRIVATE void xhci_start(xhci_t *xhci)
{
    if (xhci->max_slots > XHCI_MAX_SLOTS)
    {
        xhci->max_slots = XHCI_MAX_SLOTS;
    }

    // Program the Max Device Slots Enabled field in the CONFIG
    // register to enable the device slots that system software
    // is going to use.
    xhci_write_opt(xhci,XHCI_OPT_CONFIG,xhci->max_slots);
    uint32_t hcsp2 = xhci_read_cap(xhci,XHCI_CAP_HCSPARAM2);
    xhci->scrath_chapad_count = GET_HCSP2_MAX_SC_BUF(hcsp2);
    if (xhci->scrath_chapad_count > XHCI_MAX_SCRATCHPADS)
    {
        pr_log("\3 invalid number of scrath chapad: %d.\n",
               xhci->scrath_chapad_count);
        return;
    }

    // xhci_write_opt(xhci,XHCI_OPT_USBSTS,xhci_read_opt(xhci,XHCI_OPT_USBSTS));
    xhci_write_opt(xhci,XHCI_OPT_DNCTRL,0);

    uint8_t* buf;
    status_t status;
    status = pmalloc(sizeof(*xhci->dev_cxt_arr),&buf);
    if (ERROR(status))
    {
        pr_log("\3 Can not alloc addr for DCBA.\n");
        return;
    }
    xhci->dev_cxt_arr = (xhci_device_cxt_arr_t*)buf;

    xhci->dev_cxt_arr->base_addr[0] =
                         (addr_t)buf + OFFSET(xhci_device_cxt_arr_t,scratchpad);
    uint32_t i;
    for (i = 0;i < xhci->scrath_chapad_count;i++)
    {
        void *buf_arr;
        status = pmalloc(4096,&buf_arr);
        if (ERROR(status))
        {
            pr_log("\3 Can not alloc memory for scratchpad_buf_arr[%d].\n",i);
            return;
        }
        xhci->dev_cxt_arr->scratchpad[i] = (phy_addr_t)buf_arr;
        memset(KADDR_P2V(xhci->dev_cxt_arr->scratchpad[i]),0,4096);
    }

    // Program the Device Context Base Address Array Point (DCBAPP) register
    // with a 64-bit address pointing to where the Device Context Base Address
    // Array is located.
    xhci_write_opt(xhci,XHCI_OPT_DCBAPP_LO,(phy_addr_t)buf & 0xffffffff);
    xhci_write_opt(xhci,XHCI_OPT_DCBAPP_HI,(phy_addr_t)buf >> 32);

    // Event Ring

    // Allocate and initialize the Event Ring Segment
    status = pmalloc(sizeof(*xhci->erst)
                     + (XHCI_MAX_COMMANDS + XHCI_MAX_EVENTS)
                     * sizeof(xhci_trb_t),
                     &buf);

    if (ERROR(status))
    {
        pr_log("\3 Can not alloc addr for DCBA.\n");
        return;
    }
    addr_t addr = (addr_t)KADDR_P2V(buf);
    xhci->erst = (xhci_erst_t*)addr;
    memset(xhci->erst,0,   sizeof(*xhci->erst)
                       + (XHCI_MAX_COMMANDS + XHCI_MAX_EVENTS)
                       * sizeof(xhci_trb_t));

    // Allocate the Event Ring Segment Table (ERST)
    // Initialize ERST table entries to point to and to define
    // the size (in TRBs) of the respective Event Ring Segment.
    xhci->erst->rs_addr = (addr_t)buf + sizeof(xhci_erst_t);
    xhci->erst->rs_size = XHCI_MAX_EVENTS;
    xhci->erst->rsvdz = 0;

    addr += sizeof(xhci_erst_t);
    xhci->event_ring.ring = (xhci_trb_t*)addr;
    addr += XHCI_MAX_EVENTS * sizeof(xhci_trb_t) + 63;
    addr &= ~63ULL; // 64-bit align
    xhci->command_ring.ring = (xhci_trb_t*)addr;

    // ERSTSZ
    // Program the Interrupter Event Ring Segment Table Size
    // (ERSTSZ) register with the number of segments
    // described by the Event Ring Segment Table.
    pr_log("\1 Setting ERST size.\n");
    xhci_write_run(xhci,XHCI_IRS_ERSTSZ(0),ERSTSZ_SET(1));

    // ERDP
    // Program the Interrupter Event Ring Dequeue Pointer (ERDP)
    // register with the starting address of the first segment
    // described by the Event Ring Segment Table.
    pr_log("\1 Setting ERDP: %p.\n",xhci->erst->rs_addr);
    xhci_write_run(xhci,XHCI_IRS_ERDP_LO(0),xhci->erst->rs_addr & 0xffffffff);
    xhci_write_run(xhci,XHCI_IRS_ERDP_HI(0),xhci->erst->rs_addr >> 32);

    // Set ERSTBA
    // Program the Interrupter Event Ring Segment Table Base
    // Address (ERSTBA) register with a 64-bit address pointer to where the
    // Event Ring Segment Table is located.
    pr_log("\1 Setting ERST base address: %p\n",buf);
    xhci_write_run(xhci,XHCI_IRS_ERSTBA_LO(0),(phy_addr_t)buf & 0xffffffff);
    xhci_write_run(xhci,XHCI_IRS_ERSTBA_HI(0),(phy_addr_t)buf >> 32);

    buf += sizeof(xhci_erst_t) + XHCI_MAX_EVENTS * sizeof(xhci_trb_t) + 63;
    buf = (uint8_t*)((phy_addr_t)buf & ~63ULL); // 64-bit align

    // Define the Command Ring Dequeue Pointer by programming the Command
    // Ring Control Register with a 64-bit address pointing to the starting
    // address of the first TRB of Command Ring.
    if ((xhci_read_opt(xhci,XHCI_OPT_CRCR_LO) & CRCR_CRR) != 0)
    {
        pr_log("\1 Command Ring is Running. stop it.\n");
        xhci_write_opt(xhci,XHCI_OPT_CRCR_LO,CRCR_CS);
        xhci_write_opt(xhci,XHCI_OPT_CRCR_HI,      0);
        xhci_write_opt(xhci,XHCI_OPT_CRCR_LO,CRCR_CA);
        xhci_write_opt(xhci,XHCI_OPT_CRCR_HI,      0);
        while(xhci_read_opt(xhci,XHCI_OPT_CRCR_LO) & CRCR_CRR) continue;
    }
    pr_log("\1 Setting CRCR = %p.\n",buf);
    uint32_t crcr_lo = ((phy_addr_t)buf & 0xffffffff) | CRCR_RCS;
    uint32_t crcr_hi = (phy_addr_t)buf >> 32;
    xhci_write_opt(xhci,XHCI_OPT_CRCR_LO,crcr_lo);
    xhci_write_opt(xhci,XHCI_OPT_CRCR_HI,crcr_hi);

    xhci->command_ring.ring[XHCI_MAX_COMMANDS - 1].addr = (phy_addr_t)buf;

    // Set interrupt rate.
    // Initializing the Interval field of the Interrupt Moderation register
    // with the target interrupt moderation rate.
    // interrupts/sec = 1/(250 × (10^-9 sec) × Interval)
    pr_log("\1 Setting interrupt rate.\n");
    xhci_write_run(xhci,XHCI_IRS_IMOD(0),0x000003f8);

    // Enable the Interrupter by writing a '1' to the Interrupt Enable
    // (IE) field of the Interrupter Management register
    uint32_t iman = xhci_read_run(xhci,XHCI_IRS_IMAN(0));
    iman |= IMAN_INTR_EN;
    xhci_write_run(xhci,XHCI_IRS_IMAN(0),iman);

    // Enable system bus interrupt generation by writing a '1' to the
    // Interrupter Enable (INTE) flag of the USBCMD register

    // Write the USBCMD to turn the host controller ON via setting the
    // Run/Stop (R/S) bit to '1'. This operation allows the xHC to begin accepting
    // doorbell references.
    xhci_write_opt(xhci,XHCI_OPT_USBCMD,USBCMD_RUN | USBCMD_INTE | USBCMD_HSEE);

    // wait for start up.
    while(xhci_read_opt(xhci,XHCI_OPT_USBSTS) & USBSTS_HCH) continue;

    pr_log("xHCI Cap Regs:\n");
    pr_log("CAPLENGTH  %08x\n",xhci_read_cap(xhci,XHCI_CAP_CAPLENGTH));
    pr_log("HCIVERSION %08x\n",xhci_read_cap(xhci,XHCI_CAP_HCIVERSION));
    pr_log("HCSPARAM1  %08x\n",xhci_read_cap(xhci,XHCI_CAP_HCSPARAM1));
    pr_log("HCSPARAM2  %08x\n",xhci_read_cap(xhci,XHCI_CAP_HCSPARAM2));
    pr_log("HCSPARAM3  %08x\n",xhci_read_cap(xhci,XHCI_CAP_HCSPARAM3));
    pr_log("HCCPARAM1  %08x\n",xhci_read_cap(xhci,XHCI_CAP_HCCPARAM1));
    pr_log("DBOFF      %08x\n",xhci_read_cap(xhci,XHCI_CAP_DBOFF));
    pr_log("RSTOFF     %08x\n",xhci_read_cap(xhci,XHCI_CAP_RSTOFF));
    pr_log("HCCPARAM2  %08x\n",xhci_read_cap(xhci,XHCI_CAP_HCCPARAM2));

    pr_log("xHCI Opt Regs:\n");
    pr_log("USBCMD:    %08x\n",xhci_read_opt(xhci,XHCI_OPT_USBCMD));
    pr_log("USBSTS:    %08x\n",xhci_read_opt(xhci,XHCI_OPT_USBSTS));
    pr_log("PAGESIZE:  %08x\n",xhci_read_opt(xhci,XHCI_OPT_PAGESIZE));
    pr_log("DNCTRL:    %08x\n",xhci_read_opt(xhci,XHCI_OPT_DNCTRL));
    pr_log("CRCR LO:   %08x\n",xhci_read_opt(xhci,XHCI_OPT_CRCR_LO));
    pr_log("CRCR HI:   %08x\n",xhci_read_opt(xhci,XHCI_OPT_CRCR_HI));
    pr_log("DCBAPP LO: %08x\n",xhci_read_opt(xhci,XHCI_OPT_DCBAPP_LO));
    pr_log("DCBAPP HI: %08x\n",xhci_read_opt(xhci,XHCI_OPT_DCBAPP_HI));
    pr_log("CONFIG:    %08x\n",xhci_read_opt(xhci,XHCI_OPT_CONFIG));
    return;
}

PRIVATE void intr_xHCI_handler(intr_stack_t *stack)
{
    eoi(stack->nr);
    inform_intr(USB_SRV);
    return;
}

PUBLIC status_t xhci_setup(void)
{
    uint32_t number_of_xhci = pci_dev_count(0x0c,0x03,0x30);
    if (number_of_xhci == 0)
    {
        pr_log("\3xhci_dev not found.\n");
        return K_NOT_FOUND;
    }

    register_handle(IRQ_XHCI,intr_xHCI_handler);

    pr_log("\1 this machine has %d xHCI in total.\n",number_of_xhci);
    addr_t addr;
    status_t status = pmalloc(sizeof(xhci_set[0]) * number_of_xhci,&addr);
    if (ERROR(status))
    {
        pr_log("\3 Can not alloc memory for xHCI set.\n");
        return K_NOMEM;
    }
    xhci_set = KADDR_P2V(addr);
    pr_log("\1 xhci_set at: %p.\n",xhci_set);
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
        pr_log("\1 xhci_dev vendor ID: %04x,Device ID: %04x.\n",
                pci_dev_read_vendor_id(device),
                (pci_dev_config_read(device,0) >> 16) & 0xffff);

        // Initialization MMIO map
        addr_t xhci_mmio_base = pci_dev_read_bar(device,0);
        pr_log("\1 xhci mmio base: %p\n",xhci_mmio_base);

        page_map((uint64_t*)KERNEL_PAGE_DIR_TABLE_POS,
                 (void*)xhci_mmio_base,
                 (void*)KADDR_P2V(xhci_mmio_base));

        xhci_mmio_base      = (addr_t)KADDR_P2V(xhci_mmio_base);
        uint8_t cap_length  = *(uint8_t*)xhci_mmio_base & 0xff;

        xhci->mmio_base     = (void*)xhci_mmio_base;
        xhci->cap_regs      = (uint8_t*)xhci_mmio_base;
        xhci->opt_regs      = xhci->cap_regs + cap_length;
        xhci->run_regs      = xhci->cap_regs + xhci_read_cap(xhci,XHCI_CAP_RSTOFF);
        xhci->doorbell_regs = xhci->cap_regs + (xhci_read_cap(xhci,XHCI_CAP_DBOFF) & 3);

        xhci->max_ports = GET_FIELD(xhci_read_cap(xhci,XHCI_CAP_HCSPARAM1),HCSP1_MAX_PORTS);
        xhci->max_slots = GET_FIELD(xhci_read_cap(xhci,XHCI_CAP_HCSPARAM1),HCSP1_MAX_SLOTS);

        xhci->msi_vector = IRQ_XHCI;

        xhci->event_ring.cycle_bit   = 1;
        xhci->command_ring.cycle_bit = 1;
        xhci->event_ring.enqueue_index   = 0;
        xhci->event_ring.dequeue_index   = 0;
        xhci->command_ring.enqueue_index = 0;
        xhci->command_ring.dequeue_index = 0;

        // pci interrupt_line
        // this register corresponds to
        // the PIC IRQ numbers 0-15 (and not I/O APIC IRQ numbers)
        // and a value of 0xFF defines no connection
        uint32_t interrupt_line = pci_dev_config_read(device,0x3c) & 0xff;

        // pci interrupt_pin
        // Specifies which interrupt pin the device uses.
        // Where a value of 0x1 is INTA#, 0x2 is INTB#, 0x3 is INTC#, 0x4 is INTD#,
        // and 0x0 means the device does not use an interrupt pin.

        // uint32_t interrupt_pin  = (pci_dev_config_read(device,0x3c) >> 8) & 0xff;

        if (interrupt_line == 0xff)
        {
            pr_log("\3 xHCI was assigned an invalid IRQ.\n");
        }

        // configure and enable MSI
        configure_msi(device,1,0,interrupt_line,0);
        status = pci_dev_configure_msi(device,IRQ_XHCI,1);
        if (ERROR(status))
        {
            pr_log("\3 xHCI: can not configure MSI.\n");
        }
        pci_dev_enable_msi(device);

        status = xhci_init(xhci);
        if (ERROR(status))
        {
            pr_log("\3 Can not init xhci %d\n",i);
            return status;
        }
    }
    return K_SUCCESS;
}

PUBLIC status_t xhci_init(xhci_t *xhci)
{
    pr_log("\1 xHCI init.\n");

    uint16_t hci_ver = GET_FIELD(xhci_read_cap(xhci,XHCI_CAP_HCIVERSION),HCIVERSION);
    pr_log("\1 interface version: %04x\n",hci_ver);
    if (hci_ver < 0x0090 || hci_ver > 0x0120)
    {
        pr_log("\3 Unsupported HCI version.\n");
        return K_NOSUPPORT;
    }

    // halt xHCI
    xhci_halt(xhci);

    // reset xHCI
    xhci_reset(xhci);

    uint32_t hccp1 = xhci_read_cap(xhci,XHCI_CAP_HCCPARAM1);
    pr_log("\1 HCCPARAM1: %08x\n",hccp1);
    if (hccp1 == 0xffffffff)
    {
        return K_INVAILD_PARAM;
    }
    xhci->cxt_size    = GET_FIELD(hccp1,HCCP1_CSZ) == 1 ? 64 : 32;
    xhci->xecp_offset = GET_FIELD(hccp1,HCCP1_XECP) << 2;
    if (xhci->xecp_offset)
    {
        xhci_read_xecp(xhci);
    }
    xhci_start(xhci);
    return K_SUCCESS;
}

PUBLIC bool xhci_ring_busy(xhci_ring_t *ring)
{
    return ring->enqueue_index != ring->dequeue_index;
}

PRIVATE void xhci_trb_queue(xhci_ring_t *ring,xhci_trb_t *trb)
{
    uint8_t i,cycle_bit;
    uint32_t temp;
    i = ring->enqueue_index;
    cycle_bit = ring->cycle_bit;
    ring->ring[i].addr   = trb->addr;
    ring->ring[i].status = trb->status;

    temp = trb->flags;

    cycle_bit ? (temp |= TRB_3_CYCLE_BIT) : (temp &= TRB_3_CYCLE_BIT);
    temp &= ~TRB_3_TC_BIT;
    ring->ring[i].flags = temp;
    i++;
    if (i >= XHCI_MAX_COMMANDS - 1)
    {
        temp = TRB_3_TYPE(TRB_TYPE_LINK) | TRB_3_TC_BIT;
        if (cycle_bit)
        {
            temp |= TRB_3_CYCLE_BIT;
        }
        ring->ring[i].flags = temp;
        i = 0;
        cycle_bit ^= 1;
    }
    ring->enqueue_index     = i;
    ring->cycle_bit = cycle_bit;
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
    xhci_trb_queue(&xhci->command_ring,trb);
    xhci_doorbell_ring(xhci,0,0);
    return K_SUCCESS;
}