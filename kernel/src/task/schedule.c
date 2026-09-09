// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/page.h>
#include <asm/task.h>

#include <panic.h>
#include <sysinfo.h>
#include <task.h>
#include <task/schedule.h>

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

static uint8_t select_idle_cpu(struct task *task)
{
    struct task_mgr *task_mgr = get_task_mgr();
    uint8_t          curr_cpu = get_current_cpu_id();
    if (get_cpu_struct(task->cpu_id)->main_task == task)
    {
        return task->cpu_id;
    }
    struct cpu *cur = get_cpu_struct(curr_cpu);
    spin_lock(&cur->lock);
    uint64_t min_after = cur->total_weight + task_prio_to_weight[task->prio];
    spin_unlock(&cur->lock);

    uint8_t best_cpu = curr_cpu;
    int     i;
    for (i = 0; i < task_mgr->max_cpus; i++)
    {
        if (i == curr_cpu)
        {
            continue; /* 当前CPU已作为基准 */
        }
        struct cpu *cpu = &task_mgr->cpus[i];
        if (cpu->main_task == NULL)
        {
            continue; /* CPU未启动 */
        }

        spin_lock(&cpu->lock);
        uint64_t after = cpu->total_weight + task_prio_to_weight[task->prio];
        spin_unlock(&cpu->lock);

        if (after < min_after)
        {
            min_after = after;
            best_cpu  = i;
        }
    }
    return best_cpu;
}

void cpu_task_enqueue(struct task *task)
{
    uint8_t cpu_id = select_idle_cpu(task);
    task->cpu_id   = cpu_id;

    struct cpu *cpu = get_cpu_struct(cpu_id);
    cpu_task_list_insert(cpu, task);
    return;
}

void task_pg_active(struct task *task)
{
    struct task_mgr *task_mgr = get_task_mgr();

    phys_addr_t pg_table = task_mgr->kernel_page_table_pos;
    if (task->pg_dir != 0)
    {
        pg_table = task->pg_dir;
    }
    set_pg_table(pg_table);
    return;
}

static void task_active(struct task *curr, struct task *next)
{
    next->cpu_id = curr->cpu_id;
    task_pg_active(next);
    arch_task_active(next);
    set_current_task(next);
    return;
}

static void inform_exit(struct task *task)
{
    struct task *parent_task = NULL;

    int need_retry = 1;
    do
    {
        parent_task = pid_to_task(task->ppid);

        spin_lock(&parent_task->childs_lock);
        if (list_find(&parent_task->childs_list, &task->parent_node))
        {
            list_append(&parent_task->exited_childs, &task->general_node);
            need_retry = 0;
        }
        spin_unlock(&parent_task->childs_lock);
    } while (need_retry);

    task_unblock(parent_task->pid);
    return;
}

static void check_dead_task(struct cpu *cpu)
{
    struct task *dead_task = cpu->dead_task;
    cpu->dead_task         = NULL;
    if (dead_task != NULL)
    {
        inform_exit(dead_task);
    }
    return;
}

static void set_dead_task(struct cpu *cpu, struct task *task)
{
    ASSERT(cpu->dead_task == NULL);
    cpu->dead_task = task;
    return;
}

static void switch_to(struct task *curr, struct task *next)
{
    next->status = TASK_RUNNING;
    if (next == curr)
    {
        return;
    }
    task_active(curr, next);
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

    check_dead_task(curr_cpu);
    switch (curr_task->status)
    {
        case TASK_RUNNING:
            cpu_task_list_insert(curr_cpu, curr_task);
            break;
        case TASK_DIED:
            set_dead_task(curr_cpu, curr_task);
            break;
        default:
            break;
    }
    struct task *next_task = cpu_get_next_task(curr_cpu);
    ASSERT(next_task != NULL);

    switch_to(curr_task, next_task);
    return;
}

void task_block(enum task_status status)
{
    enum intr_status intr_status = intr_disable();
    struct task     *task        = get_current_task();

    ASSERT(task->preempt_count == 0);

    spin_lock(&task->lock);

    task->status = status;

    int need_block = (int64_t)task->block_count++ >= 0;
    if (task->status != TASK_DIED && !need_block)
    {
        task->status = TASK_RUNNING;
    }

    spin_unlock(&task->lock);

    schedule();
    intr_set_status(intr_status);
    return;
}

void task_unblock(pid_t pid)
{
    enum intr_status intr_status = intr_disable();

    struct task *task = pid_to_task(pid);
    ASSERT(task != NULL);

    struct cpu *cpu = get_cpu_struct(task->cpu_id);

    spin_lock(&task->lock);
    task->block_count--;
    if (task->block_count == 0)
    {
        cpu_task_list_insert(cpu, task);
    }
    spin_unlock(&task->lock);

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