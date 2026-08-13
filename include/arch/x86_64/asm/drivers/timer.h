// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_DRIVERS_TIMER_H__
#define __ASM_DRIVERS_TIMER_H__

#define TIMER_FREQUENCY 1000UL

// apic_timer.c
void apic_timer_init(void);

// hpet.c
uint64_t get_nano_time(void);
void     hpet_init(struct boot_info *boot_info);

// timer.c
void timer_init(struct boot_info *boot_info);

#endif /* __ASM_DRIVERS_TIMER_H__ */