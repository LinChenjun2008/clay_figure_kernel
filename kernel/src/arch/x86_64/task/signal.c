// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/task/process.h>
#include <asm/task/signal.h>
#include <asm/x86.h>

#include <errno.h>
#include <panic.h>
#include <std/string.h>
#include <task.h>
#include <task/process.h>
#include <task/signal.h>
#include <task/struct.h>

extern uint8_t SIG_TRAMPOLINE_START[];
extern uint8_t SIG_TRAMPOLINE_END[];

// 把内核 trap 现场保存成用户可读写的机器上下文
static void save_sigcontext(struct sigcontext *sc, const struct pt_regs *regs)
{
    sc->r15    = regs->r15;
    sc->r14    = regs->r14;
    sc->r13    = regs->r13;
    sc->r12    = regs->r12;
    sc->r11    = regs->r11;
    sc->r10    = regs->r10;
    sc->r9     = regs->r9;
    sc->r8     = regs->r8;
    sc->rdi    = regs->rdi;
    sc->rsi    = regs->rsi;
    sc->rbp    = regs->rbp;
    sc->rbx    = regs->rbx;
    sc->rdx    = regs->rdx;
    sc->rax    = regs->rax;
    sc->rcx    = regs->rcx;
    sc->rip    = regs->rip;
    sc->rflags = regs->rflags;
    sc->rsp    = regs->rsp;
    sc->cs     = regs->cs;
    sc->ss     = regs->ss;
    sc->ds     = regs->ds;
    sc->es     = regs->es;
    sc->fs     = regs->fs;
    sc->gs     = regs->gs;
    return;
}

// 用用户(可能改写过的)机器上下文覆盖 trap 现场
// int_vector/error_code 由内核维护, 不从用户态恢复
static void
restore_sigcontext(struct pt_regs *regs, const struct sigcontext *sc)
{
    regs->r15    = sc->r15;
    regs->r14    = sc->r14;
    regs->r13    = sc->r13;
    regs->r12    = sc->r12;
    regs->r11    = sc->r11;
    regs->r10    = sc->r10;
    regs->r9     = sc->r9;
    regs->r8     = sc->r8;
    regs->rdi    = sc->rdi;
    regs->rsi    = sc->rsi;
    regs->rbp    = sc->rbp;
    regs->rbx    = sc->rbx;
    regs->rdx    = sc->rdx;
    regs->rax    = sc->rax;
    regs->rcx    = sc->rcx;
    regs->rip    = sc->rip;
    regs->rflags = sc->rflags;
    regs->rsp    = sc->rsp;
    regs->cs     = sc->cs;
    regs->ss     = sc->ss;
    regs->ds     = sc->ds;
    regs->es     = sc->es;
    regs->fs     = sc->fs;
    regs->gs     = sc->gs;
    return;
}

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
        return -EFAULT;
    }
    struct sigframe *frame = (void *)sp;

    frame->retcode = (word_t)&frame->trampoline;
    frame->signum  = (uint32_t)sig;
    frame->blocked = task->signal.blocked;

    memset(&frame->info, 0, sizeof(frame->info));
    if (info)
    {
        frame->info = *info;
    }

    regs->rflags &= ~EFLAGS_DF;

    // ucontext
    frame->uc.uc_flags          = 0;
    frame->uc.uc_link           = NULL;
    frame->uc.uc_stack.ss_sp    = NULL;
    frame->uc.uc_stack.ss_flags = SS_DISABLE; /// TODO
    frame->uc.uc_stack.ss_size  = 0;
    frame->uc.uc_sigmask        = frame->blocked;
    save_sigcontext(&frame->uc.uc_mcontext, regs);

    size_t sig_size;
    sig_size = (uintptr_t)SIG_TRAMPOLINE_END - (uintptr_t)SIG_TRAMPOLINE_START;
    ASSERT(sig_size == 16);
    memcpy(&frame->trampoline, SIG_TRAMPOLINE_START, sig_size);

    regs->rsp = (word_t)frame;
    regs->rip = (word_t)task->signal.actions[sig].sa_handler;
    regs->rdi = (word_t)sig;
    regs->rsi = (word_t)&frame->info;
    regs->rdx = (word_t)&frame->uc;

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
        int ret = setup_sigframe(task, regs, sig, &task->signal.info[sig]);
        if (ret < 0)
        {
            return -(128 + SIGSEGV);
        }
        return 0;
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
    }
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
    if (status < 0)
    {
        process_exit(-status);
    }
    return;
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact)
{
    if (sig < 1 || sig > 31)
    {
        return -EINVAL;
    }
    if (sig == SIGKILL || sig == SIGSTOP)
    {
        return -EINVAL;
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

    // 通用寄存器/rip/rsp 取用户可能改写过的 uc_mcontext;
    // 段寄存器与 cs/ss/rflags 取当前 syscall 现场的合法值:
    struct sigcontext sc = frame->uc.uc_mcontext;
    sc.ds                = regs->ds;
    sc.es                = regs->es;
    sc.fs                = regs->fs;
    sc.gs                = regs->gs;
    sc.cs                = regs->cs;
    sc.ss                = regs->ss;
    sc.rflags            = regs->rflags;

    restore_sigcontext(regs, &sc); // 恢复被打断的用户现场

    return regs->rax;
}