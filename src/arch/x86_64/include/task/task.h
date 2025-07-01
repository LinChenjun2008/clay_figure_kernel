/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#ifndef __TASK_H__
#define __TASK_H__

#include <kernel/syscall.h>

#include <device/spinlock.h>
#include <device/sse.h>
#include <lib/alloc_table.h>
#include <lib/list.h>
#include <sync/atomic.h>

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
    task_context_t *context;
    addr_t          kstack_base;
    size_t          kstack_size;

    addr_t ustack_base;
    size_t ustack_size;

    pid_t pid;
    pid_t ppid;

    char                   name[32];
    volatile task_status_t status;
    uint64_t               spinlock_count;
    uint64_t               priority;
    uint64_t               jiffies;
    uint64_t               ideal_runtime;
    uint64_t               vrun_time;

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

typedef struct core_taskmgr_s
{
    list_t     task_list;
    uint64_t   min_vruntime;
    pid_t      idle_task;
    spinlock_t task_list_lock;
} core_taskmgr_t;

typedef struct taskmgr_s
{
    task_struct_t  task_table[MAX_TASK];
    spinlock_t     task_table_lock;
    core_taskmgr_t core[NR_CPUS];
} taskmgr_t;

/**
 * @brief 将pid转换为task结构体
 * @param pid pid
 * @note 0 <= pid <= MAX_TASK
 */
PUBLIC task_struct_t *pid2task(pid_t pid);

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
 * @brief 将任务添加到列表中
 * @param list 任务列表
 * @param task 要添加的任务结构体指针
 */
PUBLIC void task_list_insert(list_t *list, task_struct_t *task);

/**
 * @brief 在列表中获取下一个可以运行的任务
 * @param list 任务列表
 * @return 成功将返回下一个任务结构,失败则返回NULL
 */
PUBLIC task_struct_t *get_next_task(list_t *list);

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
PUBLIC uint64_t get_core_min_vruntime(uint32_t cpu_id);
/**
 * @brief 更新vrun_time,并判断是否需要调度
 */
PUBLIC void     schedule(void);

/**
 * @brief 进行任务调度
 * @note 如果进程持有自旋锁,则不会触发调度
 */
PUBLIC void do_schedule(void);

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
 * @brief 使当前进程让出cpu
 */
PUBLIC void task_yield(void);

/**
 * @brief 使当前进程休眠一定时间
 * @param milliseconds 要休眠的毫秒数
 */
PUBLIC void task_msleep(uint32_t milliseconds);

/// tss.c
PUBLIC void   init_tss(uint8_t nr_cpu);
PUBLIC void   update_tss_rsp0(task_struct_t *task);
PUBLIC addr_t get_running_prog_kstack(void);

/// prog.c
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

#endif