// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_CAP_OBJECT_H__
#define __SYSCALL_CAP_OBJECT_H__

#define CAP_SLOTS    32
#define CAP_TABLE_NR 64
#define MAX_SLOTS    (CAP_SLOTS * CAP_TABLE_NR)

#include <sync/spinlock.h>
#include <syscall/cap.h>
#include <syscall/cap/struct.h>
#include <types.h>

struct cap_opt
{
    void (*destory)(struct cap_head *);
};

// 任何cap对象的头部必须是cap_head
struct cap_head
{
    enum cap_type   type;
    struct spinlock lock;
    int32_t         reference_count;

    // 操作函数
    struct cap_opt opt;
};

struct cap_slot_table
{
    struct cap_slot slots[CAP_SLOTS];
    uint16_t        count;
};

struct cap_node
{
    struct cap_head        head;
    uint16_t               key_seed;
    uint64_t               slots_map;
    struct cap_slot_table *table[CAP_TABLE_NR];
};

struct cap_ipc
{
    struct cap_head head;
    pid_t           port;
};

struct cap_mem
{
    struct cap_head head;
    phys_addr_t     phys;
    size_t          pages;
};

struct cap_task
{
    struct cap_head head;
    pid_t           pid;
};

#endif /* __SYSCALL_CAP_OBJECT_H__ */