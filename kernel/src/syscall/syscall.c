// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/ptrace.h>
#include <asm/syscall.h>

#include <syscall.h>
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

static int check_user_address(const void *addr, size_t size)
{
    if (addr == NULL)
    {
        return 0;
    }
    struct task *task = get_current_task();
    if (!IS_USER_TASK(task))
    {
        return 0;
    }
    return mm_check_addr(task, (void *)addr, size, ADDR_ALLOCATED);
}

static word_t sys_exit(struct pt_regs *regs)
{
    int status = (int)regs->rsi;

    process_exit(status);
    return 0; // process_exit 不返回
}

static word_t sys_fork(struct pt_regs *regs)
{
    (void)regs;
    return (word_t)process_fork();
}

static word_t sys_wait(struct pt_regs *regs)
{
    pid_t pid     = (pid_t)regs->rsi;
    int  *status  = (int *)regs->rdx;
    int   options = (int)regs->r10;
    int   ret;

    ret = check_user_address(status, sizeof(*status));
    if (ret < 0)
    {
        return (word_t)ret;
    }
    return (word_t)task_waitpid(pid, status, options);
}

static word_t sys_send(struct pt_regs *regs)
{
    struct msg_head *msg     = (struct msg_head *)regs->rsi;
    pid_t            dst_pid = (pid_t)regs->rdx;

    return (word_t)ipc_send(msg, dst_pid);
}

static word_t sys_recv(struct pt_regs *regs)
{
    struct msg_head *msg    = (struct msg_head *)regs->rsi;
    int              option = (int)regs->rdx;

    return (word_t)ipc_recv(msg, option);
}

static word_t sys_addr(struct pt_regs *regs)
{
    uintptr_t addr  = (uintptr_t)regs->rsi;
    size_t    pages = (size_t)regs->rdx;

    return mm_allocate_address(addr, pages);
}

static word_t sys_free(struct pt_regs *regs)
{
    uintptr_t addr  = (uintptr_t)regs->rsi;
    size_t    pages = (size_t)regs->rdx;

    mm_free_address(addr, pages);
    return 0;
}

static word_t sys_kill(struct pt_regs *regs)
{
    pid_t pid = (pid_t)regs->rsi;
    int   sig = (int)regs->rdx;

    return send_signal_from_user(pid, sig);
}

static word_t sys_sigaction(struct pt_regs *regs)
{
    int               sig    = (int)regs->rsi;
    struct sigaction *act    = (void *)regs->rdx;
    struct sigaction *oldact = (void *)regs->r10;
    int               ret;

    ret = check_user_address(act, sizeof(*act));
    if (ret < 0)
    {
        return (word_t)ret;
    }
    ret = check_user_address(oldact, sizeof(*oldact));
    if (ret < 0)
    {
        return (word_t)ret;
    }
    return (word_t)sigaction(sig, act, oldact);
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
