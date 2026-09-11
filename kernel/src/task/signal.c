// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <errno.h>
#include <std/string.h>
#include <sync/spinlock.h>
#include <task.h>
#include <task/schedule.h>
#include <task/signal.h>

void init_signal(struct signal_struct *signal)
{
    init_spinlock(&signal->lock);
    signal->pending = 0;
    signal->blocked = 0;

    int i;
    for (i = 0; i < 32; i++)
    {
        memset(&signal->actions[i], 0, sizeof(signal->actions[0]));
        memset(&signal->info[i], 0, sizeof(signal->info[0]));
    }
    return;
}

void copy_signal(struct signal_struct *dst, struct signal_struct *src)
{
    init_signal(dst);
    dst->blocked = src->blocked;
    int i;
    for (i = 0; i < 32; i++)
    {
        memcpy(&dst->actions[i], &src->actions[i], sizeof(src->actions[0]));
    }
    return;
}

int send_signal(pid_t pid, int sig, struct siginfo *info)
{
    if (sig < 1 || sig > 31)
    {
        return -EINVAL;
    }
    struct task *task = pid_to_task(pid);
    if (task == NULL || TASK_STATUS(task->status) == TASK_DIED)
    {
        return -ESRCH;
    }
    spin_lock(&task->signal.lock);
    task->signal.pending |= (1ULL << sig);
    task->signal.info[sig] = *info;
    spin_unlock(&task->signal.lock);

    task_unblock(pid, WAKE_SIGNAL);
    return 0;
}
