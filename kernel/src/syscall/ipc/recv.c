// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <errno.h>
#include <std/string.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/schedule.h>
#include <task/struct.h>

static void *copy_to_user(void *dst, const void *src, size_t size)
{
    if ((uintptr_t)dst >= KERNEL_VMA_BASE)
    {
        return NULL;
    }
    return memcpy(dst, src, size);
}

// // 接收来自event的消息
// static int ipc_event_lock(struct mailbox *dst, pid_t from)
// {
//     if (from != PID_ANY && from != PID_EVENT)
//     {
//         return 0;
//     }

//     int has_event = 0;
//     int i;
//     for (i = 0; i < EVT_NR; i++)
//     {
//         if (dst->evt_msg[i] == 0)
//         {
//             continue;
//         }
//         has_event = 1;
//         break;
//     }
//     if (!has_event)
//     {
//         return 0;
//     }

//     struct message *evt = &dst->msg;
//     memset(evt, 0, sizeof(*evt));
//     evt->source = PID_EVENT;
//     for (i = 0; i < EVT_NR; i++)
//     {
//         if (dst->evt_msg[i] == 0)
//         {
//             continue;
//         }
//         evt->type |= (1U << i);
//         evt->m32[i]     = dst->evt_msg[i];
//         dst->evt_msg[i] = 0;
//     }
//     return 1;
// }

// 获取一条来自其他任务的消息
static struct task *
ipc_recv_lock(struct mailbox *dst, pid_t dst_pid, pid_t from)
{
    struct list_node *node = NULL;

    struct check_send_list_pack pack;
    pack.dst_pid = dst_pid;
    pack.from    = from;

    node = list_traversal_remove(&dst->send_list, check_send_list, &pack);

    if (node == NULL)
    {
        dst->recv_from = from;
        return NULL;
    }

    struct mailbox *src = send_node_to_mailbox(node);
    src->send_to        = PID_NULL;
    dst->recv_from      = PID_NULL;
    memcpy(&dst->msg, &src->msg, sizeof(dst->msg));
    return mailbox_to_task(src);
}

// 尝试取一条匹配消息/事件并投递; 返回 0 成功, -ENOENT 表示暂无可用消息/事件
static int ipc_recv_try(struct task *dest_task, pid_t from, struct message *msg)
{
    struct mailbox *dst = &dest_task->mailbox;
    struct task    *src_task;
    // int             has_event;

    spin_lock(&dst->send_lock);
    // has_event = ipc_event_lock(dst, from);
    // if (!has_event)
    // {
    src_task = ipc_recv_lock(dst, dest_task->pid, from);
    // }
    spin_unlock(&dst->send_lock);

    // if (has_event)
    // {
    //     copy_to_user(msg, &dst->msg, sizeof(*msg));
    //     return 0;
    // }
    if (src_task == NULL)
    {
        return -ENOENT;
    }

    copy_to_user(msg, &dst->msg, sizeof(*msg));
    wake_up(&src_task->mailbox.send_wq);
    return 0;
}

// 把本任务从发送者的 recv_list 摘除(若仍在)
static void ipc_recv_leave(struct mailbox *dst, struct mailbox *src)
{
    spin_lock(&src->recv_lock);
    if (list_find(&src->recv_list, &dst->recv_node))
    {
        list_remove(&dst->recv_node);
    }
    spin_unlock(&src->recv_lock);
    return;
}

int ipc_recv(pid_t src_pid, struct message *msg)
{
    struct task    *dest_task = get_current_task();
    struct mailbox *dst       = &dest_task->mailbox;
    struct task    *src_task  = NULL;
    struct mailbox *src       = NULL;

    // 有效pid: 从特定任务接收消息
    if (check_pid_avaiability(src_pid))
    {
        int closed;

        src_task = pid_to_task(src_pid);
        if (src_task == NULL)
        {
            return -ESRCH;
        }
        src = &src_task->mailbox;

        spin_lock(&src->recv_lock);
        closed = src->closed;
        // 防止 recv_node 重复入队(上一次 recv 异常返回可能留下残留节点)
        if (!closed && !list_find(&src->recv_list, &dst->recv_node))
        {
            list_append(&src->recv_list, &dst->recv_node);
        }
        spin_unlock(&src->recv_lock);

        if (closed)
        {
            return -ESRCH;
        }
    }
    dst->recv_from = src_pid;

    int ret = -ENOENT;

    // 在block前已经接收到消息
    if (ipc_recv_try(dest_task, src_pid, msg) == 0)
    {
        ret = 0;
    }
    else
    {
        struct ipc_recv_pack pack;
        pack.task = dest_task;
        pack.from = src_pid;

        while (1)
        {
            int wake = wait_event(&dst->recv_wq, &pack, TASK_RECEIVE);

            // 这次已收到
            if (ipc_recv_try(dest_task, src_pid, msg) == 0)
            {
                ret = 0;
                break;
            }
            // 因发送方退出导致接收失败
            if (dst->recv_err)
            {
                dst->recv_err = 0;
                ret           = -ESRCH;
                break;
            }
            // 被信号打断: 立刻返回 -EINTR, 由调用方返回用户态后投递信号
            if (wake == -EINTR)
            {
                ret = -EINTR;
                break;
            }
            // 继续等
        }
    }

    // 无论是否接收成功,都从发送者的链表中移除
    if (src != NULL)
    {
        ipc_recv_leave(dst, src);
    }
    dst->recv_from = PID_NULL;
    return ret;
}
