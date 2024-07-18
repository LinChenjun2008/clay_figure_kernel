#include <kernel/global.h>
#include <device/pci.h>
#include <device/usb/hci/xhci.h>
#include <mem/mem.h>
#include <io.h>
#include <device/pic.h>
#include <intr.h>

#include <lib/list.h>

#include <log.h>

PUBLIC xhci_t xhci;

extern volatile uint64_t global_ticks;

PRIVATE void switch_to_xhci(pci_device_t *xhci_dev)
{
    pci_device_t *ehci = pci_dev_match(0x0c,0x03,0x20);
    if (ehci == NULL || (pci_dev_config_read(ehci,0) & 0xffff) != 0x8086)
    {
        // ehci not exist
        return;
    }

    uint32_t superspeed_ports = pci_dev_config_read(xhci_dev, 0xdc); // USB3PRM
    pci_dev_config_write(xhci_dev, 0xd8, superspeed_ports); // USB3_PSSEN
    uint32_t ehci2xhci_ports = pci_dev_config_read(xhci_dev, 0xd4); // XUSB2PRM
    pci_dev_config_write(xhci_dev, 0xd0, ehci2xhci_ports); // XUSB2PR
}

PRIVATE void pr_usbsts()
{
    pr_log("\2 USBSTS: %08x\n",xhci_read_opt_reg32(XHCI_OPT_REG_USBSTS));
}

PRIVATE void xhci_halt()
{
    pr_log("\1 xHCI halt.\n");
    uint32_t usbcmd = xhci_read_opt_reg32(XHCI_OPT_REG_USBCMD);
    usbcmd &= ~USBCMD_RUN;
    xhci_write_opt_reg32(XHCI_OPT_REG_USBCMD,usbcmd);
    while((xhci_read_opt_reg32(XHCI_OPT_REG_USBSTS) & USBSTS_HCH) == 0);
    pr_log("\1 xHCI shutdown.\n");
}

PRIVATE void xhci_reset()
{
    pr_log("\1 xHCI reset.\n");
    uint32_t usbcmd = xhci_read_opt_reg32(XHCI_OPT_REG_USBCMD);
    usbcmd |= USBCMD_HCRST;
    xhci_write_opt_reg32(XHCI_OPT_REG_USBCMD,usbcmd);
    // Wait 1ms
    intr_status_t intr_status = intr_enable();
    pr_log("\1 xHCI reseting,wait 1ms.\n");
    uint64_t old_ticks = global_ticks;
    while(global_ticks < old_ticks + 10);
    intr_set_status(intr_status);

    while((xhci_read_opt_reg32(XHCI_OPT_REG_USBCMD) & USBCMD_HCRST));

    while((xhci_read_opt_reg32(XHCI_OPT_REG_USBSTS) & USBSTS_CNR));

    pr_log("\1 xHCI reset successfully.\n");
}

