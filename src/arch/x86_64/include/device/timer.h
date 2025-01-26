/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __TIMER_H__
#define __TIMER_H__

#define IRQ0_FREQUENCY 1000
#define INPUT_FREQUENCY 1193180

#define COUNTER0_VALUE (INPUT_FREQUENCY / IRQ0_FREQUENCY)
#define COUNTER0_VALUE_LO ((INPUT_FREQUENCY / IRQ0_FREQUENCY)        & 0xff)
#define COUNTER0_VALUE_HI (((INPUT_FREQUENCY / IRQ0_FREQUENCY) >> 8) & 0xff)

PUBLIC void pit_init();
PUBLIC void apic_timer_init();

#endif