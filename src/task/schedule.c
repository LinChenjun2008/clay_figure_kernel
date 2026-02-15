// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/mem/page.h>

#include <print.h>
#include <task.h>

static const uint64_t task_prio_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548,  7620,  6100,  4904,  3906,
    /* -5  */ 3121,  2501,  1991,  1586,  1277,
    /* 0   */ 1024,  820,   655,   526,   423,
    /* +5  */ 335,   272,   215,   172,   137,
    /* +10 */ 110,   87,    70,    56,    45,
    /* +15 */ 36,    29,    23,    18,    15
};

static void update_min_vrun_time(cpu_t *cpu, uint64_t vrun_time)
{
    uint64_t min_vrun_time     = vrun_time;
    uint64_t cur_min_vrun_time = cpu->min_vrun_time;

    cpu->min_vrun_time = MAX_VRUNTIME(cur_min_vrun_time, min_vrun_time);
    return;
}

static void update_vrun_time(task_struct_t *task)
{
    uint64_t nice0_weight = task_prio_to_weight[DEFAULT_PRIO];
    uint64_t cur_weight   = task_prio_to_weight[task->prio];

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
    cpu_t *cpu = get_cpu_struct(task->cpu_id);
    update_min_vrun_time(cpu, task->vrun_time);

    return;
}

uint64_t get_min_vrun_time(uint32_t cpu_id)
{
    cpu_t *curr_cpu = get_cpu_struct(cpu_id);
    return curr_cpu->min_vrun_time;
}

void task_update(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct_t *curr_task = get_current_task();
    curr_task->run_time++;
    update_vrun_time(curr_task);
    return;
}

static task_struct_t *get_next_task_lock(cpu_t *cpu)
{
    list_node_t *node;
    node = list_pop(&cpu->task_list);

    task_struct_t *next = CONTAINER_OF(task_struct_t, general_tag, node);
    cpu->running_tasks--;
    cpu->total_weight -= task_prio_to_weight[next->prio];
    return next;
}

static void adjust_vrun_time(cpu_t *cpu, task_struct_t *task)
{
    int64_t balance = task->vrun_time - get_min_vrun_time(task->cpu_id);
    task->vrun_time = cpu->min_vrun_time + balance;
    return;
}

static void get_unblocked_task_lock(cpu_t *cpu)
{
    task_man_t *task_man = get_task_man();
    ASSERT(!list_empty(&task_man->blocked_tasks));

    list_node_t *node      = list_head(&task_man->blocked_tasks);
    list_node_t *node_next = list_next(node);

    while (node_next != list_tail(&task_man->blocked_tasks))
    {
        node                = node_next;
        node_next           = list_next(node);
        task_struct_t *task = CONTAINER_OF(task_struct_t, general_tag, node);

        if (task->status != TASK_READY)
        {
            continue;
        }
        task_man->unblocked_tasks--;
        list_remove(node);
        adjust_vrun_time(cpu, task);
        task_list_insert(cpu, task);
    }
    return;
}

static void task_list_insert_lock(cpu_t *cpu, task_struct_t *task)
{
    list_t        *list = &cpu->task_list;
    list_node_t   *node = list_next(list_head(list));
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
    cpu->running_tasks++;
    cpu->total_weight += task_prio_to_weight[task->prio];
    task->status = TASK_READY;
    return;
}

void task_list_insert(cpu_t *cpu, task_struct_t *task)
{
    spin_lock(&cpu->lock);
    task_list_insert_lock(cpu, task);
    spin_unlock(&cpu->lock);
    return;
}

void task_page_table_active(task_struct_t *task)
{
    cpu_t    *cpu        = get_cpu_struct(get_current_cpu_id());
    uint64_t *page_table = cpu->kernel_page_table_pos;
    if (task->page_dir != NULL)
    {
        page_table = task->page_dir;
    }
    set_page_table(page_table);
    return;
}

void task_active(task_struct_t *task)
{
    task->cpu_id = get_current_cpu_id();
    task_page_table_active(task);
    arch_task_active(task);
    set_current_task(task);
    return;
}

static void switch_to(task_struct_t *curr, task_struct_t *next)
{
    set_current_task(next);
    arch_switch_to(&curr->context, &next->context);
    return;
}

void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_man_t    *task_man  = get_task_man();
    task_struct_t *curr_task = get_current_task();
    uint32_t       cpu_id    = curr_task->cpu_id;
    cpu_t         *curr_cpu  = get_cpu_struct(cpu_id);

    if (curr_task->preempt_count > 0)
    {
        return;
    }

    if (curr_task->status == TASK_RUNNING)
    {
        task_list_insert(curr_cpu, curr_task);
    }

    spin_lock(&task_man->lock);
    if (task_man->unblocked_tasks != 0)
    {
        get_unblocked_task_lock(curr_cpu);
    }
    ASSERT(task_man->unblocked_tasks == 0);
    spin_unlock(&task_man->lock);

    task_struct_t *next = NULL;

    spin_lock(&curr_cpu->lock);
    next = get_next_task_lock(curr_cpu);
    spin_unlock(&curr_cpu->lock);

    ASSERT(next != NULL);

    next->status = TASK_RUNNING;

    task_active(next);
    switch_to(curr_task, next);
    return;
}

void task_block(task_status_t status)
{
    intr_status_t  intr_status = intr_disable();
    task_man_t    *task_man    = get_task_man();
    task_struct_t *curr_task   = get_current_task();
    ASSERT(curr_task->preempt_count == 0);
    ASSERT(status != TASK_READY && status != TASK_RUNNING);
    curr_task->status = status;

    spin_lock(&task_man->lock);
    list_append(&task_man->blocked_tasks, &curr_task->general_tag);
    ASSERT(list_find(&task_man->blocked_tasks, &curr_task->general_tag));
    spin_unlock(&task_man->lock);

    schedule();
    intr_set_status(intr_status);
    return;
}

void task_unblock(pid_t pid)
{
    intr_status_t  intr_status = intr_disable();
    task_man_t    *task_man    = get_task_man();
    task_struct_t *task        = pid_to_task(pid);
    ASSERT(task->status != TASK_READY && task->status != TASK_RUNNING);
    task->status = TASK_READY;

    spin_lock(&task_man->lock);
    ASSERT(list_find(&task_man->blocked_tasks, &task->general_tag));
    task_man->unblocked_tasks++;
    spin_unlock(&task_man->lock);

    intr_set_status(intr_status);
    return;
}

void task_yield(void)
{
    intr_status_t intr_status = intr_disable();
    schedule();
    intr_set_status(intr_status);
    return;
}
