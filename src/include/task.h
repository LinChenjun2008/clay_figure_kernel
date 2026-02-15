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

#define TASK_STRUCT_KSTACK_BASE  8
#define TASK_STRUCT_KSTACK_PAGES 16

#define MAX_VRUNTIME(A, B) ((int64_t)((A) - (B)) > 0 ? (A) : (B))

#define USER_STACK_VADDR_TOP 0x0000800000000000
#define USER_VADDR_START     0x800000

#include <asm/arch_task.h>
#include <asm/sync/spinlock.h>
#include <lib/list.h>
#include <mm_struct.h>

typedef int32_t pid_t;

// 任务状态标志
typedef enum
{
    TASK_READY = 1, // 任务就绪,随时进入运行状态
    TASK_RUNNING,   // 任务正在运行
    TASK_BLOCKED,   // 任务阻塞
    TASK_WAITING,   // 等待子任务结束
    TASK_DIED       // 任务结束
} task_status_t;

typedef struct
{
    task_context_t *context; // 任务上下文

    uintptr_t kstack_base;  // 内核栈基地值(虚拟地址)
    size_t    kstack_pages; // 内核栈所用的页数

    uintptr_t ustack_base;  // 用户栈基址(物理地址)(如果有)
    size_t    ustack_pages; // 用户栈所用的页数(如果有)

    pid_t pid;  // 任务id
    pid_t ppid; // 父级任务id

    int childs;

    char                   name[32];      // 任务名
    volatile task_status_t status;        // 任务状态
    uint64_t               preempt_count; // 抢占计数
    uint64_t               cpu_id;        // 任务所在cpu的id
    uint64_t              *page_dir;      // 任务页表地址(物理地址)
    list_node_t            general_tag;   // 任务在任务列表中的节点

    uint64_t prio;      // 任务优先级
    uint64_t run_time;  // 任务运行时间(总计)
    uint64_t vrun_time; // 虚拟运行时间

    mm_struct_t mm_alloc; // 任务可分配的虚拟地址表
    mm_struct_t mm_using; // 任务使用中的虚拟地址表
    mm_struct_t mm_pages; // 任务正在使用的物理页表
} task_struct_t;

typedef struct
{
    spinlock_t lock;
    list_t     task_list;

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

STATIC_ASSERT(
    OFFSET(task_struct_t, kstack_base) == TASK_STRUCT_KSTACK_BASE,
    ""
);

STATIC_ASSERT(
    OFFSET(task_struct_t, kstack_pages) == TASK_STRUCT_KSTACK_PAGES,
    ""
);

// arch/task.c

void           set_current_task(task_struct_t *task);
task_struct_t *get_current_task(void);
uint8_t        get_current_cpu_id(void);
void           arch_switch_to(task_context_t **curr, task_context_t **next);
void           arch_task_active(task_struct_t *task);

// task.c
void task_init(boot_info_t *boot_info, int max_tasks);
void make_main_task(uintptr_t stack_base, size_t stack_pages);

task_man_t *get_task_man(void);
cpu_t      *get_cpu_struct(uint8_t id);

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

void create_task_context(task_struct_t *task, void *func, void *arg);

task_struct_t *task_start(
    const char *name,
    uint64_t    prio,
    size_t      kstack_pages,
    void       *func,
    void       *arg
);

// arch/process.c
void switch_to_user(void *func, uint64_t kstack);

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

#endif /* __TASK_H__ */