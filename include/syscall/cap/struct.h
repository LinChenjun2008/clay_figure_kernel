// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_CAP_STRUCT_H__
#define __SYSCALL_CAP_STRUCT_H__

#include <sync/spinlock.h>

#define CAP_READ  (1 << 0)
#define CAP_WRITE (1 << 1)
#define CAP_EXEC  (1 << 2)
#define CAP_CTRL  (1 << 3)

struct cap_head;

struct cap_slot
{
    struct cap_head *head;
    uint16_t         key_seed; // key种子,用于避免handle泄漏
    uint16_t         key;      // 访问cap的key
    uint32_t         rights;   // 操作head所属cap的权限
};

#endif /* __SYSCALL_CAP_STRUCT_H__ */