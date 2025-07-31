// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#ifndef __SOFTIRQ_H__
#define __SOFTIRQ_H__

#define MAX_SOFTIRQ 64

#define SOFTIRQ_TIMER    0
#define SOFTIRQ_KEYBOARD 1

typedef void (*softirq_action_t)(void *);

typedef struct softirq_s
{
    softirq_action_t action;
    void            *data;
} softirq_t;

PUBLIC void softirq_init(void);
PUBLIC void register_softirq(uint8_t irq, softirq_action_t action, void *data);
PUBLIC void unregister_softirq(uint8_t irq);
PUBLIC void raise_softirq(uint8_t irq);
PUBLIC void do_softirq(void);

#endif