// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/sysinfo.h>

#include <sysinfo.h>

struct task_mgr *get_task_mgr(void)
{
    enum intr_status intr_status = intr_disable();
    struct task_mgr *task_mgr    = arch_get_task_mgr();
    intr_set_status(intr_status);
    return task_mgr;
}

uint8_t get_current_cpu_id(void)
{
    return arch_get_current_cpu_id();
}

struct cpu *get_cpu_struct(uint8_t cpu_id)
{
    struct task_mgr *task_mgr = get_task_mgr();
    return &task_mgr->cpus[cpu_id];
}

struct cpu *get_curr_cpu_struct(void)
{
    uint8_t cpu_id = get_current_cpu_id();
    return get_cpu_struct(cpu_id);
}

struct page_mgr *get_page_mgr(void)
{
    struct task_mgr *task_mgr = get_task_mgr();
    return task_mgr->system_info->page_mgr;
}

struct system_info *get_system_info(void)
{
    struct task_mgr *task_mgr = get_task_mgr();
    return task_mgr->system_info;
}

void set_cpu_struct(struct cpu *cpu)
{
    arch_set_cpu_struct(cpu);
    return;
}
