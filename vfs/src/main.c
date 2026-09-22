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

int main()
{
    int status = 0;

    // 准备消息.
    struct msg_head *msg = allocate_pages(NULL, 1);
    if (msg == NULL)
    {
        return -ENOMEM;
    }

    // 初始化消息结构体.
    msg->key           = 0;
    msg->type          = MSG_NORMAL;
    msg->header_legnth = sizeof(*msg);
    msg->legnth        = msg->header_legnth + sizeof(size_t);

    // 发送消息表示启动完成
    status = send(msg, 0);

    // 1. 获取ramfs大小
    status = recv(msg, 0);
    if (status < 0)
    {
        return status;
    }
    size_t ramfs_size  = *(size_t *)((uintptr_t)msg + msg->header_legnth);
    size_t ramfs_pages = (ramfs_size / 4096) + 1;

    uint8_t *ramfs_data = allocate_pages(NULL, ramfs_pages);
    if (ramfs_data == NULL)
    {
        return -ENOMEM;
    }

    // 读取文件
    // msg body第一个字节为标志位,其余为数据.
    msg->key           = 0;
    off_t ramfs_offset = 0;
    while (1)
    {
        msg->header_legnth = sizeof(*msg);
        msg->legnth        = 4096;
        msg->key++;

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
        uint8_t flag = *((uint8_t *)msg + msg->header_legnth);
        if (flag == 1)
        {
            break;
        }
        size_t copy_size = msg->legnth - msg->header_legnth - sizeof(uint8_t);
        void  *copy_src = (uint8_t *)msg + msg->header_legnth + sizeof(uint8_t);
        void  *copy_dst = (uint8_t *)ramfs_data + ramfs_offset;

        memcpy(copy_dst, copy_src, copy_size);
    }
    return 0;
}