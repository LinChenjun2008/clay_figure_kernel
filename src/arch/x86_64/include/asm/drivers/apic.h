// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_DRIVER_APIC_H__
#define __ASM_DRIVER_APIC_H__

#define MADT_SIGNATURE SIGNATURE_32('A', 'P', 'I', 'C')

#define APIC_REG_ID         0x020
#define APIC_REG_VERSION    0x030
#define APIC_REG_TPR        0x080
#define APIC_REG_PPR        0x0a0
#define APIC_REG_EOI        0x0b0
#define APIC_REG_SVR        0x0f0
#define APIC_REG_ICR_LO     0x300
#define APIC_REG_ICR_HI     0x310
#define APIC_REG_LVT_TIMER  0x320
#define APIC_REG_LVT_LINT0  0x350
#define APIC_REG_LVT_LINT1  0x360
#define APIC_REG_LVT_ERROR  0x370
#define APIC_REG_TIMER_ICNT 0x380
#define APIC_REG_TIMER_CCNT 0x390
#define APIC_REG_TIMER_DIV  0x3e0

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define ICR_DELIVER_MODE_FIXED    0
#define ICR_DELIVER_MODE_SMI      2
#define ICR_DELIVER_MODE_NMI      4
#define ICR_DELIVER_MODE_INIT     5
#define ICR_DELIVER_MODE_START_UP 6

#define ICR_DEST_MODE_PHY   0
#define ICR_DEST_MODE_LOGIC 1

#define ICR_LEVEL_DE_ASSEST 0
#define ICR_LEVEL_ASSERT    1

#define ICR_TRIGGER_EDGE  0
#define ICR_TRIGGER_LEVEL 1

#define ICR_DELIVER_STATUS_IDLE         0
#define ICR_DELIVER_STATUS_SEND_PENDING 1

#define ICR_NO_SHORTHAND     0
#define ICR_SELF             1
#define ICR_ALL_INCLUDE_SELF 2
#define ICR_ALL_EXCLUDE_SELF 3

uint32_t local_apic_read(uint16_t index);
void     local_apic_write(uint16_t index, uint32_t value);
void     local_apic_init(void);

uint64_t ioapic_rte_read(uint8_t ioapic_id, uint8_t index);
void     ioapic_rte_write(uint8_t ioapic_id, uint8_t index, uint64_t value);
int      ioapic_irq_enable(uint8_t irq, uint8_t vector, uint8_t destination);
void     apic_init(boot_info_t *boot_info);
uint8_t  apic_cpu_count(void);
uint8_t  apic_max_lapic_id(void);
void     apic_send_eoi(void);
uint8_t  apic_id(void);

#endif /* __ASM_DRIVER_APIC_H__ */