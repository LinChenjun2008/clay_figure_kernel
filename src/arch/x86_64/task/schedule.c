// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/timer.h>   // MS_TO_TICKS,get_current_ticks
#include <intr.h>           // intr functions
#include <kernel/syscall.h> // task_ipc_check
#include <task/task.h>      // task structs & functions,list,sse

PRIVATE const uint64_t task_prio_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548,  7620,  6100,  4904,  3906,
    /* -5  */ 3121,  2501,  1991,  1586,  1277,
    /* 0   */ 1024,  820,   655,   526,   423,
    /* +5  */ 335,   272,   215,   172,   137,
    /* +10 */ 110,   87,    70,    56,    45,
    /* +15 */ 36,    29,    23,    18,    15
};

PRIVATE void update_min_vrun_time(task_man_t *task_man, uint64_t vrun_time)
{
    uint64_t min_vrun_time     = vrun_time;
    uint64_t cur_min_vrun_time = task_man->min_vrun_time;

    task_man->min_vrun_time = MAX_VRUNTIME(cur_min_vrun_time, min_vrun_time);
    return;
}

PRIVATE void update_vrun_time(task_struct_t *task)
{
    uint64_t nice0_weight = task_prio_to_weight[DEFAULT_PRIORITY];
    uint64_t cur_weight   = task_prio_to_weight[task->priority];

    uint64_t vrun_time     = task->run_time * nice0_weight / cur_weight;
    uint64_t min_vrun_time = get_min_vrun_time(task->cpu_id);

    task->vrun_time = MAX_VRUNTIME(vrun_time, min_vrun_time);

    if (task->vrun_time != vrun_time)
    {
        // 如果task->vrun_time != vrun_time,表示当前vrun_time小于min_vrun_time,
        // 需要对vrun_time进行调整,防止长时间占用cpu.
        // 上文已经调整过vrun_time,此处调整run_time,使下次计算得到的vrun_time是正常值
        task->run_time = task->vrun_time * cur_weight / nice0_weight;
    }
    task_man_t *task_man = get_task_man(task->cpu_id);
    update_min_vrun_time(task_man, task->vrun_time);

    return;
}

PUBLIC uint64_t get_min_vrun_time(uint32_t cpu_id)
{
    task_man_t *task_man = get_task_man(cpu_id);
    return task_man->min_vrun_time;
}

PUBLIC void task_update(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct_t *cur_task = running_task();
    cur_task->run_time++;
    update_vrun_time(cur_task);
    return;
}

/**
 * @brief 检查send_recv_list中是否有可以运行的任务
 * @param task_man
 * @return
 */
PRIVATE void check_send_recv_list(task_man_t *task_man)
{
    if (list_empty(&task_man->send_recv_list))
    {
        return;
    }
    list_node_t *node      = list_head(&task_man->send_recv_list);
    list_node_t *node_next = list_next(node);
    do
    {
        node                = node_next;
        node_next           = list_next(node);
        task_struct_t *task = CONTAINER_OF(task_struct_t, general_tag, node);
        if (task_ipc_check(task->pid))
        {
            list_remove(node);
            spinlock_lock(&task_man->task_list_lock);
            task_list_insert(task_man, task);
            spinlock_unlock(&task_man->task_list_lock);
        }
    } while (node_next != list_tail(&task_man->send_recv_list));
    return;
}

/**
 * @brief 在列表中获取下一个可以运行的任务
 * @param task_man 任务管理结构
 * @return 成功将返回下一个任务结构,失败则返回NULL
 */
PRIVATE task_struct_t *get_next_task(task_man_t *task_man)
{
    list_t      *list = &task_man->task_list;
    list_node_t *node = NULL;
    if (!list_empty(list))
    {
        node = list_pop(list);
    }
    else
    {
        // 无任务可运行 - 唤醒idle
        task_unblock_sub(task_man->idle_task->pid);
        node = &task_man->idle_task->general_tag;
        list_remove(node);
    }
    task_struct_t *next = CONTAINER_OF(task_struct_t, general_tag, node);
    task_man->running_tasks--;
    task_man->total_weight -= task_prio_to_weight[next->priority];
    return next;
}

extern void ASMLINKAGE
asm_switch_to(task_context_t **cur, task_context_t **next);

PUBLIC void schedule(void)
{
    task_struct_t *cur_task = running_task();
    uint32_t       cpu_id   = cur_task->cpu_id;
    task_man_t    *task_man = get_task_man(cpu_id);

    if (cur_task->preempt_count > 0)
    {
        return;
    }

    intr_status_t intr_status = intr_disable();

    if (cur_task->status == TASK_RUNNING)
    {
        spinlock_lock(&task_man->task_list_lock);
        task_list_insert(task_man, cur_task);
        spinlock_unlock(&task_man->task_list_lock);
    }

    if (cur_task->status == TASK_SENDING || cur_task->status == TASK_RECEIVING)
    {
        list_append(&task_man->send_recv_list, &cur_task->general_tag);
    }
    check_send_recv_list(task_man);

    task_struct_t *next = NULL;

    spinlock_lock(&task_man->task_list_lock);
    next = get_next_task(task_man);
    spinlock_unlock(&task_man->task_list_lock);

    next->status = TASK_RUNNING;
    proc_activate(next);
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
    if (list_find(list, &task->general_tag))
    {
        PR_LOG(LOG_WARN, "this task is already in the list: %s.\n", task->name);
        return;
    }
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
    task->status = TASK_READY;
    return;
}

PUBLIC void task_block(task_status_t status)
{
    intr_status_t  intr_status = intr_disable();
    task_struct_t *cur_task    = running_task();
    ASSERT(cur_task->preempt_count == 0);
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
    task_list_insert(task_man, task);
    return;
}

PUBLIC void task_yield(void)
{
    intr_status_t intr_status = intr_disable();
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