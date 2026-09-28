// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <user/lib.h>

//
#include <errno.h>
#include <user/bitmap.h>
#include <user/services/vfs.h>
#include <user/string.h>

void *ramfs_base;

static int read_ramfs(struct msg_head *msg)
{
    int status = 0;

    // 初始化消息结构体.
    msg->type          = MSG_NORMAL;
    msg->header_legnth = sizeof(*msg);
    msg->legnth        = msg->header_legnth + sizeof(size_t);

    // 1. 获取ramfs大小
    status = recv(msg, 0);
    if (status < 0)
    {
        return status;
    }
    struct vfs_ramfs_size *ramfs_size = (void *)msg;

    size_t ramfs_pages = (ramfs_size->size + 4065) / 4096;

    ramfs_base = allocate_pages(NULL, ramfs_pages);
    if (ramfs_base == NULL)
    {
        return -ENOMEM;
    }

    // 读取文件
    off_t ramfs_offset = 0;
    while (1)
    {
        struct vfs_ramfs_data *ramfs_data = (void *)msg;
        ramfs_data->head.header_legnth    = sizeof(ramfs_data->head);
        ramfs_data->head.legnth           = 4096;

        status = recv(msg, 0);
        if (status < 0)
        {
            return -ENOENT;
        }
        // vfs初始化阶段非内核数据直接丢弃.
        if (msg->source != 0)
        {
            continue;
        }
        size_t copy_size = 4096 - sizeof(*ramfs_data);
        void  *copy_src  = (void *)&ramfs_data->data;
        void  *copy_dst  = (uint8_t *)ramfs_base + ramfs_offset;

        memcpy(copy_dst, copy_src, copy_size);
        ramfs_offset += copy_size;
        // 最后一个数据块
        if (ramfs_data->flag == 1)
        {
            break;
        }
    }
    return 0;
}

int main()
{
    int status = 0;

    // 准备消息.
    struct msg_head *msg = allocate_pages(NULL, 1);
    if (msg == NULL)
    {
        return -ENOMEM;
    }

    status = read_ramfs(msg);
    if (status < 0)
    {
        return status;
    }

    while (1)
    {
        status = recv(msg, 0);
        if (status < 0)
        {
            continue;
        }
        switch (msg->key)
        {
            case VFS_OPEN:
                break;
            default:
                break;
        }
    }
    return 0;
}