PRIVATE void xhci_start()
{
    if (xhci.max_slots > XHCI_MAX_SLOTS)
    {
        xhci.max_slots = XHCI_MAX_SLOTS;
    }
    xhci_write_opt_reg32(XHCI_OPT_REG_CONFIG,xhci.max_slots);
    uint32_t hcsparam2 = xhci_read_cap_reg32(XHCI_CAP_REG_HCSPARAM2);
    xhci.scrath_chapad_count = HCSP2_MAX_SC_BUF(hcsparam2);
    if (xhci.scrath_chapad_count > XHCI_MAX_SCRATCHPADS)
    {
        pr_log("\3 invalid number of scrath chapad: %d.\n",xhci.scrath_chapad_count);
        return;
    }
    xhci_write_opt_reg32(XHCI_OPT_REG_USBSTS,xhci_read_opt_reg32(XHCI_OPT_REG_USBSTS));
    xhci_write_opt_reg32(XHCI_OPT_REG_DNCTRL,0);

    uint8_t* buf;
    buf = pmalloc(sizeof(*xhci.dev_cxt_arr));
    if (buf == NULL)
    {
        pr_log("\3 Can not alloc addr for DCBA.\n");
        return;
    }
    xhci.dev_cxt_arr = (xhci_device_cxt_arr_t*)buf;

    xhci.dev_cxt_arr->base_addr[0] = (addr_t)buf
                                    + OFFSET(xhci_device_cxt_arr_t,scratchpad);
    uint32_t i;
    for (i = 0;i < xhci.scrath_chapad_count;i++)
    {
        void *buf_arr = pmalloc(4096);
        if (buf_arr == NULL)
        {
            pr_log("\3 Can not alloc memory for scratchpad_buf_arr[%d].\n",i);
            return;
        }
        xhci.dev_cxt_arr->scratchpad[i] = (phy_addr_t)buf_arr;
        memset(KADDR_P2V(xhci.dev_cxt_arr->scratchpad[i]),0,4096);
        pr_log("\2 scratchpad buffer array %d = %p\n",i,xhci.dev_cxt_arr->scratchpad[i]);
    }
    pr_log("\2 Set DCBAPP: %p.\n",buf);
    xhci_write_opt_reg32(XHCI_OPT_REG_DCBAPP_LO,(phy_addr_t)buf & 0xffffffff);
    xhci_write_opt_reg32(XHCI_OPT_REG_DCBAPP_HI,(phy_addr_t)buf >> 32);

    // Event Ring
    buf = pmalloc(sizeof(*xhci.erst)
                  + (XHCI_MAX_COMMANDS + XHCI_MAX_EVENTS)
                  * sizeof(xhci_trb_t));

    if (buf == NULL)
    {
        pr_log("\3 Can not alloc addr for DCBA.\n");
        return;
    }
    addr_t addr = (addr_t)KADDR_P2V(buf);
    xhci.erst = (xhci_erst_t*)addr;
    memset(xhci.erst,0,   sizeof(*xhci.erst)
                       + (XHCI_MAX_COMMANDS + XHCI_MAX_EVENTS)
                       * sizeof(xhci_trb_t));

    xhci.erst->rs_addr = (addr_t)buf + sizeof(xhci_erst_t);
    xhci.erst->rs_size = XHCI_MAX_EVENTS;
    xhci.erst->rsvdz = 0;

    addr += sizeof(xhci_erst_t);
    xhci.event_ring = (xhci_trb_t*)addr;
    addr += XHCI_MAX_EVENTS * sizeof(xhci_trb_t);
    xhci.command_ring = (xhci_trb_t*)addr;

    // Set ERST size
    pr_log("\1 Setting ERST size.\n");
    xhci_write_run_reg32(XHCI_IRS_ERSTSZ(0),ERSTSZ_SET(1));

    // Set ERDP
    pr_log("\1 Setting ERDP: %p.\n",xhci.erst->rs_addr);
    xhci_write_run_reg32(XHCI_IRS_ERDP_LO(0),xhci.erst->rs_addr & 0xffffffff);
    xhci_write_run_reg32(XHCI_IRS_ERDP_HI(0),xhci.erst->rs_addr >> 32);

    // Set ERSTBA
    pr_log("\1 Setting ERST base address: %p\n",buf);
    xhci_write_run_reg32(XHCI_IRS_ERSTBA_LO(0),(phy_addr_t)buf & 0xffffff);
    xhci_write_run_reg32(XHCI_IRS_ERSTBA_HI(0),(phy_addr_t)buf >> 32);

    buf += sizeof(xhci_erst_t) + XHCI_MAX_EVENTS * sizeof(xhci_trb_t);

    if ((xhci_read_opt_reg32(XHCI_OPT_REG_CRCR_LO) & CRCR_CRR) != 0)
    {
        pr_log("\1 Command Ring is Running. stop it.\n");
        xhci_write_opt_reg32(XHCI_OPT_REG_CRCR_LO,CRCR_CS);
        xhci_write_opt_reg32(XHCI_OPT_REG_CRCR_HI,      0);
        xhci_write_opt_reg32(XHCI_OPT_REG_CRCR_LO,CRCR_CA);
        xhci_write_opt_reg32(XHCI_OPT_REG_CRCR_HI,      0);
        while(xhci_read_opt_reg32(XHCI_OPT_REG_CRCR_LO) & CRCR_CRR);
    }
    pr_log("\1 Setting CRCR = %p.\n",buf);
    xhci_write_opt_reg32(XHCI_OPT_REG_CRCR_LO,((addr_t)buf | CRCR_RCS) & 0xffffffff);
    xhci_write_opt_reg32(XHCI_OPT_REG_CRCR_HI,(addr_t)buf >> 32);

    xhci.command_ring[XHCI_MAX_COMMANDS - 1].addr = (phy_addr_t)buf;

    // Set interrupt rate.
    pr_log("\1 Setting interrupt rate.\n");
    // interrupts/sec = 1/(250 × (10^-9 sec) × Interval)
    xhci_write_run_reg32(XHCI_IRS_IMOD(0),0x000003f8);

    // enable intr
    uint32_t iman = xhci_read_run_reg32(XHCI_IRS_IMAN(0));
    iman |= IMAN_INTR_EN;
    xhci_write_run_reg32(XHCI_IRS_IMAN(0),iman);

    // run
    xhci_write_opt_reg32(XHCI_OPT_REG_USBCMD,USBCMD_RUN | USBCMD_INTE | USBCMD_HSEE);

    // wait for start up.
    pr_log("\1 wait for xHCI start up.\n");
    while(xhci_read_opt_reg32(XHCI_OPT_REG_USBSTS) & USBSTS_HCH);
    pr_log("\1 xHCI start up done.\n");
    pr_log("\1 xHCI is %s now.\n",xhci_read_opt_reg32(XHCI_OPT_REG_USBCMD) & USBCMD_RUN ? "Running" : "Stop");

    pr_usbsts();
    return;
}

PRIVATE void intr_xHCI_handler(intr_stack_t *stack)
{
    eoi(IRQ_XHCI);
    (void)stack;
    pr_log("\2 xHCI interrupt.\n");
    return;
}

