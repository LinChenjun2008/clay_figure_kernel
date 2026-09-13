// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib/linked_list.h>
#include <panic.h>
#include <sync/semaphore.h>
#include <task.h>
#include <task/schedule.h>
#include <task/struct.h>

static int acquire_semaphore(void *arg)
{
    struct semaphore *sema = arg;
    spin_lock(&sema->lock);
    int cond = sema->value > 0;
    spin_unlock(&sema->lock);
    return cond;
}

void init_semaphore(struct semaphore *sema, long value)
{
    init_spinlock(&sema->lock);
    sema->value = value;
    init_wait_queue(&sema->wq, acquire_semaphore);
    return;
}

void sema_down(struct semaphore *sema)
{

    while (1)
    {
        wait_event(&sema->wq, sema, TASK_BLOCKED | UNINTERRUPTABLE);
        int success = 0;
        spin_lock(&sema->lock);
        if (sema->value > 0)
        {
            sema->value--;
            success = 1;
        }
        spin_unlock(&sema->lock);
        if (success)
        {
            break;
        }
    }
    return;
}

void sema_up(struct semaphore *sema)
{
    spin_lock(&sema->lock);
    sema->value++;
    spin_unlock(&sema->lock);
    wake_up(&sema->wq);

    return;
}