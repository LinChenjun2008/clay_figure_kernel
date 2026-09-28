// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __USER_SERVICES_VFS_H__
#define __USER_SERVICES_VFS_H__

#define VFS_OPEN   2
#define VFS_READ   3
#define VFS_WRITE  4
#define VFS_CLOSE  5
#define VFS_DELETE 6

#define VFS_MAX_NR 6

#include <types/message.h>

struct vfs_ramfs_size
{
    struct msg_head head;
    size_t          size;
};

struct vfs_ramfs_data
{
    struct msg_head head;
    uint8_t         flag;
    uint8_t         data[];
};

#endif /* __USER_SERVICES_VFS_H__ */