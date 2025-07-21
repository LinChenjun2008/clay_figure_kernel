// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#define IRQ0_FREQUENCY  1000UL
#define INPUT_FREQUENCY 1193180

#define HPET_GCAP_ID    00
#define HPET_GEN_CONF   0x10
#define HPET_MAIN_CNT   0xf0
#define HPET_TIME0_CONF 0x100
#define HPET_TIME0_COMP 0x108

#define COUNTER0_VALUE    (INPUT_FREQUENCY / IRQ0_FREQUENCY)
#define COUNTER0_VALUE_LO ((INPUT_FREQUENCY / IRQ0_FREQUENCY) & 0xff)
#define COUNTER0_VALUE_HI (((INPUT_FREQUENCY / IRQ0_FREQUENCY) >> 8) & 0xff)

#define MSECOND_TO_TICKS(MS)   (MS * IRQ0_FREQUENCY / 1000) // MS * 1000 / freq
#define TICKS_TO_MSCOND(TICKS) (TICK * IRQ0_FREQUENCY / 1000)

PUBLIC void     pit_init(void);
PUBLIC void     apic_timer_init(void);
PUBLIC uint64_t get_current_ticks(void);
PUBLIC uint64_t get_nano_time(void);

#endif