/*
   Copyright 2024 LinChenjun

   本文件是Clay Figure Kernel的一部分。
   修改和/或再分发遵循GNU GPL version 3 (or any later version)
  
*/

#ifndef __TASK_H__
#define __TASK_H__

#include <lib/list.h>
#include <lib/alloc_table.h>
#include <device/spinlock.h>
#include <device/sse.h>
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
    task_context_t        *context;
    addr_t                 kstack_base;
    size_t                 kstack_size;

    addr_t                 ustack_base;
    size_t                 ustack_size;

    pid_t                  pid;
    pid_t                  ppid;

    char                   name[32];
    volatile task_status_t status;
    uint64_t               spinlock_count;
    uint64_t               priority;
    uint64_t               jiffies;
    uint64_t               ideal_runtime;
    uint64_t               vrun_time;

    list_node_t            general_tag;

    uint64_t               cpu_id;
    uint64_t              *page_dir;
    allocate_table_t       vaddr_table;

    message_t              msg;
    pid_t                  send_to;
    int8_t                 send_flag;
    pid_t                  recv_from;
    uint8_t                recv_flag;
    uint8_t                has_intr_msg;
    spinlock_t             send_lock;
    list_t                 sender_list;
    list_node_t            send_tag;

    fxsave_region_t       *fxsave_region;
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
 * 将pid转换为task结构体.
 * 其中0 <= pid <= MAX_TASK
 */
PUBLIC task_struct_t* pid2task(pid_t pid);

/**
 * 判断pid对应的任务是否存在.
 */
PUBLIC bool task_exist(pid_t pid);

/**
 * 获取当前正在运行的任务的结构体.
 */
PUBLIC task_struct_t* running_task();
PUBLIC task_struct_t* running_prog();
PUBLIC addr_t get_running_prog_kstack();

/**
 * 在任务表中分配一个任务.
 */
PUBLIC status_t task_alloc(pid_t *pid);

/**
 * 将任务添加到队列中.
 * 队列按vrun_time由小到大排序.
 */
PUBLIC void task_list_insert(list_t *list,task_struct_t *task);
PUBLIC list_node_t* get_next_task(list_t *list);

/**
 * 将pid对应的任务结构体标记为未使用.
 */
PUBLIC void task_free(pid_t pid);

/**
 * 初始化一个任务结构体.
 */
PUBLIC status_t init_task_struct(
    task_struct_t* task,
    const char* name,
    uint64_t priority,
    addr_t kstack_base,
    size_t kstack_size);

/**
 * 创建任务的上下文结构.
 */
PUBLIC void create_task_struct(task_struct_t *task,void *func,uint64_t arg);

/**
 * 启动一个任务.
 * 输入:
 *  name 任务名称.
 *  priority 优先级.
 *  kstack_size 任务内核态下的栈大小.
 *  func 在任务中运行的函数.
 *  arg 给任务的参数.
 * 输出:
 *  返回对应的任务结构体,失败则返回NULL.
 */
PUBLIC task_struct_t* task_start(
    const char* name,
    uint64_t priority,
    size_t kstack_size,
    void* func,
    uint64_t arg);

PUBLIC void task_init();

/// schedule.c

/**
 * 更新vrun_time,以确保vruntime不会过小.
 */
PUBLIC void update_vruntime(task_struct_t *task);

/**
 * 更新vrun_time,并判断是否需要调度.
 */
PUBLIC void schedule();

/**
 * 进行任务调度.
 * 如果进程持有自旋锁,则不会触发调度.
 */
PUBLIC void do_schedule();

/**
 * 阻塞当前任务,并将任务状态设为status.
 */
PUBLIC void task_block(task_status_t status);

/**
 * 将pid对应的进程解除阻塞.
 */
PUBLIC void task_unblock(pid_t pid);

PUBLIC void task_yield();

/// tss.c
PUBLIC void init_tss(uint8_t nr_cpu);
PUBLIC void update_tss_rsp0(task_struct_t *task);

/// prog.c
PUBLIC void prog_activate(task_struct_t *task);

/**
 * 启动一个任务,运行在用户态下.
 * 输入:
 *  name 任务名称.
 *  priority 优先级.
 *  kstack_size 任务内核态下的栈大小.
 *  func 在任务中运行的函数.
 *  arg 给任务的参数.
 * 输出:
 *  返回对应的任务结构体,失败则返回NULL.
 */
PUBLIC task_struct_t *prog_execute(
    const char *name,
    uint64_t priority,
    size_t kstack_size,
    void *prog);

#endif