// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/mem/page.h>
#include <asm/task.h>

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

static void update_min_vrun_time(struct cpu *cpu, uint64_t vrun_time)
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
    uint64_t min_vrun_time = get_min_vrun_time(task->cpu);

    task->vrun_time = MAX_VRUNTIME(vrun_time, min_vrun_time);

    if (task->vrun_time != vrun_time)
    {
        // 如果task->vrun_time != vrun_time,表示当前vrun_time小于min_vrun_time,
        // 需要对vrun_time进行调整,防止长时间占用cpu.
        // 上文已经调整过vrun_time,此处调整run_time,使下次计算得到的vrun_time是正常值
        task->run_time = task->vrun_time * cur_weight / nice0_weight;
    }
    update_min_vrun_time(task->cpu, task->vrun_time);

    return;
}

uint64_t get_min_vrun_time(struct cpu *cpu)
{
    return cpu->min_vrun_time;
}

void task_update(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    task_struct_t *curr_task = get_current_task();
    curr_task->run_time++;
    update_vrun_time(curr_task);
    return;
}

static task_struct_t *cpu_get_next_task_lock(struct cpu *cpu)
{
    struct list_node *node = NULL;
    task_struct_t    *next = NULL;

    ASSERT(!list_empty(&cpu->task_queue));
    ASSERT(cpu->running_tasks == list_len(&cpu->task_queue));

    node = list_pop(&cpu->task_queue);
    ASSERT(node != NULL);
    next = CONTAINER_OF(task_struct_t, general_tag, node);
    ASSERT(next != NULL);

    cpu->running_tasks--;
    cpu->total_weight -= task_prio_to_weight[next->prio];
    ASSERT(cpu->running_tasks >= 0);
    return next;
}

static task_struct_t *cpu_get_next_task(struct cpu *cpu)
{
    task_struct_t *ret = NULL;

    spin_lock(&cpu->lock);
    ret = cpu_get_next_task_lock(cpu);
    spin_unlock(&cpu->lock);
    return ret;
}

static void cpu_task_list_insert_lock(struct cpu *cpu, task_struct_t *task)
{
    struct list      *list = &cpu->task_queue;
    struct list_node *node = list_next(list_head(list));
    task_struct_t    *tmp;
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
    ASSERT(cpu->running_tasks >= 0);
    ASSERT(cpu->running_tasks == list_len(&cpu->task_queue));

    task->status = TASK_READY;
    return;
}

void cpu_task_list_insert(struct cpu *cpu, task_struct_t *task)
{
    spin_lock(&cpu->lock);
    cpu_task_list_insert_lock(cpu, task);
    spin_unlock(&cpu->lock);
    return;
}

static void adjust_vrun_time(struct cpu *cpu, task_struct_t *task)
{
    int64_t balance = task->vrun_time - get_min_vrun_time(task->cpu);
    task->vrun_time = cpu->min_vrun_time + balance;
    return;
}

static void cpu_lock_double(struct cpu *cpu1, struct cpu *cpu2)
{
    ASSERT(cpu1 != cpu2);
    ASSERT(cpu1->id != cpu2->id);
    if (cpu1->id < cpu2->id)
    {
        spin_lock(&cpu1->lock);
        spin_lock(&cpu2->lock);
    }
    else
    {
        spin_lock(&cpu2->lock);
        spin_lock(&cpu1->lock);
    }
    return;
}

static void cpu_unlock_double(struct cpu *cpu1, struct cpu *cpu2)
{
    spin_unlock(&cpu1->lock);
    spin_unlock(&cpu2->lock);
    return;
}

static void do_task_balance_lock(struct cpu *busy, struct cpu *idle)
{
    task_struct_t *task = cpu_get_next_task_lock(busy);
    ASSERT(task != NULL);
    if (task == busy->main_task)
    {
        task = cpu_get_next_task_lock(busy);
        ASSERT(task != NULL);
        cpu_task_list_insert_lock(busy, busy->main_task);
    }
    adjust_vrun_time(idle, task);
    cpu_task_list_insert_lock(idle, task);
    return;
}

static void task_balance_lock(struct cpu *busy, struct cpu *idle)
{
    int max_running = busy->running_tasks;
    int min_running = idle->running_tasks;

    if (max_running <= 2)
    {
        return;
    }
    if (max_running - min_running <= 1)
    {
        return;
    }
    int task_should_be_move = (max_running - min_running) / 2;
    int i;
    for (i = 0; i < task_should_be_move; i++)
    {
        do_task_balance_lock(busy, idle);
    }
    return;
}

void task_balance(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    struct task_man *task_man = get_current_task()->cpu->task_man;

    int         min_running = task_man->max_tasks + 1;
    int         max_running = -1;
    int         cur_running = 0;
    struct cpu *cpu = NULL, *busy = NULL, *idle = NULL;
    int         i = 0;
    for (i = 0; i < task_man->max_cpus; i++)
    {
        cpu = &task_man->cpus[i];
        if (cpu->main_task == NULL)
        {
            continue;
        }

        spin_lock(&cpu->lock);
        cur_running = cpu->running_tasks;
        spin_unlock(&cpu->lock);

        if (cur_running > max_running)
        {
            max_running = cur_running;
            busy        = cpu;
        }
        if (cur_running < min_running)
        {
            min_running = cur_running;
            idle        = cpu;
        }
    }

    if (busy == idle)
    {
        return;
    }
    cpu_lock_double(busy, idle);
    task_balance_lock(busy, idle);
    cpu_unlock_double(busy, idle);
    return;
}

void task_page_table_active(task_struct_t *task)
{
    uint64_t *page_table = task->cpu->task_man->kernel_page_table_pos;
    if (task->page_dir != NULL)
    {
        page_table = task->page_dir;
    }
    set_page_table(page_table);
    return;
}

void task_active(task_struct_t *task)
{
    task->cpu = get_current_task()->cpu;
    task_page_table_active(task);
    arch_task_active(task);
    set_current_task(task);
    task->status = TASK_RUNNING;
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
    task_struct_t *curr_task = get_current_task();
    struct cpu    *curr_cpu  = curr_task->cpu;

    if (curr_task->preempt_count > 0)
    {
        return;
    }

    if (curr_task->status == TASK_RUNNING)
    {
        cpu_task_list_insert(curr_cpu, curr_task);
    }

    task_struct_t *next_task = cpu_get_next_task(curr_cpu);
    ASSERT(next_task != NULL);

    task_active(next_task);
    switch_to(curr_task, next_task);
    return;
}

void task_block(task_status_t status)
{
    intr_status_t  intr_status = intr_disable();
    task_struct_t *curr_task   = get_current_task();
    ASSERT(curr_task->preempt_count == 0);
    ASSERT(curr_task->status != TASK_READY);
    ASSERT(curr_task->status != TASK_RUNNING);
    curr_task->status = status;
    schedule();
    intr_set_status(intr_status);
    return;
}

void task_unblock(pid_t pid)
{
    intr_status_t  intr_status = intr_disable();
    task_struct_t *task        = pid_to_task(pid);
    ASSERT(task->status != TASK_READY);
    ASSERT(task->status != TASK_RUNNING);
    task->status = TASK_READY;
    cpu_task_list_insert(task->cpu, task);
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
