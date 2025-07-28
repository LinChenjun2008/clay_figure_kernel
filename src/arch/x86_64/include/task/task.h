// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __TASK_H__
#define __TASK_H__

#ifndef __ASM_INCLUDE__

#    include <device/cpu.h> // NR_CPUS
#    include <device/spinlock.h>
#    include <device/sse.h>
#    include <kernel/syscall.h>
#    include <lib/alloc_table.h>
#    include <lib/list.h>
#    include <sync/atomic.h>

#endif /* __ASM_INCLUDE__ */

#define MAX_TASK 4096

#define MAX_NICE 20
#define MIN_NICE -19

#define NICE_TO_PRIO(NICE) ((NICE) + 20)
#define PRIO_TO_NICE(PRIO) ((PRIO) - 20)

#define DEFAULT_PRIORITY NICE_TO_PRIO(0)
#define SERVICE_PRIORITY NICE_TO_PRIO(-10)

#define TASK_STRUCT_KSTACK_BASE 8
#define TASK_STRUCT_KSTACK_SIZE 16

// 目标调度延迟(ns) = 24ms
// 所有可运行进程在此时段内至少运行一次
#define SCHED_LATENCY_NS 24000000

// 最小调度粒度(ns) = 6ms
// 进程被调度后的最小运行时间
#define SCHED_MIN_GRANULARITY_NS 6000000

#define MAX_VRUNTIME(A, B) ((int64_t)((A) - (B)) > 0 ? (A) : (B))

#ifndef __ASM_INCLUDE__

typedef enum task_status_e
{
    TASK_NO_TASK = 0,
    TASK_USING,
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SENDING,
    TASK_RECEIVING,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
} task_status_t;

typedef struct task_context_s
{
    wordsize_t r15;
    wordsize_t r14;
    wordsize_t r13;
    wordsize_t r12;

    wordsize_t rbp;
    wordsize_t rbx;
    wordsize_t rsi;
    wordsize_t rdi;
} task_context_t;

typedef struct task_struct_s
{
    task_context_t *context;     // 任务上下文
    addr_t          kstack_base; // 内核栈基地值
    size_t          kstack_size; // 内核栈大小(字节)

    addr_t ustack_base; // 用户栈基址(如果有)
    size_t ustack_size; // 用户栈大小(如果有)

    pid_t pid;  // 任务id
    pid_t ppid; // 父级任务id

    char                   name[32];       // 任务名
    volatile task_status_t status;         // 任务状态
    uint64_t               spinlock_count; // 任务持有的自旋锁
    uint64_t               priority;       // 任务优先级
    uint64_t               jiffies;        // 任务运行时间(总计)
    uint64_t               runtime;        // 任务被调度后的运行时间
    uint64_t               ideal_runtime;  // 任务本次调度可运行的时间
    uint64_t               vrun_time;      // 虚拟运行时间

    list_node_t general_tag;

    uint64_t         cpu_id;
    uint64_t        *page_dir;
    allocate_table_t vaddr_table;

    message_t msg;
    pid_t     send_to;
    pid_t     recv_from;

    atomic_t send_flag;
    atomic_t recv_flag;

    uint8_t     has_intr_msg;
    spinlock_t  send_lock;
    list_t      sender_list;
    list_node_t send_tag;

    fxsave_region_t *fxsave_region;
} task_struct_t;

STATIC_ASSERT(
    OFFSET(task_struct_t, kstack_base) == TASK_STRUCT_KSTACK_BASE,
    ""
);
STATIC_ASSERT(
    OFFSET(task_struct_t, kstack_size) == TASK_STRUCT_KSTACK_SIZE,
    ""
);

/**
 * @brief the task management struct for each cpu
 */
typedef struct task_man_s
{
    list_t         task_list;     // 进程队列
    uint64_t       min_vruntime;  // 最小虚拟运行时间
    uint64_t       running_tasks; // task_list中的任务数量
    uint64_t       total_weight;  // task_list中的任务总权重
    task_struct_t *idle_task;
    spinlock_t     task_list_lock;
} task_man_t;

/**
 * @brief the task management struct
 */
typedef struct global_task_man_s
{
    task_struct_t tasks[MAX_TASK];
    spinlock_t    tasks_lock;
    task_man_t    cpus[NR_CPUS];
} global_task_man_t;

/**
 * @brief 获取global_task_man指针
 * @return global_task_man
 */
PUBLIC global_task_man_t *get_global_task_man(void);

/**
 * @brief 获取对应cpu的cpu_task_man_t指针
 * @param cpu_id cpu id
 * @return &global_task_man->cpu[cpu_id]
 */
PUBLIC task_man_t *get_task_man(uint32_t cpu_id);

