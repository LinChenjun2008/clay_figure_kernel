// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/sync/spinlock.h>
#include <asm/utils.h>

#include <print.h>

void init_spinlock(spinlock_t *lk)
{
    asm_atomic_xchg(&lk->lock, 1);
    lk->intr_status = MAX_INTR_STATUS;
    return;
}

void spin_lock(spinlock_t *lk)
{
    intr_status_t intr_status = intr_disable();
    while (asm_atomic_xchg(&lk->lock, 0) == 0)
    {
        continue;
    }
    lk->intr_status = intr_status;
    return;
}

void spin_unlock(spinlock_t *lk)
{
    intr_status_t intr_status = lk->intr_status;
    lk->intr_status           = MAX_INTR_STATUS;

    uint64_t lk_value;
    lk_value = asm_atomic_xchg(&lk->lock, 1);
    ASSERT(lk_value == 0);
    (void)lk_value;

    intr_set_status(intr_status);
    return;
}