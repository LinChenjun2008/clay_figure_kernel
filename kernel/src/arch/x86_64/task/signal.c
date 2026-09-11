// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/task/process.h>
#include <asm/task/signal.h>
#include <asm/x86.h>

#include <panic.h>
#include <std/string.h>
#include <task.h>
#include <task/process.h>
#include <task/signal.h>
#include <task/struct.h>

extern uint8_t SIG_TRAMPOLINE_START[];
extern uint8_t SIG_TRAMPOLINE_END[];

static int setup_sigframe(
    struct task    *task,
    struct pt_regs *regs,
    int             sig,
    siginfo_t      *info
)
{
    uintptr_t sp = regs->rsp - sizeof(struct sigframe);
    sp &= ~0xfULL;
    sp -= 8;

    uintptr_t stack_lo;
    stack_lo = USER_STACK_VADDR_TOP - (task->ustack_pages << PG_SIZE_SHIFT);
    if (sp < stack_lo || sp + sizeof(struct sigframe) > USER_STACK_VADDR_TOP)
    {
        return -1;
    }
    struct sigframe *frame = (void *)sp;

    frame->retcode = (word_t)&frame->trampoline;
    if (info)
    {
        frame->info = *info;
    }
    frame->signum  = (uint32_t)sig;
    frame->blocked = task->signal.blocked;
    frame->regs    = *regs;

    size_t sig_size;
    sig_size = (uintptr_t)SIG_TRAMPOLINE_END - (uintptr_t)SIG_TRAMPOLINE_START;
    ASSERT(sig_size == 16);
    memcpy(&frame->trampoline, SIG_TRAMPOLINE_START, sig_size);

    regs->rsp = (word_t)frame;
    regs->rflags &= ~EFLAGS_DF;
    regs->rip = (word_t)task->signal.actions[sig].sa_handler;
    regs->rdi = (word_t)sig;
    regs->rsi = (word_t)&frame->info;
    regs->rdx = 0; // ucontext = NULL

    regs->rbp = 0;

    return 0;
}

static int signal_deliver(struct task *task, struct pt_regs *regs, int sig)
{
    void (*handler)(int) = task->signal.actions[sig].sa_handler;
    if (handler == SIG_IGN)
    {
        return 0;
    }
    if (handler != SIG_DFL)
    {
        return setup_sigframe(task, regs, sig, &task->signal.info[sig]);
    }
    switch (sig)
    {
        case SIGCHLD:
        case SIGCONT:
            return 0;
        case SIGKILL:
        case SIGTERM:
        case SIGSEGV:
        default:
            return -(128 + sig);
            break;
    }
    return -1;
}

void signal_check(struct pt_regs *regs)
{
    struct task *task = get_current_task();
    if (task->status == TASK_DIED)
    {
        return;
    }
    if (task->pg_dir == 0)
    {
        return;
    }
    if ((regs->cs & 3) != 3)
    {
        return;
    }
    if (task->preempt_count > 0)
    {
        return;
    }
    uint32_t pending;
    int      sig    = -1;
    int      status = 0;
    spin_lock(&task->signal.lock);
    task->signal.blocked &= ~(1 << SIGKILL);
    task->signal.blocked &= ~(1 << SIGSTOP);

    pending = task->signal.pending & ~task->signal.blocked;
    for (sig = 1; sig < 32; sig++)
    {
        if (pending & (1ULL << sig))
        {
            task->signal.pending &= ~(1ULL << sig);
            break;
        }
    }
    if (sig != 32)
    {
        status = signal_deliver(task, regs, sig);
    }
    spin_unlock(&task->signal.lock);
    if (status != 0)
    {
        process_exit(status);
    }
    return;
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact)
{
    if (sig < 1 || sig > 31)
    {
        return -1;
    }
    if (sig == SIGKILL || sig == SIGSTOP)
    {
        return -1;
    }
    struct task *task = get_current_task();
    if (oldact)
    {
        *oldact = task->signal.actions[sig];
    }
    if (act)
    {
        task->signal.actions[sig] = *act;
    }
    return 0;
}

word_t sys_sigret(struct pt_regs *regs)
{
    struct task *task = get_current_task();

    // handler 的 ret 已弹出 retcode, 故 rsp 指向 frame->signum, 即 frame + 8
    struct sigframe *frame = (struct sigframe *)(regs->rsp - 8);

    task->signal.blocked = frame->blocked;
    *regs                = frame->regs; // 整体恢复被打断的用户现场

    return regs->rax;
}