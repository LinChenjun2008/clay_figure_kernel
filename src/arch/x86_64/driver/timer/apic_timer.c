// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/asmfunc.h>
#include <asm/driver/apic.h>
#include <asm/driver/timer.h>

void apic_timer_init()
{
    local_apic_write(APIC_REG_TPR, 0); // Set TPR
    local_apic_write(APIC_REG_TIMER_DIV, 0x00000003);

    local_apic_write(APIC_REG_TIMER_ICNT, -1);
    uint32_t apic_ticks = 0;
    uint64_t nano_time  = get_nano_time();
    nano_time += 8000000;
    while (get_nano_time() < nano_time) continue;

    // Stop APIC timer
    local_apic_write(APIC_REG_LVT_TIMER, 0x10000);
    apic_ticks = -local_apic_read(APIC_REG_TIMER_CCNT) + 1;

    // 1000 Hz
    apic_ticks >>= 3;

    local_apic_write(APIC_REG_LVT_TIMER, 0x80 | 0x20000);
    local_apic_write(APIC_REG_TIMER_DIV, 0x00000003);
    local_apic_write(APIC_REG_TIMER_ICNT, apic_ticks);
    return;
}