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

PRIVATE const int task_prio_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548,  7620,  6100,  4904,  3906,
    /* -5  */ 3121,  2501,  1991,  1586,  1277,
    /* 0   */ 1024,  820,   655,   526,   423,
    /* +5  */ 335,   272,   215,   172,   137,
    /* +10 */ 110,   87,    70,    56,    45,
    /* +15 */ 36,    29,    23,    18,    15
};

PRIVATE void update_min_vruntime(task_man_t *task_man, uint64_t vruntime)
{
    uint64_t min_vruntime = vruntime;

    // 确保task_man->min_vruntime单调递增
    if ((int64_t)(task_man->min_vruntime - min_vruntime) < 0)
    {
        task_man->min_vruntime = min_vruntime;
    }
    return;
}

PRIVATE void update_vruntime(task_struct_t *task)
{
    uint64_t nice0_weight = task_prio_to_weight[DEFAULT_PRIORITY];
    uint64_t cur_weight   = task_prio_to_weight[task->priority];

    uint64_t vruntime     = task->jiffies * nice0_weight / cur_weight;
    uint64_t min_vruntime = get_min_vruntime(task->cpu_id);

    task->vrun_time = MAX_VRUNTIME(vruntime, min_vruntime);

    if (task->vrun_time != vruntime)
    {
        // 如果task->vrun_time != vruntime,表示当前vruntime小于min_vruntime,
        // 需要对vruntime进行调整,防止长时间占用cpu.
        // 上文已经调整过vruntime,此处调整jiffies,使下次计算得到的vruntime是正常值
        task->jiffies = task->vrun_time * cur_weight / nice0_weight;
    }
    task_man_t *task_man = get_task_man(task->cpu_id);
    update_min_vruntime(task_man, task->vrun_time);

    return;
}

PUBLIC uint64_t get_min_vruntime(uint32_t cpu_id)
{
    task_man_t *task_man = get_task_man(cpu_id);
    return task_man->min_vruntime;
}

PUBLIC void task_update(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct_t *cur_task = running_task();
    task_man_t    *task_man = get_task_man(cur_task->cpu_id);
    cur_task->jiffies++;
    cur_task->runtime++;
    if (cur_task->runtime >= cur_task->ideal_runtime)
    {
        cur_task->runtime = 0;

        // 可运行的总任务数
        uint64_t running_tasks = task_man->running_tasks + 1;

        // 可运行的总任务数的总权重
        uint64_t weight       = task_prio_to_weight[cur_task->priority];
        uint64_t total_weight = task_man->total_weight + weight;

        // 根据权重计算ideal_runtime
        uint64_t period_ns  = SCHED_MIN_GRANULARITY_NS * running_tasks;
        uint64_t runtime_ns = period_ns * weight / total_weight;

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
    task_struct_t *cur_task = running_task();
    uint32_t       cpu_id   = apic_id();
    task_man_t    *task_man = get_task_man(cpu_id);

    if (cur_task->spinlock_count > 0)
    {
        return;
    }
    intr_status_t intr_status = intr_disable();
    if (cur_task->status == TASK_RUNNING)
    {
        spinlock_lock(&task_man->task_list_lock);
        ASSERT(!list_find(&task_man->task_list, &cur_task->general_tag));
        task_list_insert(task_man, cur_task);
        spinlock_unlock(&task_man->task_list_lock);
        cur_task->status = TASK_READY;
    }
    task_struct_t *next = NULL;
    spinlock_lock(&task_man->task_list_lock);
    next = get_next_task(task_man);

    ASSERT(next != NULL);

    list_remove(&next->general_tag);
    spinlock_unlock(&task_man->task_list_lock);
    next->status = TASK_RUNNING;
    prog_activate(next);
    asm_fxsave(cur_task->fxsave_region);
    asm_fxrstor(next->fxsave_region);
    next->cpu_id = cpu_id;
    asm_switch_to(&cur_task->context, &next->context);

    intr_set_status(intr_status);
    return;
}

PUBLIC void task_list_insert(task_man_t *task_man, task_struct_t *task)
{
    ASSERT(task_man != NULL);
    list_t *list = &task_man->task_list;
    ASSERT(!list_find(list, &task->general_tag));
    list_node_t   *node = list->head.next;
    task_struct_t *tmp;
    while (node != &list->tail)
    {
        tmp = CONTAINER_OF(task_struct_t, general_tag, node);
        if ((int64_t)(task->vrun_time - tmp->vrun_time) < 0)
        {
            break;
        }
        node = list_next(node);
    }
    list_in(&task->general_tag, node);

    task_man->running_tasks++;
    task_man->total_weight += task_prio_to_weight[task->priority];

    return;
}

PRIVATE bool task_check(list_node_t *node, uint64_t arg)
{
    (void)arg;
    task_struct_t *task = CONTAINER_OF(task_struct_t, general_tag, node);
    if (atomic_read(&task->recv_flag) == 1)
    {
        return FALSE;
    }
    if ((int64_t)atomic_read(&task->send_flag) > 0)
    {
        task_struct_t *receiver = pid_to_task(task->send_to);
        atomic_set(&receiver->recv_flag, 0);
        return FALSE;
    }
    return TRUE;
}

PUBLIC task_struct_t *get_next_task(task_man_t *task_man)
{
    list_t        *list = &task_man->task_list;
    list_node_t   *node = list_traversal(list, task_check, 0);
    task_struct_t *next = NULL;
    if (node == NULL)
    {
        // 无任务可运行 - 唤醒idle
        task_unblock_sub(task_man->idle_task->pid);
        node = &task_man->idle_task->general_tag;
    }
    next = CONTAINER_OF(task_struct_t, general_tag, node);

    task_man->running_tasks--;
    task_man->total_weight -= task_prio_to_weight[next->priority];

    return next;
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
    intr_status_t intr_status = intr_disable();

    task_struct_t *task     = pid_to_task(pid);
    task_man_t    *task_man = get_task_man(task->cpu_id);

    spinlock_lock(&task_man->task_list_lock);
    task_unblock_sub(pid);
    spinlock_unlock(&task_man->task_list_lock);

    intr_set_status(intr_status);
    return;
}

PUBLIC void task_unblock_sub(pid_t pid)
{
    task_struct_t *task     = pid_to_task(pid);
    task_man_t    *task_man = get_task_man(task->cpu_id);
    ASSERT(task != NULL);
    ASSERT(task->status != TASK_READY);
    ASSERT(!list_find(&task_man->task_list, &task->general_tag));

    task_list_insert(task_man, task);
    task->status = TASK_READY;
    return;
}

PUBLIC void task_yield(void)
{
    intr_status_t  intr_status = intr_disable();
    task_struct_t *task        = running_task();
    task_man_t    *task_man    = get_task_man(task->cpu_id);
    spinlock_lock(&task_man->task_list_lock);

    task->status = TASK_READY;
    task_list_insert(task_man, task);

    spinlock_unlock(&task_man->task_list_lock);
    schedule();
    intr_set_status(intr_status);
    return;
}

PUBLIC void task_msleep(uint32_t milliseconds)
{
    uint32_t timeout_tick = get_current_ticks() + MS_TO_TICKS(milliseconds);

    while ((int64_t)(get_current_ticks() - timeout_tick) < 0)
    {
        task_yield();
    }
    return;
}