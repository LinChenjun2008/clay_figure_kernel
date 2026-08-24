// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/drivers/apic/ioapic.h>
#include <asm/drivers/apic/lapic.h>
#include <asm/drivers/apic/struct.h>
#include <asm/drivers/pic.h>
#include <asm/utils/io.h>

#include <print.h>
#include <std/string.h> // memset

struct apic apic;

static uint8_t set_lapic(struct madt_lapic *apic_lapic)
{
    struct lapic *lapic = &apic.lapic[apic_lapic->apic_id];
    lapic->processor_id = apic_lapic->processor_id;
    printk(
        "lapic[%d]: processor_id=%d, flags=%08x.\n",
        apic_lapic->apic_id,
        apic_lapic->processor_id,
        apic_lapic->flags
    );
    return apic_lapic->apic_id;
}

static void set_ioapic(struct madt_ioapic *apic_ioapic)
{
    struct ioapic *ioapic = &apic.ioapic[apic_ioapic->ioapic_id];

    uintptr_t ioapic_addr = apic_ioapic->address;

    ioapic->id         = apic_ioapic->ioapic_id;
    ioapic->index_addr = (uint8_t *)ioapic_addr;
    ioapic->data_addr  = (uint32_t *)(ioapic_addr + 0x10UL);
    ioapic->eoi_addr   = (uint32_t *)(ioapic_addr + 0x40UL);

    uint64_t ioapic_version = ioapic_rte_read(ioapic->id, 1);
    uint64_t max_pins       = ((ioapic_version >> 16) & 0xff);

    ioapic->gsi_start = apic_ioapic->gsi_base;
    ioapic->gsi_end   = ioapic->gsi_start + max_pins;

    printk(
        "IOAPIC[%d]: address=%p, version: %#x,gsi: %d ~ %d.\n",
        ioapic->id,
        ioapic->index_addr,
        ioapic_version,
        ioapic->gsi_start,
        ioapic->gsi_end
    );
    return;
}

static void
set_ioapic_irq_override(struct madt_ioapic_irq_override *irq_override)
{
    struct ioapic_irq_override *irq = &apic.irq[irq_override->irq];

    const char *polarity_mode_str[4] = { "dfl", "high", "Reserved", "low" };
    const char *trigger_mode_str[4]  = { "dfl", "edge", "Reserved", "level" };

    irq->bus   = irq_override->bus;
    irq->gsi   = irq_override->gsi;
    irq->flags = irq_override->flags;
    printk(
        "IRQ-%02d (bus %02d, gsi %02d, flags %04x, %s, %s).\n",
        irq_override->irq,
        irq_override->bus,
        irq_override->gsi,
        irq_override->flags,
        trigger_mode_str[(irq_override->flags >> 2) & 0x3],
        polarity_mode_str[(irq_override->flags >> 0) & 0x3]
    );
    return;
}

static void read_madt(struct boot_info *boot_info)
{
    apic.cores              = 0;
    apic.local_apic_address = 0;
    apic.max_lapic_id       = 0;
    memset(&apic.lapic, 0, sizeof(apic.lapic));
    memset(&apic.ioapic, 0, sizeof(apic.ioapic));
    memset(&apic.irq, 0, sizeof(apic.irq));

    // Assume GSI=IRQ before read I/O APIC interrupt source override table.
    int i;
    for (i = 0; i < IRQ_CNT; i++)
    {
        apic.irq[i].gsi = i;
    }
    apic.irq[0].gsi = 2;
    apic.irq[2].gsi = -1;

    struct madt *madt;
    madt = (struct madt *)acpi_find_table(boot_info, MADT_SIGNATURE);
    apic.local_apic_address = madt->local_apic_address;
    printk("local apic address: %p.\n", apic.local_apic_address);
    uint8_t max_lapic_id = 0;

    uint8_t          *p2 = (uint8_t *)madt + madt->header.length;
    uint8_t          *p;
    struct madt_head *madt_head = (struct madt_head *)(madt + 1);
    for (p = (uint8_t *)(madt + 1); p < p2; p += madt_head->record_length)
    {
        madt_head = (struct madt_head *)p;
        switch (madt_head->type)
        {
            case 0:
                apic.cores++;
                max_lapic_id      = set_lapic((struct madt_lapic *)madt_head);
                apic.max_lapic_id = MAX(apic.max_lapic_id, max_lapic_id);
                break;
            case 1:
                set_ioapic((struct madt_ioapic *)madt_head);
                break;
            case 2:
                set_ioapic_irq_override(
                    (struct madt_ioapic_irq_override *)madt_head
                );
                break;
        }
    }
    printk("Max local APIC ID: %d.\n", apic.max_lapic_id);
    return;
}

void apic_init(struct boot_info *boot_info)
{
    read_madt(boot_info);

    // 禁止8259A的所有中断
    io_out8(PIC_M_DATA, 0xff);
    io_out8(PIC_S_DATA, 0xff);

    // IMCR
    io_out8(0x22, 0x70);
    io_out8(0x23, 0x01);

    local_apic_init();
    int i;
    for (i = 0; i < MAX_IOAPIC; i++)
    {
        ioapic_init(i);
    }
    return;
}

uint8_t apic_cpu_count(void)
{
    return apic.cores;
}

uint8_t apic_max_lapic_id(void)
{
    return apic.max_lapic_id;
}


void apic_send_eoi(void)
{
    local_apic_write(APIC_REG_EOI, 0);
    return;
}

uint8_t apic_id(void)
{
    return (local_apic_read(APIC_REG_ID) >> 24) & 0xff;
}