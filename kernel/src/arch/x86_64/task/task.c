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

    ASSERT(task->pg_dir != NULL);

    size_t ustack_size  = task->ustack_pages << PG_SIZE_SHIFT;
    void  *ustack_vaddr = (void *)(USER_STACK_VADDR_TOP - ustack_size);
    if (mm_allocate_address(ustack_vaddr, task->ustack_pages) == NULL)
    {
        process_exit(-1);
    }

    task_pg_active(task);

    void *entry = load_segment(file);
    if (entry == NULL)
    {
        process_exit(-1);
    }

    switch_to_user(entry, arg);
    while (1);
    return;
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

void arch_task_active(struct task *task)
{
    if (task->pg_dir != NULL)
    {
        update_tss_rsp0(task);
    }
    return;
}
