// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/page.h>
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

static void cpu_task_list_insert_lock(struct cpu *cpu, struct task *task)
{
    struct list      *list = &cpu->task_queue;
    struct list_node *node = list_next(list_head(list));
    struct task      *tmp;

    ASSERT(list != NULL);
    while (node != list_tail(list))
    {
        tmp = CONTAINER_OF(struct task, general_node, node);
        if ((int64_t)(task->vrun_time - tmp->vrun_time) < 0)
        {
            break;
        }
        node = list_next(node);
    }
    ASSERT(node != NULL);
    list_insert(&task->general_node, node);

    cpu->running_tasks++;
    cpu->total_weight += task_prio_to_weight[task->prio];

    task->status = TASK_READY;
    return;
}

void cpu_task_list_insert(struct cpu *cpu, struct task *task)
{
    spin_lock(&cpu->lock);
    cpu_task_list_insert_lock(cpu, task);
    spin_unlock(&cpu->lock);
    return;
}

static void update_min_vrun_time(struct cpu *cpu, uint64_t vrun_time)
{
    uint64_t min_vrun_time     = vrun_time;
    uint64_t cur_min_vrun_time = cpu->min_vrun_time;
    cpu->min_vrun_time         = MAX_VRUNTIME(cur_min_vrun_time, min_vrun_time);
    return;
}

static void update_vrun_time(struct task *task)
{
    struct cpu *cpu = get_cpu_struct(task->cpu_id);

    uint64_t nice0_weight = task_prio_to_weight[DEFAULT_PRIO];
    uint64_t cur_weight   = task_prio_to_weight[task->prio];

    uint64_t vrun_time     = task->run_time * nice0_weight / cur_weight;
    uint64_t min_vrun_time = get_min_vrun_time(cpu);

    task->vrun_time = MAX_VRUNTIME(vrun_time, min_vrun_time);

    if (task->vrun_time != vrun_time)
    {
        // 如果task->vrun_time != vrun_time,表示当前vrun_time小于min_vrun_time,
        // 需要对vrun_time进行调整,防止长时间占用cpu.
        // 已经调整过vrun_time,此处调整run_time,使下次计算得到的vrun_time是正常值
        task->run_time = task->vrun_time * cur_weight / nice0_weight;
    }
    update_min_vrun_time(cpu, task->vrun_time);

    return;
}

uint64_t get_min_vrun_time(struct cpu *cpu)
{
    return cpu->min_vrun_time;
}

void task_update(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    struct task *curr_task = get_current_task();
    curr_task->run_time++;
    update_vrun_time(curr_task);
    return;
}

static struct task *cpu_get_next_task_lock(struct cpu *cpu)
{
    struct list_node *node = NULL;
    struct task      *next = NULL;

    ASSERT(!list_empty(&cpu->task_queue));
    ASSERT(cpu->running_tasks == list_len(&cpu->task_queue));

    node = list_pop(&cpu->task_queue);
    ASSERT(node != NULL);
    next = CONTAINER_OF(struct task, general_node, node);
    ASSERT(next != NULL);

    cpu->running_tasks--;
    cpu->total_weight -= task_prio_to_weight[next->prio];
    return next;
}

static struct task *cpu_get_next_task(struct cpu *cpu)
{
    struct task *ret = NULL;

    spin_lock(&cpu->lock);
    ret = cpu_get_next_task_lock(cpu);
    spin_unlock(&cpu->lock);
    return ret;
}

static void adjust_vrun_time(struct cpu *cpu, struct task *task)
{
    struct cpu *task_cpu = get_cpu_struct(task->cpu_id);

    int64_t balance = task->vrun_time - get_min_vrun_time(task_cpu);
    task->vrun_time = cpu->min_vrun_time + balance;
    return;
}

