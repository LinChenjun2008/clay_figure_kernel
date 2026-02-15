// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/asmfunc.h>
#include <asm/desc/tss.h> // update_tss_rsp0
#include <asm/driver/apic.h>
#include <asm/x86.h>

#include <task.h>

void set_current_task(task_struct_t *task)
{
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)task);
    return;
}

task_struct_t *get_current_task(void)
{
    return (task_struct_t *)rdmsr(IA32_KERNEL_GS_BASE);
}

uint8_t get_current_cpu_id()
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