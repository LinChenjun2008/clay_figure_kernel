// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_CAP_STRUCT_H__
#define __SYSCALL_CAP_STRUCT_H__

#include <sync/spinlock.h>

#define CAP_SLOTS 32

#define CAP_READ  (1 << 0)
#define CAP_WRITE (1 << 1)
#define CAP_EXEC  (1 << 2)
#define CAP_CTRL  (1 << 3)

enum cap_type
{
    CAP_NONE = 0,
    CAP_NODE,
    CAP_IPC,
    CAP_MEM,
    CAP_TASK,
    CAP_NR // 有效对象类型数量
};

struct cap_head
{
    enum cap_type   type;
    struct spinlock lock;
    uint32_t        reference_count;
};

struct cap_slot_entry
{
    struct cap_head *head;
    uint32_t         key;    // 访问cap的key
    uint32_t         rights; // 权限

    struct cap_node *owner;   // 所属 cnode(反向指针, 跨 cnode 撤销定位)
    uint32_t         slot_id; // 所在槽位(id >= 1)

    struct cap_slot_entry *parent;
    struct cap_slot_entry *child;
    struct cap_slot_entry *next;
};

struct cap_slot
{
    struct cap_slot_entry *entry;
    uint32_t               key_seed;
};

struct cap_node
{
    struct cap_head head;
    struct cap_slot slots[CAP_SLOTS];
};

#endif /* __SYSCALL_CAP_STRUCT_H__ */