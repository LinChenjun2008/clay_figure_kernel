// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <print.h>
#include <sync/mutex.h>
#include <sync/semaphore.h>
#include <task.h>

void init_mutex(struct mutex *mutex)
{
    mutex->holder = NULL;
    init_semaphore(&mutex->lock, 1);
    return;
}

void mutex_lock(struct mutex *mutex)
{
    struct task *task = get_current_task();
    ASSERT(task != mutex->holder);
    sema_down(&mutex->lock);
    task->preempt_count++;
    mutex->holder = task;
    return;
}

void mutex_unlock(struct mutex *mutex)
{
    struct task *task = get_current_task();
    ASSERT(task == mutex->holder);
    ASSERT(task->preempt_count > 0);
    task->preempt_count--;
    mutex->holder = NULL;
    sema_up(&mutex->lock);
    return;
}