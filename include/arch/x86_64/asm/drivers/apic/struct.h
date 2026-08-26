// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_DRIVERS_APIC_STRUCT_H__
#define __ASM_DRIVERS_APIC_STRUCT_H__

#include <drivers/acpi.h>

#define MAX_LAPIC  256
#define MAX_IOAPIC 256

#define IRQ_START 0x20
#define IRQ_CNT   0xe0

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

#endif /* __ASM_DRIVERS_APIC_STRUCT_H__ */