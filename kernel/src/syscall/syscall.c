// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/ptrace.h>
#include <asm/syscall.h>

#include <syscall.h>
#include <syscall/cap.h>
#include <syscall/cap/object.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/struct.h>

// syscall functions
#include <asm/task/signal.h>

#include <errno.h>
#include <mem.h>
#include <std/string.h>
#include <task/fork.h>
#include <task/process.h>
#include <task/signal.h>
#include <task/wait.h>

void *syscall_table[NR_CONT];

static word_t sys_exit(struct pt_regs *regs)
{
    process_exit((int)regs->rsi);
    return 0; // process_exit 不返回
}

static word_t sys_fork(struct pt_regs *regs)
{
    (void)regs;
    return (word_t)process_fork();
}

static word_t sys_wait(struct pt_regs *regs)
{
    word_t ret = -1;
    ret = task_waitpid((pid_t)regs->rsi, (int *)regs->rdx, (int)regs->r10);
    return ret;
}

static word_t sys_send(struct pt_regs *regs)
{
    struct task *task    = get_current_task();
    cap_handle_t handle  = (cap_handle_t)regs->rsi;
    pid_t        dst_pid = PID_NULL;

    struct cap_head *head = cap_lookup(task->cnode, handle, CAP_WRITE);
    if (head == NULL)
    {
        return -EACCES;
    }
    if (head->type == CAP_IPC)
    {
        struct cap_ipc *cap = (struct cap_ipc *)head;
        dst_pid             = cap->port;
    }
    cap_release(head);

    if (dst_pid == PID_NULL)
    {
        return -EACCES;
    }
    struct task *dst_task = pid_to_task(dst_pid);
    if (dst_task == NULL || TASK_STATUS(dst_task->status) == TASK_DIED)
    {
        cap_delete(task->cnode, handle);
        return -ESRCH;
    }
    return (word_t)ipc_send(dst_pid, (struct message *)regs->rdx);
}

static word_t sys_recv(struct pt_regs *regs)
{
    return (word_t)ipc_recv((pid_t)regs->rsi, (struct message *)regs->rdx);
}

static word_t sys_addr(struct pt_regs *regs)
{
    return mm_allocate_address((uintptr_t)regs->rsi, (size_t)regs->rdx);
}

static word_t sys_free(struct pt_regs *regs)
{
    mm_free_address((uintptr_t)regs->rsi, (size_t)regs->rdx);
    return 0;
}

static word_t sys_kill(struct pt_regs *regs)
{
    return send_signal_from_user((pid_t)regs->rsi, (int)regs->rdx);
}

static word_t sys_sigaction(struct pt_regs *regs)
{
    word_t ret;
    ret = sigaction((int)regs->rsi, (void *)regs->rdx, (void *)regs->r10);
    return ret;
}

static void register_syscall(uint64_t num, void *func)
{
    if (num >= NR_CONT)
    {
        return;
    }
    syscall_table[num] = func;
    return;
}

void syscall_init(void)
{
    int i;
    for (i = 0; i < NR_CONT; i++)
    {
        syscall_table[i] = NULL;
    }
    register_syscall(NR_EXIT, sys_exit);
    register_syscall(NR_FORK, sys_fork);
    register_syscall(NR_WAIT, sys_wait);
    register_syscall(NR_SEND, sys_send);
    register_syscall(NR_RECV, sys_recv);
    register_syscall(NR_ADDR, sys_addr);
    register_syscall(NR_FREE, sys_free);
    register_syscall(NR_KILL, sys_kill);
    register_syscall(NR_SACT, sys_sigaction);
    register_syscall(NR_SRET, sys_sigret);
    return;
}

void syscall_enable(void)
{
    arch_syscall_enable();
    return;
}
