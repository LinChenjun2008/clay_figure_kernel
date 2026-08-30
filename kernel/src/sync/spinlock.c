// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/utils/atomic_ops.h>

#include <print.h>
#include <sync/spinlock.h>

void init_spinlock(struct spinlock *lk)
{
    arch_atomic_xchg(&lk->lock, 1);
    lk->intr_status = MAX_INTR_STATUS;
    return;
}

void spin_lock(struct spinlock *lk)
{
    enum intr_status intr_status = intr_disable();
    while (arch_atomic_xchg(&lk->lock, 0) == 0)
    {
        arch_pause();
        continue;
    }
    lk->intr_status = intr_status;
    return;
}

void spin_unlock(struct spinlock *lk)
{
    enum intr_status intr_status = lk->intr_status;
    lk->intr_status              = MAX_INTR_STATUS;

    uint64_t lk_value;
    lk_value = arch_atomic_xchg(&lk->lock, 1);
    ASSERT(lk_value == 0);
    (void)lk_value;

    intr_set_status(intr_status);
    return;
}