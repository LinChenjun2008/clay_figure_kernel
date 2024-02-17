#include <kernel/global.h>
#include <device/pci.h>
#include <mem/mem.h>
#include <io.h>

#include <log.h>

typedef struct
{
    void     *xhci_mmio_base;
    uint32_t *opt_regs;
} xhci_t;

PRIVATE xhci_t xhci;
// divided by 4 for use as uint[] indices.
#define USBCMD                                 0
#define USBSTS   (0x04 / 4)
#define PAGESIZE (0x08 / 4)
#define DNCTRL   (0x14 / 4)
#define CRCR     (0x18 / 4)
#define DCBAPP   (0x30 / 4)
#define CONFIG   (0x38 / 4)

PRIVATE void switch_to_xhci(pci_device_t *dev)
{
    if (pci_dev_match(0x0c,0x03,0x20) == NULL || (pci_dev_config_read(dev,0) & 0xffff) != 0x8086)
    {
        // ehci not exist
        return;
    }

    uint32_t superspeed_ports = pci_dev_config_read(dev, 0xdc); // USB3PRM
    pci_dev_config_write(dev, 0xd8, superspeed_ports); // USB3_PSSEN
    uint32_t ehci2xhci_ports = pci_dev_config_read(dev, 0xd4); // XUSB2PRM
    pci_dev_config_write(dev, 0xd0, ehci2xhci_ports); // XUSB2PR
    pr_log("\1switch ehci to xhci: SS = %02x, xHCI = %02x\n",
            superspeed_ports, ehci2xhci_ports);
}

PUBLIC void xhci_init()
{
    pci_device_t *device = pci_dev_match(0x0c,0x03,0x30);
    if (device == NULL)
    {
        pr_log("\3xhci_dev not found.\n");
        return;
    }
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

    xhci.xhci_mmio_base = (void*)xhci_mmio_base;
    xhci.opt_regs       = (uint32_t*)opt_point;

    pr_log("\1USBCMD: 0x%x\n",xhci.opt_regs[USBCMD]);
    pr_log("\1USBSTS: 0x%x\n",xhci.opt_regs[USBSTS]);

    uint32_t usbcmd = xhci.opt_regs[USBCMD];
    usbcmd |= 1; // run/stop = 1

    while (xhci.opt_regs[USBSTS] & 1);
    xhci.opt_regs[USBCMD] = usbcmd;
    usbcmd = xhci.opt_regs[USBCMD];
    pr_log("\1xHCI %s\n",xhci.opt_regs[USBCMD] & 1 ? "Running" : "Stop");
    return;
}