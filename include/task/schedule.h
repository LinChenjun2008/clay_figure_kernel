// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_SCHEDULE_H__
#define __TASK_SCHEDULE_H__

#include <lib/linked_list.h>
#include <sync/spinlock.h>

struct cpu;
struct task;

typedef int (*wq_cond_t)(void *);

struct wait_queue
{
    struct spinlock lock;
    struct list     queue;
    wq_cond_t       condition;
};

void cpu_task_list_insert(struct cpu *cpu, struct task *task);
void cpu_task_enqueue(struct task *task);

uint64_t get_min_vrun_time(struct cpu *cpu);
void     task_update(void);
void     task_pg_active(struct task *task);
void     schedule(void);

void init_wait_queue(struct wait_queue *wq, wq_cond_t condition);
int  wait_event(struct wait_queue *wq, void *arg, uint32_t status);
void wake_up(struct wait_queue *wq);
void wake_up_signal(pid_t pid);

void task_yield(void);


#endif /* __TASK_SCHEDULE_H__ */