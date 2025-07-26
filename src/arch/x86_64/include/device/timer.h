// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#define IRQ0_FREQUENCY 1000UL

#define MS_TO_TICKS(MS) ((MS) * 1000 / IRQ0_FREQUENCY)
#define US_TO_TICKS(US) ((US) * 1000000 / IRQ0_FREQUENCY)
#define NS_TO_TICKS(NS) ((NS) * 1000000000 / IRQ0_FREQUENCY)

#define TICKS_TO_MS(TICKS) ((TICK) * 1000 / IRQ0_FREQUENCY)
#define TICKS_TO_US(TICKS) ((TICK) * 1000000 / IRQ0_FREQUENCY)
#define TICKS_TO_NS(TICKS) ((TICK) * 1000000000 / IRQ0_FREQUENCY)

PUBLIC void     pit_init(void);
PUBLIC void     apic_timer_init(void);
PUBLIC uint64_t get_current_ticks(void);
PUBLIC uint64_t get_nano_time(void);

#endif