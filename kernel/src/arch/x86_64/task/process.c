// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/ptrace.h>
#include <asm/task.h>
#include <asm/x86.h>

#include <print.h>
#include <std/string.h>
#include <syscall.h>
#include <task.h>

void ASMLINKAGE asm_switch_to_user(struct pt_regs *regs);

void ASMLINKAGE asm_exit(int status);

static void kernel_process(void *func)
{
    int ret = ((int (*)(void))func)();
    asm_exit(ret);
    return;
}

void switch_to_user(void *func)
{
    ASSERT(intr_get_status() == INTR_OFF);

    struct task *task = get_current_task();

    uintptr_t kstack = (uintptr_t)task->context;
    kstack += sizeof(*task->context);
    kstack += sizeof(void *);

    struct pt_regs *regs = (struct pt_regs *)kstack;
    memset(regs, 0, sizeof(*regs));

    regs->ds = SELECTOR_USER_DATA64;
    regs->es = SELECTOR_USER_DATA64;
    regs->fs = SELECTOR_USER_DATA64;
    regs->gs = SELECTOR_USER_DATA64;

    regs->rdi = (uint64_t)func;

    regs->rip    = (uint64_t)kernel_process;
    regs->cs     = SELECTOR_USER_CODE64;
    regs->rflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;
    regs->rsp    = USER_STACK_VADDR_TOP;
    regs->ss     = SELECTOR_USER_DATA64;

    asm_switch_to_user(regs);
    return;
}
