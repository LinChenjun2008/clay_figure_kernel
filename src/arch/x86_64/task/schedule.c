// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h>   // apic_id
#include <device/timer.h> // MS_TO_TICKS,get_current_ticks
#include <intr.h>         // intr functions
#include <task/task.h>    // task structs & functions,list,sse

PRIVATE void update_min_vruntime(cpu_task_man_t *task_man, uint64_t vruntime)
{
    uint64_t ideal_min_vruntime = vruntime;
    if (task_man->min_vruntime < ideal_min_vruntime)
    {
        task_man->min_vruntime = ideal_min_vruntime;
    }
    return;
}

PRIVATE void update_vruntime(task_struct_t *task)
{
    task->vrun_time         = task->jiffies / task->priority;
    cpu_task_man_t *cur_cpu = get_task_man(task->cpu_id);
    update_min_vruntime(cur_cpu, task->vrun_time);
    return;
}

PUBLIC uint64_t get_core_min_vruntime(uint32_t cpu_id)
{
    cpu_task_man_t *cur_cpu = get_task_man(cpu_id);
    return cur_cpu->min_vruntime;
}

PUBLIC void task_update(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct_t  *cur_task = running_task();
    cpu_task_man_t *cur_cpu  = get_task_man(cur_task->cpu_id);
    cur_task->jiffies++;
    cur_task->runtime++;
    if (cur_task->runtime >= cur_task->ideal_runtime)
    {
        cur_task->runtime = 0;

        // 可运行的总任务数
        uint64_t running_tasks = cur_cpu->running_tasks + 1;

        // 可运行的总任务数的总权重
        uint64_t total_weight = cur_cpu->total_weight + cur_task->priority;

        // 根据权重计算ideal_runtime
        uint64_t period_ns  = SCHED_MIN_GRANULARITY_NS * running_tasks;
        uint64_t runtime_ns = period_ns * cur_task->priority / total_weight;

        period_ns  = MAX(SCHED_LATENCY_NS, period_ns);
        runtime_ns = MAX(SCHED_MIN_GRANULARITY_NS, runtime_ns);

        cur_task->ideal_runtime = NS_TO_TICKS(runtime_ns);
    }
    update_vruntime(cur_task);
    return;
}

extern void ASMLINKAGE
asm_switch_to(task_context_t **cur, task_context_t **next);

PUBLIC void schedule(void)
{
    task_struct_t  *cur_task = running_task();
    uint32_t        cpu_id   = apic_id();
    cpu_task_man_t *cur_cpu  = get_task_man(cpu_id);

    if (cur_task->spinlock_count > 0)
    {
        return;
    }
    intr_status_t intr_status = intr_disable();
    if (cur_task->status == TASK_RUNNING)
    {
        spinlock_lock(&cur_cpu->task_list_lock);
        ASSERT(!list_find(&cur_cpu->task_list, &cur_task->general_tag));
        task_list_insert(cur_cpu, cur_task);
        spinlock_unlock(&cur_cpu->task_list_lock);
        cur_task->status = TASK_READY;
    }
    task_struct_t *next = NULL;
    spinlock_lock(&cur_cpu->task_list_lock);
    next = get_next_task(cur_cpu);

    ASSERT(next != NULL);

    list_remove(&next->general_tag);
    spinlock_unlock(&cur_cpu->task_list_lock);
    next->status = TASK_RUNNING;
    prog_activate(next);
    asm_fxsave(cur_task->fxsave_region);
    asm_fxrstor(next->fxsave_region);
    next->cpu_id = cpu_id;
    asm_switch_to(&cur_task->context, &next->context);

    intr_set_status(intr_status);
    return;
}

PUBLIC void task_block(task_status_t status)
{
    intr_status_t  intr_status = intr_disable();
    task_struct_t *cur_task    = running_task();
    ASSERT(cur_task->spinlock_count == 0);
    cur_task->status = status;
    schedule();
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_unblock(pid_t pid)
{
    task_struct_t  *task        = pid2task(pid);
    intr_status_t   intr_status = intr_disable();
    cpu_task_man_t *cur_cpu     = get_task_man(task->cpu_id);
    spinlock_lock(&cur_cpu->task_list_lock);

    task_unblock_sub(pid);

    spinlock_unlock(&cur_cpu->task_list_lock);
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_unblock_sub(pid_t pid)
{
    task_struct_t  *task    = pid2task(pid);
    cpu_task_man_t *cur_cpu = get_task_man(task->cpu_id);
    ASSERT(task != NULL);
    ASSERT(task->status != TASK_READY);
    ASSERT(!list_find(&cur_cpu->task_list, &task->general_tag));
    task_list_insert(cur_cpu, task);
    task->status = TASK_READY;
}

PUBLIC void task_yield(void)
{
    intr_status_t   intr_status = intr_disable();
    task_struct_t  *task        = running_task();
    cpu_task_man_t *cur_cpu     = get_task_man(task->cpu_id);
    spinlock_lock(&cur_cpu->task_list_lock);
    task->status    = TASK_READY;
    task->vrun_time = get_core_min_vruntime(task->cpu_id);
    task_list_insert(cur_cpu, task);
    spinlock_unlock(&cur_cpu->task_list_lock);
    schedule();
    intr_set_status(intr_status);
}

PUBLIC void task_msleep(uint32_t milliseconds)
{
    uint32_t timeout_tick = get_current_ticks() + MS_TO_TICKS(milliseconds);

    while ((int64_t)(get_current_ticks() - timeout_tick) < 0)
    {
        task_yield();
    }
}