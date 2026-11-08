// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <asm/page.h> // PG_SIZE

#include "file.h"
#include "inode.h"
#include "path.h"
#include <errno.h>
#include <lib/bitmap.h>
#include <lib/string.h>
#include <user/lib.h>
#include <user/services/vfs.h>

struct vfs *fs = NULL;

void *ramfs_base = NULL;

static int read_ramfs(void)
{
    int status = 0;

    // 1. 获取ramfs大小
    struct msg_head head;
    size_t          total_size = 0;

    while (1)
    {
        head.data = &total_size;
        head.size = sizeof(total_size);

        status = recv(&head, 0);
        if (status < 0)
        {
            return status;
        }
        // vfs初始化阶段非内核数据直接丢弃.
        if (head.source != 0)
        {
            continue;
        }
        break;
    }

    // 2. 读取数据
    size_t ramfs_pages = DIV_ROUND_UP(total_size, PG_SIZE);
    ramfs_base         = allocate_pages(NULL, ramfs_pages);
    if (ramfs_base == NULL)
    {
        return -ENOMEM;
    }

    size_t offset = 0;
    while (offset < total_size)
    {
        size_t chunk = total_size - offset;
        if (chunk > PG_SIZE)
        {
            chunk = PG_SIZE;
        }
        head.data = (uint8_t *)ramfs_base + offset;
        head.size = chunk;

        status = recv(&head, 0);
        if (status < 0)
        {
            return -ENOENT;
        }
        // vfs初始化阶段非内核数据直接丢弃.
        if (head.source != 0)
        {
            continue;
        }
        offset += head.size; // 实际收到的数据大小
    }
    return 0;
}

int main()
{
    int status = 0;
    status     = init_vfs();
    if (status < 0)
    {
        dprintf("Failed to create vfs.\n");
        return status;
    }
    status = read_ramfs();
    if (status < 0)
    {
        return status;
    }
    struct msg_head msg;

    msg.data = NULL;
    msg.size = 0;

    while (1)
    {
        status = recv(&msg, IPC_PEEK);
        if (status < 0)
        {
            continue;
        }
        switch (msg.key)
        {
            case VFS_OPEN:
                break;
            default:
                break;
        }
    }
    return 0;
}