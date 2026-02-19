// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_H__
#define __TASK_H__

// 最大支持的任务数
#define MAX_TASKS 32768

// PID的最小值
#define MIN_PID 0

// PID的最大值
#define MAX_PID (MAX_TASKS - 1)

#define PID_NO_TASK -1

#define MAX_NICE 20
#define MIN_NICE -19

#define NICE_TO_PRIO(NICE) ((NICE) + 20)
#define PRIO_TO_NICE(PRIO) ((PRIO) - 20)

#define DEFAULT_PRIO NICE_TO_PRIO(0)
#define SERVICE_PRIO NICE_TO_PRIO(-10)
#define IDLE_PRIO    NICE_TO_PRIO(20)

#define MAX_VRUNTIME(A, B) ((int64_t)((A) - (B)) > 0 ? (A) : (B))

#define USER_STACK_VADDR_TOP 0x0000800000000000
#define USER_VADDR_START     0x800000

#ifndef __ASSEMBLER__

#    include <task/struct.h>

typedef struct
{
    list_t task_list;

    uint64_t       min_vrun_time;
    uint64_t       running_tasks;
    uint64_t       total_weight;
    task_struct_t *main_task;
    void          *kernel_page_table_pos;
} cpu_t;

typedef struct
{
    spinlock_t      lock;
    task_struct_t **task_table;
    int             max_tasks;
    cpu_t          *cpus;
    int             max_cpus;
    list_t          blocked_tasks;
    volatile int    unblocked_tasks;
} task_man_t;

// task.c
void task_init(boot_info_t *boot_info, int max_tasks);
void make_main_task(uintptr_t stack_base, size_t stack_pages);

task_man_t    *get_task_man(void);
cpu_t         *get_cpu_struct(uint8_t id);
void           set_current_task(task_struct_t *task);
task_struct_t *get_current_task(void);
uint8_t        get_current_cpu_id(void);

task_struct_t *pid_to_task(pid_t pid);
pid_t          task_to_pid(task_struct_t *task);

task_struct_t *allocate_task_struct(void);
void           free_task_struuct(task_struct_t *task);

void init_task_struct(
    task_struct_t *task,
    const char    *name,
    uint64_t       prio,
    uintptr_t      kstack_base,
    size_t         kstack_pages,
    size_t         ustack_pages
);

task_struct_t *task_start(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    void       *func,
    void       *arg
);

// process.c
task_struct_t *process_execute(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    size_t      ustack_pages,
    void       *proc
);

// schedule.c
uint64_t get_min_vrun_time(uint32_t cpu_id);
void     task_update(void);
void     task_list_insert(cpu_t *cpu, task_struct_t *task);
void     task_page_table_active(task_struct_t *task);
void     task_active(task_struct_t *task);
void     schedule(void);
void     task_block(task_status_t status);
void     task_unblock(pid_t pid);
void     task_yield(void);

#endif /* __ASSEMBLER__ */

#endif /* __TASK_H__ */