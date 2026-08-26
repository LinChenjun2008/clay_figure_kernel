// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_DRIVERS_APIC_LAPIC_H__
#define __ASM_DRIVERS_APIC_LAPIC_H__

uint32_t local_apic_read(uint16_t index);
void     local_apic_write(uint16_t index, uint32_t value);
void     local_apic_init(void);

#endif /* __ASM_DRIVERS_APIC_LAPIC_H__ */