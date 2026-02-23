// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc/tss.h> // update_tss_rsp0
#include <asm/drivers/apic.h>
#include <asm/interrupt.h>
#include <asm/ptrace.h>
#include <asm/task.h>
#include <asm/utils.h>
#include <asm/x86.h>

#include <print.h>

static void kernel_task(uintptr_t func, uint64_t arg)
{
    intr_enable();
    ((void (*)(uint64_t))func)(arg);
    while (1) continue;
    return;
}

void create_task_context(struct task *task, void *func, void *arg)
{
    ASSERT(task->context != NULL);
    uintptr_t kstack = (uintptr_t)task->context;
    kstack -= sizeof(uintptr_t);
    *(uintptr_t *)kstack = (uintptr_t)kernel_task;
    kstack -= sizeof(struct task_context);
    struct task_context *context = (struct task_context *)kstack;
    task->context                = context;
    context->rsi                 = (uint64_t)arg;
    context->rdi                 = (uint64_t)func;
    return;
}

void arch_set_current_task(struct task *task)
{
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)task);
    return;
}

struct task *arch_get_current_task(void)
{
    return (struct task *)rdmsr(IA32_KERNEL_GS_BASE);
}

uint8_t arch_get_current_cpu_id()
{
    return apic_id();
}

void ASMLINKAGE
asm_switch_to(struct task_context **curr, struct task_context **next);

void arch_switch_to(struct task_context **curr, struct task_context **next)
{
    asm_switch_to(curr, next);
    return;
}

void arch_task_active(struct task *task)
{
    if (task->page_dir != NULL)
    {
        update_tss_rsp0(task);
    }
    return;
}