PUBLIC void xhci_init()
{
    register_handle(IRQ_XHCI,intr_xHCI_handler);

    pr_log("\1 xHCI init.\n");

    memset(&xhci,0,sizeof(xhci));

    pci_device_t *device = pci_dev_match(0x0c,0x03,0x30);
    if (device == NULL)
    {
        pr_log("\3xhci_dev not found.\n");
        return;
    }
    pr_log("\1 xhci_dev vendor ID: %04x.\n",pci_dev_config_read(device,0) & 0xffff);

    addr_t xhci_mmio_base = pci_dev_read_bar(device,0);
    pr_log("\1 xhci mmio base: %p\n",xhci_mmio_base);

    page_map((uint64_t*)KERNEL_PAGE_DIR_TABLE_POS,(void*)xhci_mmio_base,(void*)KADDR_P2V(xhci_mmio_base));
    xhci_mmio_base      = (addr_t)KADDR_P2V(xhci_mmio_base);
    uint8_t cap_length  = *(uint8_t*)xhci_mmio_base & 0xff;

    xhci.mmio_base = (void*)xhci_mmio_base;
    xhci.cap_regs  = (uint8_t*)xhci_mmio_base;
    xhci.opt_regs  = xhci.cap_regs + cap_length;
    xhci.run_regs  = xhci.cap_regs + xhci_read_cap_reg32(XHCI_CAP_REG_RSTOFF);
    xhci.max_ports = HCSP1_MAX_PORTS(xhci_read_cap_reg32(XHCI_CAP_REG_HCSPARAM1));
    xhci.max_slots = HCSP1_MAX_SLOTS(xhci_read_cap_reg32(XHCI_CAP_REG_HCSPARAM1));

    xhci.msi_vector = IRQ_XHCI;

    uint16_t hci_ver = HCIVERSION(xhci_read_cap_reg32(XHCI_CAP_REG_HCIVERSION));
    pr_log("\1 interface version: %04x\n",hci_ver);
    if (hci_ver < 0x0090 || hci_ver > 0x0120)
    {
        pr_log("\3 Unsupported HCI version.\n");
        return;
    }
    uint32_t hccparams = xhci_read_cap_reg32(XHCI_CAP_REG_HCCPARAMS);
    pr_log("\1 HCCPARAMS: %08x\n",hccparams);
    if (hccparams == 0xffffffff)
    {
        return;
    }
    xhci.cxt_size = HCCP_CSZ(hccparams) == 1 ? 64 : 32;

    if ((pci_dev_config_read(device,0) & 0xffff) == 0x8086)
    {
        switch_to_xhci(device);
    }
    // halt xHCI
    xhci_halt();

    // reset xHCI
    xhci_reset();
    pr_usbsts();

    // pci interrupt_line
    // this register corresponds to
    // the PIC IRQ numbers 0-15 (and not I/O APIC IRQ numbers)
    // and a value of 0xFF defines no connection
    uint32_t interrupt_line = pci_dev_config_read(device,0x3c) & 0xff;

    // pci interrupt_pin
    // Specifies which interrupt pin the device uses.
    // Where a value of 0x1 is INTA#, 0x2 is INTB#, 0x3 is INTC#, 0x4 is INTD#,
    // and 0x0 means the device does not use an interrupt pin.
    uint32_t interrupt_pin  = (pci_dev_config_read(device,0x3c) >> 8) & 0xff;
    pr_log("\2 intr line: %02x\n",interrupt_line);
    pr_log("\2 intr pin:  %02x\n",interrupt_pin);
    if (interrupt_line == 0xff)
    {
        pr_log("\3 xHCI was assigned an invalid IRQ.\n");
        // return;
    }
    // configure and enable MSI
    // configure_msi(device,1,0,interrupt_line,0);
    uint32_t status = pci_dev_configure_msi(device,IRQ_XHCI,1);
    if (ERROR(status))
    {
        pr_log("\3 xHCI: can not configure MSI.\n");
        return;
    }
    pci_dev_enable_msi(device);
    pr_log("\1 xHCI init done.\n");
    pr_usbsts();
    xhci_start();
}

PUBLIC uint32_t xhci_read_cap_reg32(uint32_t reg)
{
    return *(volatile uint32_t*)(xhci.mmio_base + reg);
}

PUBLIC uint32_t xhci_read_opt_reg32(uint32_t reg)
{
    return *(volatile uint32_t*)(xhci.opt_regs + reg);
}

PUBLIC void xhci_write_opt_reg32(uint32_t reg,uint32_t value)
{
    *(volatile uint32_t*)(xhci.opt_regs + reg) = value;
    return;
}

PUBLIC uint32_t xhci_read_run_reg32(uint32_t reg)
{
    return *(volatile uint32_t*)(xhci.run_regs + reg);
}

PUBLIC void xhci_write_run_reg32(uint32_t reg,uint32_t val)
{
    *(volatile uint32_t*)(xhci.run_regs + reg) = val;

    return;
}