// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_STRUCT_H__
#define __TASK_STRUCT_H__

#ifndef __ASSEMBLER__

#    include <asm/ptrace.h>
#    include <asm/sync/spinlock.h>

#    include <lib/linked_list.h>
#    include <sync/atomic.h>

typedef int32_t pid_t;

struct task_mgr
{
    struct system_info *system_info;
    struct spinlock     lock;
    struct task       **task_table;
    uint8_t            *pid_table;
    int                 max_tasks;
    struct cpu         *cpus;
    int                 max_cpus;
    void               *kernel_page_table_pos;
};

struct cpu
{
    struct task *curr_task;
    struct task *main_task;
    struct task *dead_task;

    struct task_mgr *task_mgr;

    struct spinlock lock;
    size_t          running_tasks;
    struct list     task_queue;
    struct list     blocked_queue;

    uint64_t min_vrun_time;
    uint64_t total_weight;
};

// 任务状态标志
enum task_status
{
    TASK_READY = 1, // 任务就绪,随时进入运行状态
    TASK_RUNNING,   // 任务正在运行
    TASK_BLOCKED,   // 任务阻塞
    TASK_SEND,
    TASK_RECEIVE,
    TASK_WAITING, // 等待子任务结束
    TASK_DIED     // 任务结束
};

#    define EVT_NR 8

struct message
{
    int32_t  source;
    uint32_t type;
    union
    {
        uint32_t m32[14];
        uint64_t m64[7];
    };
};

struct mailbox
{
    struct message   msg;
    uint8_t          evt_msg[EVT_NR];
    pid_t            send_to;
    pid_t            recv_from;
    struct list      send_list;
    struct list_node send_node;
    struct spinlock  send_lock;
};

struct task
{
    struct task_context *context;

    uintptr_t kstack_base;
    size_t    kstack_pages;

    uintptr_t ustack_base;
    size_t    ustack_pages;
    void     *ustack_sp;

    uint8_t cpu_id;

    pid_t pid;
    pid_t ppid;

    char name[32];

    volatile enum task_status status;
    struct atomic             block_count;
    uint64_t                  preempt_count;
    uint64_t                 *pg_dir;
    struct list_node          general_node;

    uint64_t prio;
    uint64_t run_time;
    uint64_t vrun_time;

    struct atomic   childs;
    int             return_status;
    struct list     exited_childs;
    struct spinlock exited_lock;

    struct mailbox mailbox;
};

#endif /* __ASSEMBLER__ */

#define CPU_CURRENT_TASK 0x00
#define CPU_MAIN_TASK    0x08
#define CPU_DEAD_TASK    0x10
#define CPU_TASK_MGR     0x18

#define TASK_STRUCT_KSTACK_BASE  0x08
#define TASK_STRUCT_KSTACK_PAGES 0x10
#define TASK_STRUCT_USTACK_SP    0x28

#endif /* __TASK_STRUCT_H__ */