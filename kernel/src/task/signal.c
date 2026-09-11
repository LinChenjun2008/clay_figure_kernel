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

static int signal_ignored(struct task *task, int sig)
{
    void (*handler)(int) = task->signal.actions[sig].sa_handler;
    if (handler == SIG_IGN)
    {
        return 1;
    }
    if (handler != SIG_DFL)
    {
        return 0;
    }
    switch (sig)
    {
        case SIGCHLD:
        case SIGURG:
        case SIGWINCH:
        case SIGIO:
            return 1;
        default:
            return 0;
    }
}

// 只登记待处理信号, 不唤醒(唤醒方式由调用方决定)
// 返回 1 = 已登记, 0 = 被忽略
static int signal_queue(struct task *task, int sig, struct siginfo *info)
{
    struct siginfo new_info;
    memset(&new_info, 0, sizeof(new_info));
    if (info != NULL)
    {
        new_info = *info;
    }
    new_info.si_signo = sig;

    spin_lock(&task->signal.lock);
    if (signal_ignored(task, sig))
    {
        spin_unlock(&task->signal.lock);
        return 0;
    }
    task->signal.pending |= (1ULL << sig);
    task->signal.info[sig] = new_info;
    spin_unlock(&task->signal.lock);
    return 1;
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
    if (signal_queue(task, sig, info))
    {
        task_unblock(pid, WAKE_SIGNAL);
    }
    return 0;
}

int send_signal_from_user(pid_t pid, int sig)
{
    struct siginfo info;
    memset(&info, 0, sizeof(info));
    info.si_code = SI_USER;
    info.si_pid  = get_current_task()->pid;
    return send_signal(pid, sig, &info);
}

// 硬件异常导致的信号
int send_signal_fault(pid_t pid, int sig, int code, void *addr)
{
    struct siginfo info;
    memset(&info, 0, sizeof(info));
    info.si_code = code;
    info.si_addr = addr;
    return send_signal(pid, sig, &info);
}

// 子进程状态变化: 若父进程会处理 SIGCHLD 则登记它, 并唤醒父进程(WAKE_NORMAL)
int send_signal_child(pid_t pid, int code, pid_t child, int status)
{
    struct task *task = pid_to_task(pid);
    if (task == NULL || TASK_STATUS(task->status) == TASK_DIED)
    {
        return -ESRCH;
    }

    struct siginfo info;
    memset(&info, 0, sizeof(info));
    info.si_code   = code;
    info.si_pid    = child;
    info.si_status = status;

    signal_queue(task, SIGCHLD, &info);
    task_unblock(pid, WAKE_NORMAL);
    return 0;
}