static void cpu_lock_double(uint8_t cpu1_id, uint8_t cpu2_id)
{
    ASSERT(cpu1_id != cpu2_id);
    struct cpu *cpu1 = get_cpu_struct(cpu1_id);
    struct cpu *cpu2 = get_cpu_struct(cpu2_id);
    if (cpu1_id < cpu2_id)
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

static void cpu_unlock_double(uint8_t cpu1_id, uint8_t cpu2_id)
{
    struct cpu *cpu1 = get_cpu_struct(cpu1_id);
    struct cpu *cpu2 = get_cpu_struct(cpu2_id);
    spin_unlock(&cpu1->lock);
    spin_unlock(&cpu2->lock);
    return;
}

static void do_task_balance_lock(struct cpu *busy, struct cpu *idle)
{
    struct task *task = cpu_get_next_task_lock(busy);
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
    struct task_mgr *task_mgr = get_task_mgr();

    int         min_running = task_mgr->max_tasks + 1;
    int         max_running = -1;
    int         cur_running = 0;
    struct cpu *cpu         = NULL;

    int busy_id = 0, idle_id = 0;
    int i = 0;
    for (i = 0; i < task_mgr->max_cpus; i++)
    {
        cpu = &task_mgr->cpus[i];
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
            busy_id     = i;
        }
        if (cur_running < min_running)
        {
            min_running = cur_running;
            idle_id     = i;
        }
    }

    if (busy_id == idle_id)
    {
        return;
    }
    cpu_lock_double(busy_id, idle_id);

    struct cpu *busy = get_cpu_struct(busy_id);
    struct cpu *idle = get_cpu_struct(idle_id);
    task_balance_lock(busy, idle);

    cpu_unlock_double(busy_id, idle_id);
    return;
}

void task_pg_active(struct task *task)
{
    struct task_mgr *task_mgr = get_task_mgr();

    void *pg_table = task_mgr->kernel_page_table_pos;
    if (task->pg_dir != NULL)
    {
        pg_table = task->pg_dir;
    }
    set_pg_table(pg_table);
    return;
}

void task_active(struct task *task)
{
    task->cpu_id = get_current_task()->cpu_id;
    task_pg_active(task);
    arch_task_active(task);

    set_current_task(task);
    task->status = TASK_RUNNING;
    return;
}

static void inform_exit(struct task *task)
{
    struct task *parent_task = pid_to_task(task->ppid);

    spin_lock(&parent_task->exited_lock);
    list_append(&parent_task->exited_childs, &task->general_node);
    spin_unlock(&parent_task->exited_lock);

    return;
}

static void switch_to(struct task *curr, struct task *next)
{
    arch_switch_to(&curr->context, &next->context);
    return;
}

void schedule(void)
{
    ASSERT(intr_get_status() == INTR_OFF);
    struct task *curr_task = get_current_task();
    struct cpu  *curr_cpu  = get_cpu_struct(curr_task->cpu_id);

    ASSERT(curr_cpu != NULL);
    if (curr_task->preempt_count > 0)
    {
        return;
    }

    switch (curr_task->status)
    {
        case TASK_RUNNING:
            cpu_task_list_insert(curr_cpu, curr_task);
            break;
        case TASK_DIED:
            inform_exit(curr_task);
            break;
        default:
            break;
    }
    struct task *next_task = cpu_get_next_task(curr_cpu);
    ASSERT(next_task != NULL);
    task_active(next_task);
    switch_to(curr_task, next_task);
    return;
}

void task_block(enum task_status status)
{
    enum intr_status intr_status = intr_disable();
    struct task     *task        = get_current_task();
    ASSERT(task->preempt_count == 0);

    task->status = status;
    schedule();
    intr_set_status(intr_status);
    return;
}

void task_unblock(pid_t pid)
{
    enum intr_status intr_status = intr_disable();

    struct task *task = pid_to_task(pid);
    struct cpu  *cpu  = get_cpu_struct(task->cpu_id);
    cpu_task_list_insert(cpu, task);

    intr_set_status(intr_status);
    return;
}

void task_yield(void)
{
    enum intr_status intr_status = intr_disable();
    schedule();
    intr_set_status(intr_status);
    return;
}