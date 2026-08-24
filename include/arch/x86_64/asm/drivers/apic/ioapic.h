// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_DRIVERS_APIC_IOAPIC_H__
#define __ASM_DRIVERS_APIC_IOAPIC_H__

uint64_t ioapic_rte_read(uint8_t ioapic_id, uint8_t index);
void     ioapic_rte_write(uint8_t ioapic_id, uint8_t index, uint64_t value);
int      ioapic_irq_enable(uint8_t irq, uint8_t vector, uint8_t destination);
void     ioapic_init(uint8_t ioapic_id);

#endif /* __ASM_DRIVERS_APIC_IOAPIC_H__ */