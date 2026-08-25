// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/ptrace.h>
#include <asm/task/process.h>
#include <asm/x86.h>

#include <mem/page.h>
#include <print.h>
#include <std/string.h>
#include <sysinfo.h>
#include <task.h>

void *create_pg_dir(void)
{
    uint64_t *pg_dir = NULL;
    pg_dir           = allocate_a_page();
    if (pg_dir == 0)
    {
        return NULL;
    }
    struct task_mgr *task_mgr = get_task_mgr();

    uint64_t *kernel_pg_dir = PHYS_TO_VIRT(task_mgr->kernel_page_table_pos);
    memset(pg_dir, 0, PT_SIZE);
    memcpy(pg_dir + 0x100, kernel_pg_dir + 0x100, PT_SIZE / 2);
    return VIRT_TO_PHYS(pg_dir);
}

void ASMLINKAGE arch_switch_to_user(struct pt_regs *regs);

void switch_to_user(void *func, void *arg)
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

    regs->rdi = (uint64_t)arg;

    regs->rip    = (uint64_t)func;
    regs->cs     = SELECTOR_USER_CODE64;
    regs->rflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;
    regs->rsp    = USER_STACK_VADDR_TOP;
    regs->ss     = SELECTOR_USER_DATA64;

    arch_switch_to_user(regs);
    return;
}
