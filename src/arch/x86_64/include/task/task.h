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
#    include <lib/list.h>
#    include <mem/vmm.h>
#    include <sync/atomic.h>

#endif /* __ASM_INCLUDE__ */

// 最大支持的任务数
#define TASKS 4096

// PID的最大值
#define MAX_PID (TASKS - 1)

// 表示"任务不存在"时使用的pid
#define PID_NO_TASK TASKS

#define MAX_NICE 20
#define MIN_NICE -19

#define NICE_TO_PRIO(NICE) ((NICE) + 20)
#define PRIO_TO_NICE(PRIO) ((PRIO) - 20)

#define DEFAULT_PRIORITY NICE_TO_PRIO(0)
#define SERVICE_PRIORITY NICE_TO_PRIO(-10)

#define TASK_STRUCT_KSTACK_BASE 8
#define TASK_STRUCT_KSTACK_SIZE 16

#define MAX_VRUNTIME(A, B) ((int64_t)((A) - (B)) > 0 ? (A) : (B))

#ifndef __ASM_INCLUDE__

// 任务状态标志
typedef enum task_status_e
{
    TASK_READY = 1, // 任务就绪,随时进入运行状态
    TASK_RUNNING,   // 任务正在运行
    TASK_BLOCKED,   // 任务阻塞
    TASK_SENDING,   // 任务正在发送消息
    TASK_RECEIVING, // 任务正在接收消息
    TASK_DIED       // 任务结束
} task_status_t;

// 任务上下文结构
typedef struct task_context_s
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;

    uint64_t rbp;
    uint64_t rbx;
    uint64_t rsi;
    uint64_t rdi;
} task_context_t;

typedef struct task_struct_s
{
    task_context_t *context; // 任务上下文

    uintptr_t kstack_base; // 内核栈基地值
    size_t    kstack_size; // 内核栈大小(字节)

    uintptr_t ustack_base; // 用户栈基址(如果有)
    size_t    ustack_size; // 用户栈大小(如果有)

    pid_t pid;  // 任务id
    pid_t ppid; // 父级任务id

    char                   name[32];      // 任务名
    volatile task_status_t status;        // 任务状态
    uint64_t               preempt_count; // 抢占计数
    uint64_t               priority;      // 任务优先级
    uint64_t               run_time;      // 任务运行时间(总计)
    uint64_t               vrun_time;     // 虚拟运行时间

    list_node_t general_tag; // 任务在任务列表中的节点

    uint64_t     cpu_id;    // 任务所在cpu的id
    uint64_t    *page_dir;  // 任务页表地址(物理地址)
    vmm_struct_t vmm_free;  // 任务可以使用的虚拟地址表
    vmm_struct_t vmm_using; // 任务正在使用的虚拟地址表

    message_t msg;       // 任务消息结构体
    pid_t     send_to;   // 任务发送消息的目的地
    pid_t     recv_from; // 任务接收消息的来源

    atomic_t send_flag; // 任务发送消息的状态标志
    atomic_t recv_flag; // 任务接收消息的状态标志

    uint8_t     has_intr_msg; // 任务如果有来自中断的消息,由此变量记录
    spinlock_t  send_lock;    // 要操作任务的sender_list时需要获取此锁
    list_t      sender_list;  // 向任务发送消息的所有任务列表
    list_node_t send_tag; // 向其他任务发送消息时,用于加入目标任务的sender_list

    fxsave_region_t *fxsave_region; // 用于fxsave/fxrstor指令
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
    list_t     task_list; // 任务队列
    spinlock_t task_list_lock;

    // 正在进行IPC的任务队列
    // 只能操作各个cpu自己的send_recv_list,因此不用上锁
    list_t send_recv_list;

    uint64_t       min_vrun_time; // 最小虚拟运行时间
    uint64_t       running_tasks; // task_list中的任务数量
    uint64_t       total_weight;  // task_list中的任务总权重
    task_struct_t *idle_task;
} task_man_t;

/**
 * @brief the task management struct
 */
typedef struct global_task_man_s
{
    task_struct_t tasks[TASKS];
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
 * @note 0 <= pid <= MAX_PID
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
 * @return 成功将返回任务结构体指针,失败返回NULL
 */
PUBLIC task_struct_t *task_alloc(void);

/**
 * @brief 将pid对应的任务结构体标记为未使用
 * @param task task结构体
 */
PUBLIC void task_free(task_struct_t *task);

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
    uintptr_t      kstack_base,
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

/**
 * @brief 结束任务
 * @param ret_val 任务返回值
 * @return
 */
PUBLIC void task_exit(int ret_val);

/**
 * @brief 回收任务所有的资源
 * @param pid
 * @return
 */
PUBLIC void task_release_resource(pid_t pid);

/**
 * @brief 创建idle任务
 * @param
 * @return
 */
PUBLIC void create_idle_task(void);

PUBLIC void task_init(void);

/// schedule.c

/**
 * @brief 获取cpu的最小vrun_time
 * @param cpu_id cpu id
 */
PUBLIC uint64_t get_min_vrun_time(uint32_t cpu_id);

/**
 * @brief 更新当前任务的run_time,run_time以及min_run_time,并计算ideal_run_time
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

/// proc.c

/**
 * @brief 激活任务的页表
 * @param task
 * @return
 */
PUBLIC void page_table_activate(task_struct_t *task);

/**
 * @brief 激活任务的页表,更新tss的rsp0,设置gs_base
 * @param task
 * @return
 */
PUBLIC void proc_activate(task_struct_t *task);

/**
 * @brief 启动一个任务,运行在用户态下
 * @param name 任务名称
 * @param priority 优先级
 * @param kstack_size 任务内核态下的栈大小
 * @param func 在任务中运行的函数
 * @param arg 给任务的参数
 * @return 成功将返回对应的任务结构体,失败则返回NULL
 */
PUBLIC task_struct_t *proc_execute(
    const char *name,
    uint64_t    priority,
    size_t      kstack_size,
    void       *proc
);

/**
 * @brief 结束用户进程,回收proc_execute阶段分配的资源
 * @param ret_val 返回值
 * @return
 */
PUBLIC void proc_exit(int ret_val);

#endif /* __ASM_INCLUDE__ */

#endif