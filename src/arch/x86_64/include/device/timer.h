// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright 2024-2025 LinChenjun
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#define IRQ0_FREQUENCY  1000
#define INPUT_FREQUENCY 1193180

#define COUNTER0_VALUE    (INPUT_FREQUENCY / IRQ0_FREQUENCY)
#define COUNTER0_VALUE_LO ((INPUT_FREQUENCY / IRQ0_FREQUENCY) & 0xff)
#define COUNTER0_VALUE_HI (((INPUT_FREQUENCY / IRQ0_FREQUENCY) >> 8) & 0xff)

#define MSECOND_TO_TICKS(MS) (MS * IRQ0_FREQUENCY / 1000)

PUBLIC void pit_init(void);
PUBLIC void apic_timer_init(void);

#endif