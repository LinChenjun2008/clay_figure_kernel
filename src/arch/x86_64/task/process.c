// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc/desc.h>
#include <asm/mem/page.h>
#include <asm/ptrace.h>
#include <asm/task.h>
#include <asm/x86.h>

#include <print.h>
#include <std/string.h>
#include <task.h>

SYSV_ABI void asm_switch_to_user(struct pt_regs *regs);

void switch_to_user(void *func)
{
    ASSERT(intr_get_status() == INTR_OFF);
    struct pt_regs *regs;
    uintptr_t       ustack = USER_STACK_VADDR_TOP;

    ustack -= sizeof(*regs);
    regs = (struct pt_regs *)ustack;
    memset(regs, 0, sizeof(*regs));

    regs->ds = SELECTOR_USER_DATA64;
    regs->es = SELECTOR_USER_DATA64;
    regs->fs = SELECTOR_USER_DATA64;
    regs->gs = SELECTOR_USER_DATA64;

    regs->rip    = (uint64_t)func;
    regs->rflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;

    asm_switch_to_user(regs);
    return;
}
