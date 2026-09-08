// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/utils/atomic_ops.h>

#include <panic.h>
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

void spin_lock_double(struct spinlock *a, struct spinlock *b)
{
    if (a == b)
    {
        spin_lock(a);
        return;
    }
    if ((uintptr_t)a < (uintptr_t)b)
    {
        spin_lock(a);
        spin_lock(b);
    }
    else
    {
        spin_lock(b);
        spin_lock(a);
    }
    return;
}

void spin_unlock_double(struct spinlock *a, struct spinlock *b)
{
    if (a == b)
    {
        spin_unlock(a);
        return;
    }
    if ((uintptr_t)a < (uintptr_t)b)
    {
        spin_unlock(b);
        spin_unlock(a);
    }
    else
    {
        spin_unlock(a);
        spin_unlock(b);
    }
    return;
}
