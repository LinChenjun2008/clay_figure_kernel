// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h>   // apic_id
#include <device/timer.h> // MSECOND_TO_TICKS,get_current_ticks
#include <intr.h>         // intr functions
#include <task/task.h>    // task structs & functions,list,sse

extern taskmgr_t *tm;

PRIVATE void update_min_vruntime(core_taskmgr_t *taskmgr, uint64_t vruntime)
{
    uint64_t ideal_min_vruntime = vruntime;
    // task_struct_t *task = get_next_task(&taskmgr->task_list);
    // if (task != NULL)
    // {
    //     if (task->vrun_time < ideal_min_vruntime)
    //     {
    //         ideal_min_vruntime = task->vrun_time;
    //     }
    // }
    if (taskmgr->min_vruntime < ideal_min_vruntime)
    {
        taskmgr->min_vruntime = ideal_min_vruntime;
    }
    return;
}

PRIVATE void update_vruntime(task_struct_t *task)
{
    uint32_t cpu_id = apic_id();
    if (task->jiffies >= task->ideal_runtime)
    {
        task->jiffies       = 0;
        /// TODO: 根据权重计算ideal_runtime
        task->ideal_runtime = task->priority;
        task->vrun_time++;
        update_min_vruntime(&tm->core[cpu_id], task->vrun_time);
    }
    return;
}

PUBLIC uint64_t get_core_min_vruntime(uint32_t cpu_id)
{
    return tm->core[cpu_id].min_vruntime;
}

PUBLIC void task_update(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct_t *cur_task = running_task();
    cur_task->jiffies++;
    update_vruntime(cur_task);
    return;
}

PRIVATE void task_unblock_sub(pid_t pid)
{
    task_struct_t *task = pid2task(pid);
    ASSERT(task != NULL);
    ASSERT(task->status != TASK_READY);
    ASSERT(!list_find(&tm->core[task->cpu_id].task_list, &task->general_tag));
    task_list_insert(&tm->core[task->cpu_id].task_list, task);
    task->status = TASK_READY;
}

extern void ASMLINKAGE
            asm_switch_to(task_context_t **cur, task_context_t **next);
PUBLIC void schedule(void)
{
    task_struct_t *cur_task = running_task();
    uint32_t       cpu_id   = apic_id();
    if (cur_task->spinlock_count > 0)
    {
        return;
    }
    intr_status_t intr_status = intr_disable();
    if (cur_task->status == TASK_RUNNING)
    {
        spinlock_lock(&tm->core[cpu_id].task_list_lock);
        ASSERT(!list_find(&tm->core[cpu_id].task_list, &cur_task->general_tag));
        task_list_insert(&tm->core[cpu_id].task_list, cur_task);
        spinlock_unlock(&tm->core[cpu_id].task_list_lock);
        cur_task->status = TASK_READY;
    }
    task_struct_t *next = NULL;
    spinlock_lock(&tm->core[cpu_id].task_list_lock);
    next = get_next_task(&tm->core[cpu_id].task_list);

    if (next == NULL)
    {
        task_unblock_sub(tm->core[cpu_id].idle_task->pid);
        next = get_next_task(&tm->core[cpu_id].task_list);
        ASSERT(next != NULL);
    }

    list_remove(&next->general_tag);
    spinlock_unlock(&tm->core[cpu_id].task_list_lock);
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
    task_struct_t *task        = pid2task(pid);
    intr_status_t  intr_status = intr_disable();
    spinlock_lock(&tm->core[task->cpu_id].task_list_lock);

    task->status = TASK_READY;
    task_unblock_sub(pid);

    spinlock_unlock(&tm->core[task->cpu_id].task_list_lock);
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_yield(void)
{
    intr_status_t  intr_status = intr_disable();
    task_struct_t *task        = running_task();
    spinlock_lock(&tm->core[task->cpu_id].task_list_lock);
    task->status    = TASK_READY;
    task->vrun_time = get_core_min_vruntime(task->cpu_id);
    task_list_insert(&tm->core[task->cpu_id].task_list, task);
    spinlock_unlock(&tm->core[task->cpu_id].task_list_lock);
    schedule();
    intr_set_status(intr_status);
}

PUBLIC void task_msleep(uint32_t milliseconds)
{
    uint32_t timeout_tick =
        get_current_ticks() + MSECOND_TO_TICKS(milliseconds);

    while ((int64_t)(get_current_ticks() - timeout_tick) < 0)
    {
        task_yield();
    }
}