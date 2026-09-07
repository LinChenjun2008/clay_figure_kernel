// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/task/fork.h>
#include <asm/task/process.h>

#include <mem.h>
#include <task.h>
#include <task/schedule.h>

int copy_process(struct task *dst, struct task *src)
{
    if (copy_mm_struct(dst, src) < 0)
    {
        return -1;
    }
    // 刷新页表(cow)
    task_pg_active(src);
    dst->ustack_sp = src->ustack_sp;

    uintptr_t src_kstack, dst_kstack;

    src_kstack = src->kstack_base + (src->kstack_pages << PG_SIZE_SHIFT);
    dst_kstack = dst->kstack_base + (dst->kstack_pages << PG_SIZE_SHIFT);

    dst_kstack -= sizeof(struct pt_regs);
    src_kstack -= sizeof(struct pt_regs);

    struct pt_regs *src_regs = (struct pt_regs *)src_kstack;
    struct pt_regs *dst_regs = (struct pt_regs *)dst_kstack;

    *dst_regs     = *src_regs;
    dst_regs->rax = 0;

    dst_kstack -= sizeof(void *);
    *(void **)dst_kstack = arch_switch_to_user;

    dst_kstack -= sizeof(*dst->context);
    dst->context      = (struct task_context *)dst_kstack;
    dst->context->rdi = (uint64_t)dst_regs;
    return 0;
}