/**
 * @brief 将pid转换为task结构体
 * @param pid pid
 * @note 0 <= pid <= MAX_TASK
 */
PUBLIC task_struct_t *pid_to_task(pid_t pid);

/**
 * @brief 判断pid对应的任务是否存在
 * @param pid pid
 */
PUBLIC bool task_exist(pid_t pid);

/**
 * @brief 获取当前正在运行的任务的结构体
 * @return 当前正在运行的任务的结构体
 */
PUBLIC task_struct_t *running_task(void);

/**
 * @brief 在任务表中分配一个任务
 * @param pid 如果成功,pid指针处将写入分配的pid
 * @return 成功将返回K_SUCCESS,失败则返回错误码
 */
PUBLIC status_t task_alloc(pid_t *pid);

/**
 * @brief 将pid对应的任务结构体标记为未使用
 * @param pid pid
 */
PUBLIC void task_free(pid_t pid);

/**
 * @brief 初始化一个任务结构体
 * @param task 任务结构体指针
 * @param name 任务名称
 * @param priority 优先级
 * @param kstack_base 任务内核态下的栈基址
 * @param kstack_size 任务内核态下的栈大小
 */
PUBLIC status_t init_task_struct(
    task_struct_t *task,
    const char    *name,
    uint64_t       priority,
    addr_t         kstack_base,
    size_t         kstack_size
);

/**
 * @brief 创建任务的上下文结构
 * @param task 任务结构体指针
 * @param func 在任务中运行的函数
 * @param arg 给任务的参数
 */
PUBLIC void create_task_struct(task_struct_t *task, void *func, uint64_t arg);

/**
 * @brief 启动一个任务
 * @param name 任务名称
 * @param priority 优先级
 * @param kstack_size 任务内核态下的栈大小
 * @param func 在任务中运行的函数
 * @param arg 给任务的参数
 * @return 成功将返回对应的任务结构体,失败则返回NULL
 */
PUBLIC task_struct_t *task_start(
    const char *name,
    uint64_t    priority,
    size_t      kstack_size,
    void       *func,
    uint64_t    arg
);

PUBLIC void task_init(void);

/// schedule.c

/**
 * @brief 获取cpu的最小vrun_time
 * @param cpu_id cpu id
 */
PUBLIC uint64_t get_min_vruntime(uint32_t cpu_id);

/**
 * @brief 更新当前任务的runtime,runtime以及min_runtime,并计算ideal_runtime
 */
PUBLIC void task_update(void);

/**
 * @brief 进行任务调度
 * @note 如果进程持有自旋锁,则不会触发调度
 */
PUBLIC void schedule(void);

/**
 * @brief 将任务添加到列表中
 * @param task_man 任务管理结构
 * @param task 要添加的任务结构体指针
 * @note 使用此函数前请确保获取了taskmgr的task_list_lock锁
 */
PUBLIC void task_list_insert(task_man_t *task_man, task_struct_t *task);

/**
 * @brief 在列表中获取下一个可以运行的任务
 * @param task_man 任务管理结构
 * @return 成功将返回下一个任务结构,失败则返回NULL
 */
PUBLIC task_struct_t *get_next_task(task_man_t *task_man);

/**
 * @brief 阻塞当前任务,并将任务状态设为status
 * @param status 阻塞后的任务状态
 */
PUBLIC void task_block(task_status_t status);

/**
 * @brief 将pid对应的进程解除阻塞
 * @param pid pid
 */
PUBLIC void task_unblock(pid_t pid);

/**
 * @brief 将pid对应的进程解除阻塞,但不会操作task_list_lock锁
 * @param pid pid
 */
PUBLIC void task_unblock_sub(pid_t pid);

/**
 * @brief 使当前进程让出cpu
 */
PUBLIC void task_yield(void);

/**
 * @brief 使当前进程休眠一定时间
 * @param milliseconds 要休眠的毫秒数
 */
PUBLIC void task_msleep(uint32_t milliseconds);

/// tss.c
PUBLIC void init_tss(uint8_t cpu_id);
PUBLIC void update_tss_rsp0(task_struct_t *task);

/// prog.c

/**
 * @brief 激活任务的页表,更新tss的rsp0,设置gs_base
 * @param task
 * @return
 */
PUBLIC void prog_activate(task_struct_t *task);

/**
 * @brief 启动一个任务,运行在用户态下
 * @param name 任务名称
 * @param priority 优先级
 * @param kstack_size 任务内核态下的栈大小
 * @param func 在任务中运行的函数
 * @param arg 给任务的参数
 * @return 成功将返回对应的任务结构体,失败则返回NULL
 */
PUBLIC task_struct_t *prog_execute(
    const char *name,
    uint64_t    priority,
    size_t      kstack_size,
    void       *prog
);

#endif /* __ASM_INCLUDE__ */

#endif