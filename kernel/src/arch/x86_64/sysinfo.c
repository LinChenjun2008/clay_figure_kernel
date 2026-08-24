// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/sysinfo.h>
#include <asm/utils/regs.h>
#include <asm/x86.h>

#include <task/struct.h>

struct task_mgr *ASMLINKAGE asm_get_task_mgr(void);

struct task_mgr *arch_get_task_mgr(void)
{
    return asm_get_task_mgr();
}

uint8_t arch_get_current_cpu_id(void)
{
    return apic_id();
}

void arch_set_cpu_struct(struct cpu *cpu)
{
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)cpu);
    return;
}