/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#ifndef __TIMER_H__
#define __TIMER_H__

#define IRQ0_FREQUENCY 1000
#define INPUT_FREQUENCY 1193180

#define COUNTER0_VALUE (INPUT_FREQUENCY / IRQ0_FREQUENCY)
#define COUNTER0_VALUE_LO ((INPUT_FREQUENCY / IRQ0_FREQUENCY)        & 0xff)
#define COUNTER0_VALUE_HI (((INPUT_FREQUENCY / IRQ0_FREQUENCY) >> 8) & 0xff)

PUBLIC void pit_init();

#endif