// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __TASK_H__
#define __TASK_H__

// PID的最小值
#define MIN_PID 0

// PID的最大值
#define MAX_PID (1 << 22)

// pid 回绕时重新搜索的起始位置
#define RESERVED_PIDS 300

#define TASK_SLOT_L1_MASK  0xff
#define TASK_SLOT_L1_SHIFT 16
#define TASK_SLOT_L2_MASK  0xff
#define TASK_SLOT_L2_SHIFT 8
#define TASK_SLOT_L3_MASK  0xff
#define TASK_SLOT_L3_SHIFT 0

#define PID_INDEX_SHIFT 0
#define PID_INDEX_MASK  0xffff
#define PID_COUNT_SHIFT 16
#define PID_COUNT_MASK  0xffff

#define PID_ANY   -1
#define PID_CHILD -2
#define PID_EVENT -3
#define PID_NULL  -4
#define PID_ERROR -5

#define MAX_NICE 20
#define MIN_NICE -19

#define NICE_TO_PRIO(NICE) ((NICE) + 20)
#define PRIO_TO_NICE(PRIO) ((PRIO) - 20)

#define DEFAULT_PRIO NICE_TO_PRIO(0)
#define SERVICE_PRIO NICE_TO_PRIO(-10)
#define IDLE_PRIO    NICE_TO_PRIO(20)

#define MAX_VRUNTIME(A, B) ((int64_t)((A) - (B)) > 0 ? (A) : (B))

void task_init(struct system_info *system_info);
void make_main_task(uintptr_t stack_base, size_t stack_pages);

void         set_current_task(struct task *task);
struct task *get_current_task(void);

struct task *pid_to_task(pid_t pid);
int          check_pid_avaiability(pid_t pid);
pid_t        allocate_pid(void);
void         release_pid(pid_t pid);
int          pid_table_insert(struct task *task);
void         pid_table_remove(struct task *task);
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

#endif /* __TASK_H__ */