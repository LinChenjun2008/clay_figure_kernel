// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib/linked_list.h>
#include <print.h>
#include <sync/semaphore.h>
#include <task.h>
#include <task/schedule.h>
#include <task/struct.h>

void init_semaphore(struct semaphore *sema, long value)
{
    init_spinlock(&sema->lock);
    sema->value = value;
    init_list(&sema->wait_list);
    return;
}

static int sema_down_sub(struct semaphore *sema, struct task *task)
{
    ASSERT(!list_find(&sema->wait_list, &task->sema_node));
    sema->value--;
    if (sema->value >= 0)
    {
        return 0;
    }
    list_append(&sema->wait_list, &task->sema_node);
    return 1;
}

void sema_down(struct semaphore *sema)
{
    struct task *task = get_current_task();

    spin_lock(&sema->lock);
    int need_block = sema_down_sub(sema, task);
    spin_unlock(&sema->lock);
    if (need_block)
    {
        task_block(TASK_BLOCKED);
    }
    return;
}

void sema_up(struct semaphore *sema)
{
    struct task *wake = NULL;

    spin_lock(&sema->lock);

    sema->value++;
    if (sema->value <= 0)
    {
        ASSERT(!list_empty(&sema->wait_list));
        struct list_node *node = list_pop(&sema->wait_list);

        ASSERT(list_len(&sema->wait_list) == (size_t)-sema->value);
        wake = CONTAINER_OF(struct task, sema_node, node);
    }
    spin_unlock(&sema->lock);

    if (wake != NULL)
    {
        task_unblock(wake->pid);
    }
    return;
}