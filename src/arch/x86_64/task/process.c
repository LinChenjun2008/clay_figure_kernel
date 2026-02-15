// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/x86.h>

#include <task.h>

static void process_start(int (*func)(void *), void *arg)
{
    func(arg);
    while (1);
}

SYSV_ABI void asm_switch_to_user(
    void    *process_start,
    void    *func,
    uint64_t kstack,
    uint64_t ustack,
    uint64_t rflags
);

void switch_to_user(void *func, uint64_t kstack)
{
    uint64_t rflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;
    uint64_t ustack = USER_STACK_VADDR_TOP;
    asm_switch_to_user(process_start, func, kstack, ustack, rflags);
    return;
}