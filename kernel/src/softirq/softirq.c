// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>

#include <panic.h>
#include <softirq.h>
#include <std/string.h>
#include <sync/spinlock.h>

struct softirq_handle
{
    void (*action)(void *);
    void *data;
};

struct softirq
{
    struct spinlock       lock;
    uint64_t              pending;
    uint64_t              blocked;
    struct softirq_handle handles[MAX_SOFTIRQ];
};

static struct softirq softirq;

void softirq_init(void)
{
    memset(&softirq, 0, sizeof(softirq));
    init_spinlock(&softirq.lock);
    softirq.pending = 0;
    softirq.blocked = -1ULL;
    return;
}

void register_softirq(uint8_t irq, void *handler, void *data)
{
    if (irq >= MAX_SOFTIRQ)
    {
        return;
    }
    spin_lock(&softirq.lock);
    softirq.handles[irq].action = handler;
    softirq.handles[irq].data   = data;
    softirq.blocked &= ~(1ULL << irq);
    spin_unlock(&softirq.lock);

    return;
}

void unregister_softirq(uint8_t irq)
{
    if (irq >= MAX_SOFTIRQ)
    {
        return;
    }
    spin_lock(&softirq.lock);
    softirq.handles[irq].action = NULL;
    softirq.handles[irq].data   = NULL;
    softirq.blocked |= (1ULL << irq);
    spin_unlock(&softirq.lock);

    return;
}

void raise_softirq(uint8_t irq)
{
    if (irq >= MAX_SOFTIRQ)
    {
        return;
    }
    spin_lock(&softirq.lock);
    softirq.pending |= (1ULL << irq);
    spin_unlock(&softirq.lock);
    return;
}

void softirq_handler(void)
{
    uint64_t pending_irq = 0;
    uint8_t  irq         = 0;
    spin_lock(&softirq.lock);
    pending_irq = softirq.pending & ~softirq.blocked;
    softirq.pending &= ~pending_irq;
    spin_unlock(&softirq.lock);
    if (!pending_irq)
    {
        return;
    }
    for (irq = 0; irq < MAX_SOFTIRQ; irq++)
    {
        if (!(pending_irq & (1ULL << irq)))
        {
            continue;
        }
        if (softirq.handles[irq].action == NULL)
        {
            continue;
        }
        enum intr_status intr_status = intr_enable();
        softirq.handles[irq].action(softirq.handles[irq].data);
        intr_set_status(intr_status);
    }
    return;
}