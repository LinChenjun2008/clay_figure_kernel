/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <task/task.h>  // task structs & functions,list,sse
#include <device/cpu.h> // apic_id
#include <intr.h>       // intr functions

#include <log.h>

extern taskmgr_t *tm;

PUBLIC void update_vruntime(task_struct_t *task)
{
    uint64_t ideal_vruntime = tm->core[task->cpu_id].min_vruntime;
    if ((int64_t)(task->vrun_time - ideal_vruntime) < 0)
    {
        task->vrun_time = ideal_vruntime;
    }
    return;
}

PRIVATE void update_min_vruntime(core_taskmgr_t *taskmgr,uint64_t vruntime)
{
    if (vruntime > taskmgr->min_vruntime)
    {
        taskmgr->min_vruntime = vruntime;
    }
    return;
}

PUBLIC void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct_t *cur_task = running_task();
    cur_task->jiffies++;
    if (cur_task->jiffies >= cur_task->ideal_runtime)
    {
        cur_task->jiffies       = 0;
        cur_task->ideal_runtime = cur_task->priority;
        cur_task->vrun_time++;
        do_schedule();
    }
    return;
}

PRIVATE void task_unblock_without_spinlock(pid_t pid)
{
    task_struct_t *task = pid2task(pid);
    ASSERT(task != NULL);
    ASSERT(task->status != TASK_READY);

    update_vruntime(task);

    ASSERT(!list_find(&tm->core[task->cpu_id].task_list,&task->general_tag));
    task_list_insert(&tm->core[task->cpu_id].task_list,task);
    task->status = TASK_READY;
}

extern void ASMLINKAGE asm_switch_to(task_context_t **cur,task_context_t **next);
PUBLIC void do_schedule(void)
{
    task_struct_t *cur_task = running_task();
    uint32_t cpu_id = apic_id();
    if (cur_task->spinlock_count > 0)
    {
        return;
    }
    intr_status_t intr_status = intr_disable();
    if (cur_task->status == TASK_RUNNING)
    {
        spinlock_lock(&tm->core[cpu_id].task_list_lock);
        ASSERT(!list_find(&tm->core[cpu_id].task_list,&cur_task->general_tag));
        task_list_insert(&tm->core[cpu_id].task_list,cur_task);
        spinlock_unlock(&tm->core[cpu_id].task_list_lock);
        cur_task->status  = TASK_READY;
    }
    task_struct_t *next = NULL;
    list_node_t *next_task_tag = NULL;

    spinlock_lock(&tm->core[cpu_id].task_list_lock);
    next_task_tag = get_next_task(&tm->core[cpu_id].task_list);
    next = CONTAINER_OF(task_struct_t,general_tag,next_task_tag);

    if (next_task_tag == NULL)
    {
        task_unblock_without_spinlock(tm->core[cpu_id].idle_task);
        next_task_tag = get_next_task(&tm->core[cpu_id].task_list);
        next = CONTAINER_OF(task_struct_t,general_tag,next_task_tag);
        ASSERT(next_task_tag != NULL);
    }
    spinlock_unlock(&tm->core[cpu_id].task_list_lock);
    update_vruntime(next);
    next->status = TASK_RUNNING;
    prog_activate(next);
    asm_fxsave(cur_task->fxsave_region);
    asm_fxrstor(next->fxsave_region);
    next->cpu_id = cpu_id;
    update_min_vruntime(&tm->core[cpu_id],next->vrun_time);
    asm_switch_to(&cur_task->context,&next->context);

    intr_set_status(intr_status);
    return;
}

PUBLIC void task_block(task_status_t status)
{
    intr_status_t intr_status = intr_disable();
    task_struct_t *cur_task = running_task();
    ASSERT(cur_task->spinlock_count == 0);
    cur_task->status = status;
    do_schedule();
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_unblock(pid_t pid)
{
    task_struct_t *task = pid2task(pid);
    intr_status_t intr_status = intr_disable();
    spinlock_lock(&tm->core[task->cpu_id].task_list_lock);

    task->status = TASK_READY;
    task_unblock_without_spinlock(pid);

    spinlock_unlock(&tm->core[task->cpu_id].task_list_lock);
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_yield(void)
{
    intr_status_t intr_status = intr_disable();
    task_struct_t *task = running_task();
    spinlock_lock(&tm->core[task->cpu_id].task_list_lock);
    task->status = TASK_READY;
    task_list_insert(&tm->core[task->cpu_id].task_list,task);
    spinlock_unlock(&tm->core[task->cpu_id].task_list_lock);
    do_schedule();
    intr_set_status(intr_status);
}