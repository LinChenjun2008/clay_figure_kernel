// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/interrupt.h>
#include <asm/task.h>

#include <print.h>
#include <task.h>

static void kernel_task(int (*func)(uint64_t), uint64_t arg)
{
    intr_enable();
    int ret = func(arg);
    task_exit(ret);
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
    *(void **)kstack = kernel_task;

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
