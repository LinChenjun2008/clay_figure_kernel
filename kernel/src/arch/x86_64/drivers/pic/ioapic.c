// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic/ioapic.h>
#include <asm/drivers/apic/struct.h>
#include <asm/page.h> // PHYS_TO_VIRT
#include <asm/utils/barrier.h>

#include <errno.h>
#include <print.h>

extern struct apic apic;

uint64_t ioapic_rte_read(uint8_t ioapic_id, uint8_t index)
{
    struct ioapic *ioapic = &apic.ioapic[ioapic_id];

    uint64_t ret;
    io_mfence();
    *(volatile uint8_t *)PHYS_TO_VIRT(ioapic->index_addr) = index + 1;
    io_mfence();
    ret = *(volatile uint32_t *)PHYS_TO_VIRT(ioapic->data_addr);
    ret <<= 32;

    io_mfence();
    *(volatile uint8_t *)PHYS_TO_VIRT(ioapic->index_addr) = index;
    io_mfence();
    ret |= *(volatile uint32_t *)PHYS_TO_VIRT(ioapic->data_addr);

    io_mfence();
    BARRIER();
    return ret;
}

void ioapic_rte_write(uint8_t ioapic_id, uint8_t index, uint64_t value)
{
    struct ioapic *ioapic = &apic.ioapic[ioapic_id];

    io_mfence();
    *(volatile uint8_t *)PHYS_TO_VIRT(ioapic->index_addr) = index;
    io_mfence();
    *(volatile uint32_t *)PHYS_TO_VIRT(ioapic->data_addr) = value & 0xffffffff;
    value >>= 32;

    io_mfence();
    *(volatile uint8_t *)PHYS_TO_VIRT(ioapic->index_addr) = index + 1;
    io_mfence();
    *(volatile uint32_t *)PHYS_TO_VIRT(ioapic->data_addr) = value & 0xffffffff;
    io_mfence();
    return;
}

static void
ioapic_enable(uint8_t ioapic_id, uint8_t pin, uint8_t vector, uint16_t flags)
{
    uint64_t trigger_mode = (flags >> 2) & 0x03;
    uint64_t polarity     = (flags >> 0) & 0x03;
    uint64_t destination  = (flags >> 8) & 0xff;

    uint64_t rte_value;

    rte_value = ioapic_rte_read(ioapic_id, (pin << 1) + 0x10);
    rte_value &= 0xa800UL;

    // Trigger mode: 0: Edge 1: Level
    if (trigger_mode == 0b01) rte_value &= ~(1UL << 15);
    if (trigger_mode == 0b11) rte_value |= (1UL << 15);

    // Polarity: 0: High 1: Low
    if (polarity == 0b01) rte_value &= ~(1UL << 13);
    if (polarity == 0b11) rte_value |= (1UL << 13);

    rte_value |= vector;
    rte_value |= destination << 56;
    ioapic_rte_write(ioapic_id, (pin << 1) + 0x10, rte_value);
    return;
}

int ioapic_irq_enable(uint8_t irq, uint8_t vector, uint8_t destination)
{
    struct ioapic *ioapic;
    uint8_t        gsi = apic.irq[irq].gsi;
    int            i;
    for (i = 0; i < MAX_IOAPIC; i++)
    {
        ioapic = &apic.ioapic[i];
        if (ioapic->index_addr == NULL)
        {
            continue;
        }
        if (ioapic->gsi_start <= gsi && gsi < ioapic->gsi_end)
        {
            break;
        }
    }
    if (i == MAX_IOAPIC)
    {
        printk(MSG_ERR "ioapic_irq_enable: i == MAX_IOAPIC.\n");
        return -ENODEV;
    }
    uint8_t  pin   = gsi - ioapic->gsi_start;
    uint16_t flags = destination;
    flags <<= 8;
    flags |= apic.irq[irq].flags;

    ioapic_enable(i, pin, vector, flags);
    return 0;
}

void ioapic_init(uint8_t ioapic_id)
{
    struct ioapic *ioapic = &apic.ioapic[ioapic_id];
    if (ioapic->index_addr == NULL)
    {
        return;
    }

    printk(MSG_HIGHLIGHT("I/O APIC[%d]") " initializing...\n", ioapic_id);

    io_mfence();
    *(uint8_t *)PHYS_TO_VIRT(ioapic->index_addr) = 0;
    io_mfence();
    *(uint32_t *)PHYS_TO_VIRT(ioapic->data_addr) = 0x0f000000;

    /* 屏蔽所有中断 */
    uint64_t rte_value;

    uint64_t ioapic_version = ioapic_rte_read(ioapic->id, 1);
    uint8_t  max_pins       = ((ioapic_version >> 16) & 0xff);
    uint8_t  index;
    uint8_t  pin;
    for (pin = 0; pin < max_pins; pin++)
    {
        index     = (pin << 1) + 0x10;
        rte_value = ioapic_rte_read(ioapic_id, index) | 0x10000;
        ioapic_rte_write(ioapic_id, index, rte_value);
    }
    return;
}
