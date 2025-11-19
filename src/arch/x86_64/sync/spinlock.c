// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 Lin Chenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <sync/spinlock.h>

PUBLIC void init_spin(spinlock_t *spinlock)
{
    spinlock->lock        = 1;
    spinlock->intr_status = INTR_UNKNOW_STATUS;
    return;
}

extern void ASMLINKAGE asm_spin_lock(volatile uint64_t *lock);

PUBLIC void spin_lock(spinlock_t *spinlock)
{
    intr_status_t intr_status = intr_disable();
    asm_spin_lock(&spinlock->lock);
    spinlock->intr_status = intr_status;
    return;
}

PUBLIC void spin_unlock(spinlock_t *spinlock)
{
    intr_status_t intr_status = spinlock->intr_status;
    spinlock->intr_status     = INTR_UNKNOW_STATUS;
    spinlock->lock            = 1;
    intr_set_status(intr_status);
    return;
}