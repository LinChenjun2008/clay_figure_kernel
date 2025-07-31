// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <intr.h> // intr_enable,intr_set_status
#include <softirq.h>
#include <std/string.h>  // memset
#include <sync/atomic.h> // atomic_t
#include <task/task.h>   // running_task

PRIVATE struct
{
    atomic_t  status; // 标记软中断状态(位图)
    softirq_t actions[MAX_SOFTIRQ];
} softirq;

PUBLIC void softirq_init(void)
{
    atomic_set(&softirq.status, 0);
    memset(&softirq.actions, 0, sizeof(softirq.actions));
    return;
}

PUBLIC void register_softirq(uint8_t irq, softirq_action_t action, void *data)
{
    ASSERT(irq < MAX_SOFTIRQ);
    softirq.actions[irq].action = action;
    softirq.actions[irq].data   = data;
    return;
}

PUBLIC void unregister_softirq(uint8_t irq)
{
    ASSERT(irq < MAX_SOFTIRQ);
    softirq.actions[irq].action = NULL;
    softirq.actions[irq].data   = NULL;
    return;
}

PUBLIC void raise_softirq(uint8_t irq)
{
    ASSERT(irq < MAX_SOFTIRQ);
    atomic_bts(&softirq.status, irq);
    return;
}

PUBLIC void do_softirq(void)
{

    intr_status_t intr_status = intr_enable();

    int i;
    for (i = 0; i < 64; i++)
    {
        if (atomic_read(&softirq.status) == 0)
        {
            break;
        }
        if (atomic_btr(&softirq.status, i) == 1)
        {
            if (softirq.actions[i].action == NULL)
            {
                break;
            }
            softirq.actions[i].action(softirq.actions[i].data);
        }
    }

    intr_set_status(intr_status);

    return;
}