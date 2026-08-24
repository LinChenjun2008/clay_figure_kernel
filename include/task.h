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

#define PID_INDEX_SHIFT 8
#define PID_INDEX_MASK  0xffffff
#define PID_COUNT_SHIFT 0
#define PID_COUNT_MASK  0xff

#define PID_ANY   -1
#define PID_CHILD -2
#define PID_EVENT -3
#define PID_NULL  -4

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

void task_init(struct system_info *system_info, int max_tasks);
void make_main_task(uintptr_t stack_base, size_t stack_pages);

void         set_current_task(struct task *task);
struct task *get_current_task(void);

int          check_pid_avaiability(pid_t pid);
struct task *pid_to_task(pid_t pid);
struct task *allocate_task_struct(void);
void         destory_task_struct(struct task *task);

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

// exec.c
void *load_segment(void *file);

// waitpid
#    define WNOHANG 1

pid_t task_waitpid(pid_t pid, int *status, int options);

// process.c
struct task *process_execute(
    const char *file,
    uint64_t    prio,
    size_t      kstack_pages,
    size_t      ustack_pages,
    void       *arg
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
void task_block_wait(enum task_status status, pid_t wait_for);
void task_unblock(pid_t pid);
void task_yield(void);

#endif /* __ASSEMBLER__ */

#endif /* __TASK_H__ */