// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_SCHEDULE_H__
#define __TASK_SCHEDULE_H__

void cpu_task_list_insert(struct cpu *cpu, struct task *task);
void cpu_task_enqueue(struct task *task);

uint64_t get_min_vrun_time(struct cpu *cpu);
void     task_update(void);
void     task_pg_active(struct task *task);
void     schedule(void);

void task_block(enum task_status status);
void task_block_wait(enum task_status status, pid_t wait_for);
void task_unblock(pid_t pid);
void task_yield(void);


#endif /* __TASK_SCHEDULE_H__ */