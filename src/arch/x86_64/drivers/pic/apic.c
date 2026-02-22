// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/drivers/pic.h>
#include <asm/mem/page.h> // PHYS_TO_VIRT
#include <asm/utils.h>
#include <asm/x86.h> // IA32_APIC_BASE

#include <drivers/acpi.h>
#include <print.h>
#include <std/string.h> // memset

#define MAX_LAPIC  256
#define MAX_IOAPIC 256

#pragma pack(1)
struct madt
{
    struct acpi_description_header header;
    uint32_t                       local_apic_address;
    uint32_t                       flags;
};

struct madt_head
{
    uint8_t type;
    uint8_t record_length;
};

// Entry type 0: Processor local APIC
struct madt_lapic
{
    struct madt_head head;
    uint8_t          processor_id;
    uint8_t          apic_id;
    uint32_t         flags;
};

// Entry type 1: I/O APIC
struct madt_ioapic
{
    struct madt_head head;
    uint8_t          ioapic_id;
    uint8_t          reserved;
    uint32_t         address;
    uint32_t         gsi_base;
};

// Entry type 2: I/O APIC interrupt source override
struct madt_ioapic_irq_override
{
    struct madt_head head;
    uint8_t          bus;
    uint8_t          irq;
    uint32_t         gsi;
    uint16_t         flags;
};

// Entry type 3: I/O APIC Non-maskable interrupt source
struct madt_ioapic_non_maskable_irq_src
{
    struct madt_head head;
    uint8_t          nmi_source;
    uint8_t          reserved;
    uint16_t         flags;
    uint32_t         gsi;
};

// Entry type 4: Local APIC Non-maskable interrupts
struct madt_lapic_non_maskable_irq_src
{
    struct madt_head head;
    uint8_t          processor_id;
    uint16_t         flags;
    uint8_t          lint;
};

// Entry type 5: Local APIC address override
struct madt_lapic_irq_override
{
    struct madt_head head;
    uint16_t         reserved;
    uint64_t         address;
};

// Entry type 9: Processor local x2APIC
struct madt_x2apic
{
    struct madt_head head;
    uint16_t         reserved;
    uint32_t         x2apic_id;
    uint32_t         flags;
    uint32_t         acpi_id;
};
#pragma pack()

// Flags fields for type 2,3 and 4:
// | Offset | Length | Description                           |
// |--------|--------|---------------------------------------|
//          |        | Determines the polarity:              |
// |      0 |      2 | 00: No override,use default settings. |
// |        |        | 01: Active high override.             |
// |        |        | 10: Reserved.                         |
// |        |        | 11: Active low override.              |
// |--------|--------|---------------------------------------|
// |        |        | Determines the trigger mode:          |
// |      2 |      2 | 00: No override,use default settings. |
// |        |        | 01: Edge triggered.                   |
// |        |        | 10: Reserved.                         |
// |        |        | 11: Level triggered.                  |
// |--------|--------|---------------------------------------|
// |      4 |     12 | Reserved.                             |
// |--------|--------|---------------------------------------|

struct lapic
{
    uint8_t processor_id;
};

struct ioapic
{
    uint8_t   id;
    uint8_t  *index_addr;
    uint32_t *data_addr;
    uint32_t *eoi_addr;
    uint32_t  gsi_start;
    uint32_t  gsi_end;
};

struct ioapic_irq_override
{
    uint8_t bus;
    uint8_t gsi;
    uint8_t flags;
};

struct apic
{
    uint8_t                    cores;
    uint64_t                   local_apic_address;
    uint8_t                    max_lapic_id;
    struct lapic               lapic[MAX_LAPIC];
    struct ioapic              ioapic[MAX_IOAPIC];
    struct ioapic_irq_override irq[IRQ_CNT];
};

static struct apic apic;

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
    madt = (struct madt *)xsdt_find_table(boot_info, MADT_SIGNATURE);
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

uint32_t local_apic_read(uint16_t index)
{
    uint32_t ret;
    io_mfence();
    ret = *(volatile uint32_t *)PHYS_TO_VIRT(apic.local_apic_address + index);
    BARRIER();
    return ret;
}

void local_apic_write(uint16_t index, uint32_t value)
{
    io_mfence();
    *(volatile uint32_t *)PHYS_TO_VIRT(apic.local_apic_address + index) = value;
    io_mfence();
    BARRIER();
    return;
}

static void xapic_init()
{
    // enable SVR[8]
    uint32_t svr = local_apic_read(APIC_REG_SVR);
    svr |= 1 << 8;
    if (local_apic_read(APIC_REG_VERSION) >> 24 & 1)
    {
        svr |= 1 << 12;
    }
    local_apic_write(APIC_REG_SVR, svr);

    // Mask all LVT
    local_apic_write(0x2f0, 0x10000);
    local_apic_write(0x320, 0x10000);
    local_apic_write(0x330, 0x10000);
    local_apic_write(0x340, 0x10000);
    local_apic_write(0x350, 0x10000);
    local_apic_write(0x360, 0x10000);
    local_apic_write(0x370, 0x10000);
    return;
}

static void x2apic_init()
{
    // enable SVR[8]
    uint32_t svr = rdmsr(0x80f);
    svr |= 1 << 8;
    if (rdmsr(0x803) >> 24 & 1)
    {
        svr |= 1 << 12;
    }
    wrmsr(0x80f, svr);

    // Mask all LVT
    wrmsr(0x82f, 0x10000);
    wrmsr(0x832, 0x10000);
    wrmsr(0x833, 0x10000);
    wrmsr(0x834, 0x10000);
    wrmsr(0x835, 0x10000);
    wrmsr(0x836, 0x10000);
    wrmsr(0x837, 0x10000);
    return;
}

void local_apic_init()
{
    uint32_t a, b, c, d;
    asm_cpuid(1, 0, &a, &b, &c, &d);
    uint64_t ia32_apic_base = rdmsr(IA32_APIC_BASE);
    if ((c & (1 << 21)) && (ia32_apic_base & 0x400))
    {
        x2apic_init();
    }
    else
    {
        if ((d & (1 << 9)) && (ia32_apic_base & 0x800))
        {
            xapic_init();
        }
    }
    return;
}

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
    rte_value = ioapic_rte_read(ioapic_id, (pin << 1) + 0x10) & ~0x100ff;

    // Trigger mode: 0: Edge 1: Level
    if (trigger_mode == 0b01) rte_value &= ~(1UL << 15);
    if (trigger_mode == 0b11) rte_value |= (1 << 15);

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
        return -1;
    }
    uint8_t  pin   = gsi - ioapic->gsi_start;
    uint16_t flags = destination;
    flags <<= 8;
    flags |= apic.irq[irq].flags;

    ioapic_enable(i, pin, vector, flags);
    return 0;
}

static void ioapic_init(uint8_t ioapic_id)
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
    return local_apic_read(APIC_REG_ID) >> 24;
}