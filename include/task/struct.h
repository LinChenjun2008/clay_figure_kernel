// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_STRUCT_H__
#define __TASK_STRUCT_H__

#ifndef __ASSEMBLER__

#    include <asm/ptrace.h>

#    include <lib/bitmap.h>
#    include <lib/linked_list.h>
#    include <sync/atomic.h>
#    include <sync/spinlock.h>

#    define SLOTS_PER_LEVEL 256

// task_slots共三层,L1,L2中slots存储task_slots,L3中slots[]存储任务指针.
struct task_slots
{
    void *slots[SLOTS_PER_LEVEL];
    int   count;
};

struct task_table
{
    struct spinlock   lock;
    struct bitmap     pid_map;   // 分配pid的位图
    struct task_slots task_root; // task_slots L1层
    pid_t             last_pid;  // 上一个分配的pid.
};

struct task_mgr
{
    struct system_info *system_info;
    struct task_table   task_table;
    struct cpu         *cpus;
    int                 max_cpus;
    phys_addr_t         kernel_page_table_pos;
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
    pid_t    source;
    uint32_t type;
    union
    {
        uint32_t m32[14];
        uint64_t m64[7];
    };
};

struct mailbox
{
    struct message msg;             // 临时存储发送/接收的消息
    uint8_t        evt_msg[EVT_NR]; // evt_msg[i]表示时间i发生的次数
    pid_t          send_to;         // 向send_to对应的任务发送消息
    pid_t          recv_from;       // 从recv_from匹配的消息源接收消息
    uint8_t        closed;          // 邮箱关闭标记,置位后拒绝发送/接收消息
    uint8_t        recv_err;        // 接收消息出现错误的标记

    struct list      send_list; // 向当前邮箱发送消息的队列
    struct list_node send_node; // 发送消息时加入目标邮箱的send_list
    struct spinlock  send_lock; // 操作邮箱时获取的锁

    struct list      recv_list; // 从当前邮箱接收消息的队列
    struct list_node recv_node; // 接收消息时加入来源邮箱的send_list
    struct spinlock  recv_lock; // 操作邮箱时获取的锁
};

struct task
{
    struct task_context *context; // task + 0 上下文

    uintptr_t kstack_base;  // task + 8 内核栈基地址
    size_t    kstack_pages; // task + 16 内核栈页数

    size_t    ustack_pages;
    uintptr_t ustack_sp;

    uint8_t cpu_id;

    pid_t pid;
    pid_t ppid;

    char name[32];

    struct spinlock           lock;
    volatile enum task_status status;
    uint64_t                  block_count;
    uint64_t                  preempt_count;
    phys_addr_t               pg_dir;
    struct list_node          general_node;
    struct list_node          sema_node;

    uint64_t prio;
    uint64_t run_time;
    uint64_t vrun_time;

    struct mm_struct *mm;

    int return_status;

    struct spinlock  childs_lock;
    struct list      childs_list;
    struct list      exited_childs;
    struct list_node parent_node;

    struct mailbox mailbox;
};

#endif /* __ASSEMBLER__ */

#define CPU_CURRENT_TASK 0x00
#define CPU_MAIN_TASK    0x08
#define CPU_DEAD_TASK    0x10
#define CPU_TASK_MGR     0x18

#define TASK_STRUCT_KSTACK_BASE  0x08
#define TASK_STRUCT_KSTACK_PAGES 0x10
#define TASK_STRUCT_USTACK_SP    0x20

#endif /* __TASK_STRUCT_H__ */