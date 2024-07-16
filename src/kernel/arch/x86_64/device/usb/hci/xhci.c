#include <kernel/global.h>
#include <device/pci.h>
#include <device/usb/hci/xhci.h>
#include <mem/mem.h>
#include <io.h>
#include <device/pic.h>
#include <intr.h>

#include <log.h>

extern apic_t apic_struct;

extern uint64_t volatile global_ticks;

PUBLIC xhci_t xhci;

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
    pr_log("\1switch ehci to xhci: SS = %02x, xHCI = %02x\n",
            superspeed_ports, ehci2xhci_ports);
}

PRIVATE void xhci_reset()
{

    // reset controller
    uint32_t usbcmd;
    usbcmd = xhci.opt_regs->USBCMD;
    usbcmd |= HCRST;
    xhci.opt_regs->USBCMD = usbcmd;
    // Wait 1ms
    intr_status_t intr_status = intr_enable();
    pr_log("\1xHCI reseting,wait 1ms.\n");
    uint64_t old_ticks = global_ticks;
    while(global_ticks < old_ticks + 10);
    intr_set_status(intr_status);

    while (xhci.opt_regs->USBCMD & HCRST);

    // After Chip Hardware Reset5 wait until the Controller Not Ready
    // (CNR) flag in the USBSTS is ‘0’ before writing any xHC Operational
    // or Runtime registers.

    while (xhci.opt_regs->USBSTS & CNR);

    return;
}

PRIVATE void xhci_run()
{
    xhci.opt_regs->USBCMD |= RUN_STOP;
    io_mfence();
    while (xhci.opt_regs->USBSTS & HCH);

    pr_log("\1xHCI %s\n",xhci.opt_regs->USBCMD & RUN_STOP ? "Running" : "Stop");
    return;
}

