/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/usb/xhci.h>

PUBLIC uint32_t xhci_read_cap(xhci_t *xhci, uint32_t reg)
{
    return *(volatile uint32_t *)(xhci->cap_regs + reg);
}

PUBLIC uint32_t xhci_read_opt(xhci_t *xhci, uint32_t reg)
{
    return *(volatile uint32_t *)(xhci->opt_regs + reg);
}

PUBLIC void xhci_write_opt(xhci_t *xhci, uint32_t reg, uint32_t val)
{
    *(volatile uint32_t *)(xhci->opt_regs + reg) = val;
    return;
}

PUBLIC uint32_t xhci_read_run(xhci_t *xhci, uint32_t reg)
{
    return *(volatile uint32_t *)(xhci->run_regs + reg);
}

PUBLIC void xhci_write_run(xhci_t *xhci, uint32_t reg, uint32_t val)
{
    *(volatile uint32_t *)(xhci->run_regs + reg) = val;
    return;
}

PUBLIC uint32_t xhci_read_doorbell(xhci_t *xhci, uint32_t reg)
{
    return *(volatile uint32_t *)(xhci->doorbell_regs + reg);
}

PUBLIC void xhci_write_doorbell(xhci_t *xhci, uint32_t reg, uint32_t val)
{
    *(volatile uint32_t *)(xhci->doorbell_regs + reg) = val;
    return;
}