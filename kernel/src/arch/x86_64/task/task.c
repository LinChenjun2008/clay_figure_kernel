// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/interrupt.h>
#include <asm/task.h>
#include <asm/task/process.h>

#include <mem.h>
#include <mem/page.h>
#include <print.h>
#include <task.h>
#include <task/process.h>
#include <task/schedule.h>

static void kernel_task(int (*func)(uint64_t), uint64_t arg)
{
    intr_enable();
    int ret = func(arg);
    task_exit(ret);
    return;
}

static void kernel_process(void *file, void *arg)
{
    intr_disable();

    struct task *task = get_current_task();

    task->ustack_base = (uintptr_t)allocate_pages(task->ustack_pages);
    ASSERT(task->ustack_base != 0);
    if (task->ustack_base == 0)
    {
        process_exit(-1);
    }

    ASSERT(task->pg_dir != NULL);
    size_t    ustack_size  = task->ustack_pages * PG_SIZE;
    uint64_t *pg_dir       = task->pg_dir;
    void     *ustack       = VIRT_TO_PHYS(task->ustack_base);
    void     *ustack_vaddr = (void *)(USER_STACK_VADDR_TOP - ustack_size);
    page_map(pg_dir, ustack, ustack_vaddr, task->ustack_pages);
    set_page_flags(pg_dir, ustack_vaddr, PG_USER_FLAGS);
    task_pg_active(task);

    void *entry = load_segment(file);

    task_pg_active(task);
    switch_to_user(entry, arg);

    while (1);
}

void create_task_context(struct task *task, void *func, void *arg)
{
    ASSERT(task != NULL);
    ASSERT(task->context != NULL);

    // context此时处于栈顶
    uintptr_t kstack = (uintptr_t)task->context;

    // 为pt_regs预留空间
    kstack -= sizeof(struct pt_regs);

    // switch_to使用的返回地址
    kstack -= sizeof(void *);
    if (task->pg_dir == NULL && task->mm == NULL)
    {
        *(void **)kstack = kernel_task;
    }
    else
    {
        *(void **)kstack = kernel_process;
    }

    // 保存上下文所用的空间
    kstack -= sizeof(*task->context);
    struct task_context *context = (struct task_context *)kstack;
    task->context                = context;
    context->rsi                 = (uint64_t)arg;
    context->rdi                 = (uint64_t)func;
    return;
}

void ASMLINKAGE
asm_switch_to(struct task_context **curr, struct task_context **next);

void arch_switch_to(struct task *curr, struct task *next)
{
    asm_switch_to(&curr->context, &next->context);
    return;
}

void arch_task_active(struct task *task)
{
    if (task->pg_dir != NULL)
    {
        update_tss_rsp0(task);
    }
    return;
}
