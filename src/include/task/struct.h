// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_STRUCT_H__
#define __TASK_STRUCT_H__

#define TASK_STRUCT_KSTACK_BASE  8
#define TASK_STRUCT_KSTACK_PAGES 16

#ifndef __ASSEMBLER__

#    include <asm/ptrace.h>
#    include <asm/sync/spinlock.h>

#    include <lib/list.h>
#    include <mm_struct.h>

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

struct task
{
    struct task_context *context; // 任务上下文

    uintptr_t kstack_base;  // 内核栈基地值(虚拟地址)
    size_t    kstack_pages; // 内核栈所用的页数

    uintptr_t ustack_base;  // 用户栈基址(物理地址)(如果有)
    size_t    ustack_pages; // 用户栈所用的页数(如果有)

    struct cpu *volatile cpu; // 任务所在cpu的id

    pid_t pid;  // 任务id
    pid_t ppid; // 父级任务id

    int childs;

    char                   name[32];      // 任务名
    volatile task_status_t status;        // 任务状态
    uint64_t               preempt_count; // 抢占计数
    uint64_t              *page_dir;      // 任务页表地址(物理地址)
    struct list_node       general_tag;   // 任务在任务列表中的节点

    uint64_t prio;      // 任务优先级
    uint64_t run_time;  // 任务运行时间(总计)
    uint64_t vrun_time; // 虚拟运行时间

    struct mm mm_alloc; // 任务可分配的虚拟地址表
    struct mm mm_using; // 任务使用中的虚拟地址表
    struct mm mm_pages; // 任务正在使用的物理页表
};

STATIC_ASSERT(OFFSET(struct task, kstack_base) == TASK_STRUCT_KSTACK_BASE, "");

STATIC_ASSERT(
    OFFSET(struct task, kstack_pages) == TASK_STRUCT_KSTACK_PAGES,
    ""
);

#endif /* __ASSEMBLER__ */
#endif /* __TASK_STRUCT_H__ */