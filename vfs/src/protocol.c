// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "protocol.h"

#include "file.h"
#include <errno.h>
#include <user/lib.h>
#include <user/services/vfs.h>

static void  *msg_buf     = NULL;
static size_t msg_buf_cap = 0;

static int protocol_ensure_buf(size_t size)
{
    if (size + 1 <= msg_buf_cap)
    {
        return 0;
    }
    void *buf = malloc(size + 1); // +1: 给路径这类 C 字符串留一个 '\0'
    if (buf == NULL)
    {
        return -ENOMEM;
    }
    free(msg_buf);
    msg_buf     = buf;
    msg_buf_cap = size + 1;
    return 0;
}

static int protocol_do_open(struct msg_head *msg)
{
    if (msg->data == NULL || msg->size == 0)
    {
        msg->args[0] = -EINVAL;
        return -EINVAL;
    }
    int32_t i_no = vfs_open((const char *)msg->data, msg->args[0]);

    msg->data    = NULL;
    msg->size    = 0;
    msg->args[0] = i_no;
    return (i_no < 0) ? i_no : 0;
}

static int protocol_do_read(struct msg_head *msg)
{
    int32_t i_no = msg->args[0];

    if (i_no < 0)
    {
        return -EINVAL;
    }
    /// TODO: 真正读取 data
    msg->data = NULL;
    msg->size = 0;
    return 0;
}

static int protocol_do_write(struct msg_head *msg)
{
    int32_t i_no = msg->args[0];

    if (i_no < 0)
    {
        return -EINVAL;
    }
    /// TODO:
    msg->data = msg_buf;
    return 0;
}

// 分发一条请求, 并把应答发回客户端
static void protocol_handle(struct msg_head *msg)
{
    pid_t client = msg->source;
    int   status;

    switch (msg->key)
    {
        case VFS_OPEN:
            status = protocol_do_open(msg);
            break;
        case VFS_READ:
            status = protocol_do_read(msg);
            break;
        case VFS_WRITE:
            status = protocol_do_write(msg);
            break;
        default:
            status = -EINVAL;
            break;
    }

    msg->key = status;
    (void)send(msg, client); // 客户端可能已退出, 忽略错误
    return;
}

int protocol_serve(void)
{
    struct msg_head msg;

    while (1)
    {
        int ret = 0;

        // 1. 先探包: 容量声明为 0, 只为拿到 key/args 和需要的字节数
        msg.data = NULL;
        msg.size = 0;
        ret      = recv(&msg, IPC_PEEK);

        if (ret == -E2BIG)
        {
            // 2. 按需扩容落地区, 再正式收一次
            if (protocol_ensure_buf(msg.size) < 0)
            {
                dprintf(
                    "vfs: no memory for %d bytes message.\n", (int)msg.size
                );
                continue; // 收不下就无法应答, 只能跳过
            }
            msg.data = msg_buf;
            msg.size = msg_buf_cap;
            ret      = recv(&msg, IPC_PEEK);
            if (ret < 0)
            {
                continue;
            }
            // 给路径这类 C 字符串负载补 '\0'
            ((uint8_t *)msg_buf)[msg.size] = '\0';
        }
        else if (ret < 0)
        {
            continue;
        }

        protocol_handle(&msg);
    }
    return 0;
}
