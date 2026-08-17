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

#    include <asm/sync/spinlock.h>

#    include <lib/linked_list.h>
#    include <task/struct.h>

struct task_mgr
{
    struct spinlock lock;
    struct task   **task_table;
    int             max_tasks;
    struct cpu     *cpus;
    int             max_cpus;
    void           *kernel_page_table_pos;
};

struct cpu
{
    struct task *curr_task;
    struct task *main_task;
    struct task *dead_task;

    struct spinlock lock;
    size_t          running_tasks;
    struct list     task_queue;
    struct list     blocked_queue;

    uint64_t min_vrun_time;
    uint64_t total_weight;
};

void             set_task_mgr(struct task_mgr *task_mgr);
struct task_mgr *get_task_mgr(void);

void task_init(struct boot_info *boot_info, int max_tasks);
void make_main_task(uintptr_t stack_base, size_t stack_pages);

uint8_t      get_current_cpu_id(void);
struct cpu  *get_cpu_struct(uint8_t cpu_id);
void         set_cpu_struct(void);
void         set_current_task(struct task *task);
struct task *get_current_task(void);

struct task *pid_to_task(pid_t pid);
struct task *allocate_task(void);
void         free_task(struct task *task);

void init_task_struct(
    struct task *task,
    const char  *name,
    uint64_t     prio,
    uintptr_t    kstack_base,
    size_t       kstack_pages,
    size_t       ustack_pages
);
struct task *task_start(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    void       *func,
    void       *arg
);
void task_exit(int return_value);
int  task_release_resources(struct task *task);

// waitpid
#    define WNOHANG 1

pid_t task_waitpid(pid_t pid, int *status, int options);

// process.c
struct task *process_execute(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    size_t      ustack_pages,
    void       *func
);
void process_exit(int status);

// schedule.c

void cpu_task_list_insert(struct cpu *cpu, struct task *task);
void cpu_task_enqueue(struct task *task);

uint64_t get_min_vrun_time(struct cpu *cpu);
void     task_update(void);
void     task_pg_active(struct task *task);
void     task_active(struct task *task);
void     schedule(void);

void task_block(enum task_status status);
void task_unblock(pid_t pid);
void task_yield(void);

#endif /* __ASSEMBLER__ */

#endif /* __TASK_H__ */