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

void create_task_context(task_struct_t *task, void *func, void *arg)
{
    ASSERT(task->context != NULL);
    uintptr_t kstack = (uintptr_t)task->context;
    kstack -= sizeof(uintptr_t);
    *(uintptr_t *)kstack = (uintptr_t)kernel_task;
    kstack -= sizeof(task_context_t);
    task->context           = (task_context_t *)kstack;
    task_context_t *context = task->context;
    context->rsi            = (uint64_t)arg;
    context->rdi            = (uint64_t)func;
    return;
}

void arch_set_current_task(task_struct_t *task)
{
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)task);
    return;
}

task_struct_t *arch_get_current_task(void)
{
    return (task_struct_t *)rdmsr(IA32_KERNEL_GS_BASE);
}

uint8_t arch_get_current_cpu_id()
{
    return apic_id();
}

SYSV_ABI void asm_switch_to(task_context_t **curr, task_context_t **next);

void arch_switch_to(task_context_t **curr, task_context_t **next)
{
    asm_switch_to(curr, next);
    return;
}

void arch_task_active(task_struct_t *task)
{
    if (task->page_dir != NULL)
    {
        update_tss_rsp0(task);
    }
    return;
}