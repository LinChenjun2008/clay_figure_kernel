// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 Lin Chenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <sync/spinlock.h>
#include <task/task.h> // get_current_task

PUBLIC void init_spinlock(spinlock_t *spinlock)
{
    spinlock->lock = 1;
    return;
}

extern void ASMLINKAGE asm_spinlock_lock(volatile uint64_t *lock);

PUBLIC void spinlock_lock(spinlock_t *spinlock)
{
    get_current_task()->preempt_count++;
    asm_spinlock_lock(&spinlock->lock);
    return;
}

PUBLIC void spinlock_unlock(spinlock_t *spinlock)
{
    spinlock->lock = 1;
    get_current_task()->preempt_count--;
    return;
}