PUBLIC void xhci_init()
{
    memset(&xhci,0,sizeof(xhci));

    pci_device_t *device = pci_dev_match(0x0c,0x03,0x30);
    if (device == NULL)
    {
        pr_log("\3xhci_dev not found.\n");
        return;
    }

    // Config MSI
    configure_msi(device,1 /* Level*/,0 /* Fixed*/,IRQ_XHCI,0);

    // eXtensible Host Controller Interface for Universal Serial Bus
    // 4.2 :
    // Initialize the system I/O memory maps, if supported
    uintptr_t xhci_mmio_base = pci_dev_read_bar(device,0);
    pr_log("\1xhci mmio base: %p\n",xhci_mmio_base);
    page_map((uint64_t*)KERNEL_PAGE_DIR_TABLE_POS,(void*)xhci_mmio_base,(void*)KADDR_P2V(xhci_mmio_base));
    xhci_mmio_base = (uintptr_t)KADDR_P2V(xhci_mmio_base);

    if ((pci_dev_config_read(device,0) & 0xffff) == 0x8086)
    {
        switch_to_xhci(device);
    }

    uint8_t cap_length  = *(uint8_t*)xhci_mmio_base & 0xff;
    uintptr_t opt_point = xhci_mmio_base + cap_length;

    xhci.mmio_base      = (void*)xhci_mmio_base;
    xhci.cap_regs       = (xhci_cap_regs_t*)xhci_mmio_base;
    xhci.opt_regs       = (xhci_opt_regs_t*)opt_point;
    xhci.max_ports      = (xhci.cap_regs->HCSPARAMS1 >> 24) & 0xff;
    xhci.max_slots      = xhci.cap_regs->HCSPARAMS1 & MAX_SLOTS;

    pr_log("\1USBCMD: 0x%x\n",xhci.opt_regs->USBCMD);
    pr_log("\1USBSTS: 0x%x\n",xhci.opt_regs->USBSTS);
    pr_log("\1Max Slots: %d\n",xhci.max_slots);

    void *p_dev_cxt = pmalloc(sizeof(xhci_dev_context_t*) * (xhci.max_slots + 1));
    if (p_dev_cxt == NULL)
    {
        pr_log("\3Can't Allocate Memory for xhci device context!\n");
        return;
    }
    xhci.dev_cxt = KADDR_P2V(p_dev_cxt);
    int i;
    for (i = 0;i < xhci.max_slots + 1;i++)
    {
        xhci.dev_cxt[i] = NULL;
    }

    // init xhci
    uint32_t usbcmd = xhci.opt_regs->USBCMD;
    usbcmd &= ~(INTE | HSEE | EWE);
    // Halte the host controller to reset it.
    if (!(xhci.opt_regs->USBSTS & HCH))
    {
        usbcmd &= ~RUN_STOP;// run/stop = 0
    }
    xhci.opt_regs->USBCMD = usbcmd;
    while (!(xhci.opt_regs->USBSTS & HCH));

    pr_log("\1 reset xHCI.\n");
    xhci_reset();
    pr_log("\1 reset xHCI done.\n");

    pr_log("\1USBCMD: 0x%x\n",xhci.opt_regs->USBCMD);
    pr_log("\1USBSTS: 0x%x\n",xhci.opt_regs->USBSTS);
    pr_log("\1Max Slots: %d\n",xhci.max_slots);


    // eXtensible Host Controller Interface for Universal Serial Bus
    // 4.2 :
    // Program the Max Device Slots Enabled (MaxSlotsEn) field in
    // the CONFIG register to enable the device slots
    // that system software is going to use.
    uint32_t config = xhci.opt_regs->CONFIG;
    config &= ~0xff;
    config |= xhci.max_slots;
    xhci.opt_regs->CONFIG = config;

    uint32_t pagesize = xhci.opt_regs->PAGESIZE;
    pr_log("\1xHCI PAGESIZE: %d\n",pagesize);
    uint8_t shift;

    // The maximum possible page size is 128M.
    for (shift = 0;shift <= 15;shift++)
    {
        if (pagesize & (1 << shift))
        {
            break;
        }
    }
    pr_log("\1bit %d is set. pg_size = %d\n",shift,4096 << shift);

    uint32_t hcsparams2 = xhci.cap_regs->HCSPARAMS2;
    uint16_t max_scratchpad_buffers = ((hcsparams2 >> 27) & 0x1f) | (((hcsparams2 >> 21) & 0x1f) << 5);
    pr_log("\2max_scratchpad_buffers: %d\n",max_scratchpad_buffers);
    if (max_scratchpad_buffers > 0)
    {
        void *buf = pmalloc(max_scratchpad_buffers * sizeof(void*));
        if (buf == NULL)
        {
            pr_log("\3 Can not alloc memory for scratchpad_buf_arr.\n");
            return;
        }
        void **scratchpad_buf_arr = KADDR_P2V(buf);
        int i;
        for (i = 0;i < max_scratchpad_buffers;i++)
        {
            void *buf_arr = pmalloc(4096);
            if (buf_arr == NULL)
            {
                pr_log("\3 Can not alloc memory for scratchpad_buf_arr[%d].\n",i);
                return;
            }
            scratchpad_buf_arr[i] = KADDR_P2V(buf_arr);
            pr_log("\2scratchpad buffer array %d = %p\n",i, scratchpad_buf_arr[i]);
        }
        xhci.dev_cxt[0] = (xhci_dev_context_t*)scratchpad_buf_arr;
        pr_log("\2xhci.dev_cxt[0] = %p\n",xhci.dev_cxt[0]);
    }

    // eXtensible Host Controller Interface for Universal Serial Bus
    // 4.2 :
    // Program the Device Context Base Address Array Pointer (DCBAAP) register
    // with a 64-bit address pointing to where the Device Context Base Address
    // Array is located.
    xhci.opt_regs->DCBAAP = (uint64_t)xhci.dev_cxt & 0x3f;

    xhci.intr_reg_sets = (intr_reg_set_t*)((uintptr_t)xhci.mmio_base + (xhci.cap_regs->RTSOFF & OFFSET) + 0x20);
    xhci.intr_reg_sets_size = 1024;
    intr_reg_set_t *primary_intr = &xhci.intr_reg_sets[0];

    // init command ring
    xhci.cr.cycle_bit   = 1;
    xhci.cr.write_index = 0;
    xhci.cr.trb_size    = 32 * sizeof(*xhci.cr.trb);

    void *buffer = pmalloc(xhci.cr.trb_size);
    if (buffer == NULL)
    {
        pr_log("\3 Can not alloc memory for xhci.cr.trb.\n");
        return;
    }
    xhci.cr.trb = (trb_t*)KADDR_P2V(buffer);
    memset(xhci.cr.trb,0,xhci.cr.trb_size);

    // eXtensible Host Controller Interface for Universal Serial Bus
    // 4.2 :
    // Define the Command Ring Dequeue Pointer by programming the Command Ring Control Register
    // with a 64-bit address pointing to the starting address of the first TRB of the Command Ring.
    uint64_t crcr = xhci.opt_regs->CRCR;
    crcr |= RCS;
    crcr &= ~CS;
    crcr &= ~CA;
    crcr |= (uint64_t)xhci.cr.trb | CRP;
    xhci.opt_regs->CRCR = crcr;

    // Allocate and initialize the Event Ring Segment(s).
    xhci.er.cycle_bit   = 1;
    xhci.er.trb_size    = 32 * sizeof(*xhci.cr.trb);
    xhci.er.intr_reg_sets = primary_intr;
    buffer = pmalloc(xhci.er.trb_size);
    if (buffer == NULL)
    {
        pr_log("\3 Can not alloc memory for xhci.er.trb.\n");
        return;
    }
    xhci.er.trb = (trb_t*)KADDR_P2V(buffer);
    memset(xhci.er.trb,0,xhci.er.trb_size);

    buffer = pmalloc(64 /* sizeof(*xhci.er.erst) */); // 64-bit align
    if (buffer == NULL)
    {
        pr_log("\3 Can not alloc memory for xhci.er.erst.\n");
        return;
    }
    xhci.er.erst = (erst_t*)KADDR_P2V(buffer);
    memset(xhci.er.erst,0,sizeof(*xhci.er.erst));

    xhci.er.erst->rsba = (uint64_t)xhci.er.trb & 0x3f;
    xhci.er.erst->data[0] = xhci.er.trb_size & 0xffff;

    uint32_t erstsz = primary_intr->ERSTSZ;
    erstsz &= 0xffff;
    erstsz |= 1;
    primary_intr->ERSTSZ = erstsz;

    // Write dequeue pointer
    uint64_t erdp = primary_intr->ERDP;
    erdp &= ~0xfULL;
    erdp |= (uintptr_t)xhci.er.trb;
    primary_intr->ERDP = erdp;

    uint64_t erstba = primary_intr->ERSTBA;
    erstba &= ~0x3fULL;
    erstba |= (uint64_t)xhci.er.erst & 0x3fULL;
    primary_intr->ERSTBA = erstba;

    // Enable interrupt for the primary intr
    uint32_t iman = primary_intr->IMAN;
    iman |= 3; // pending,enable
    primary_intr->IMAN = iman;

    // Enable interrupt for the controller
    usbcmd = xhci.opt_regs->USBCMD;
    usbcmd |= INTE;
    xhci.opt_regs->USBCMD = usbcmd;

    xhci_run();

    pr_log("\1xHCI init done.\n");
    pr_log("\1USBCMD: 0x%x\n",xhci.opt_regs->USBCMD);
    pr_log("\1USBSTS: 0x%x\n",xhci.opt_regs->USBSTS);
    return;
}