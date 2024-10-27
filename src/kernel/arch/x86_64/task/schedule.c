/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <task/task.h>  // task structs & functions,list,sse
#include <device/cpu.h> // apic_id
#include <intr.h>       // intr functions

#include <log.h>

extern taskmgr_t *tm;

PUBLIC void schedule()
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
    task->vrun_time = running_task()->vrun_time;

    ASSERT(!list_find(&tm->core[task->cpu_id].task_list,&task->general_tag));
    task_list_insert(&tm->core[task->cpu_id].task_list,task);
    task->status = TASK_READY;
}

extern void ASMLINKAGE asm_switch_to(task_context_t **cur,task_context_t **next);
PUBLIC void do_schedule()
{
    task_struct_t *cur_task = running_task();
    uint32_t cpu_id = apic_id();
    if (cur_task->spinlock_count > 0)
    {
        return;
    }
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
    if (next_task_tag == NULL)
    {
        task_unblock_without_spinlock(tm->core[cpu_id].idle_task);
        next_task_tag = get_next_task(&tm->core[cpu_id].task_list);
        ASSERT(next_task_tag != NULL);
    }
    spinlock_unlock(&tm->core[cpu_id].task_list_lock);
    ASSERT(next_task_tag != NULL);
    next = CONTAINER_OF(task_struct_t,general_tag,next_task_tag);
    next->status = TASK_RUNNING;
    prog_activate(next);
    asm_fxsave(cur_task->fxsave_region);
    asm_fxrstor(next->fxsave_region);
    next->cpu_id = cpu_id;
    asm_switch_to(&cur_task->context,&next->context);
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

    task_unblock_without_spinlock(pid);

    spinlock_unlock(&tm->core[task->cpu_id].task_list_lock);
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_yield()
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