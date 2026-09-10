// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SOFTIRQ_H__
#define __SOFTIRQ_H__

#define MAX_SOFTIRQ 64

#define TIMER_SOFTIRQ 0

void softirq_init(void);
void register_softirq(uint8_t irq, void *handler, void *data);
void unregister_softirq(uint8_t irq);
void raise_softirq(uint8_t irq);
void softirq_handler(void);

#endif /* __SOFTIRQ_H__ */