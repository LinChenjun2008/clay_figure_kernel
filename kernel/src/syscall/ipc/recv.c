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

// 跨进程复制
static int
copy_from_other(struct msg_head *dst_msg, uintptr_t src, struct task *src_task)
{
    phys_addr_t src_phys = to_physical_address(src_task->pg_dir, src);
    if (src_phys == 0)
    {
        // 发送方的消息缓冲区所在页未映射
        return -EFAULT;
    }
    struct msg_head *src_msg = PHYS_TO_VIRT(src_phys);
    if (src_msg->legnth > dst_msg->legnth)
    {
        // 接收方缓冲区不够: 回写自身容量, 供发送方重整后重试
        src_msg->legnth = dst_msg->legnth;
        return -E2BIG;
    }
    memcpy(dst_msg, src_msg, src_msg->legnth);
    return 0;
}

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
        return NULL;
    }

    struct mailbox *src = send_node_to_mailbox(node);
    return mailbox_to_task(src);
}

// 尝试取一条匹配消息并投递.
static int
ipc_recv_try(struct task *dest_task, pid_t from, struct msg_head *msg)
{
    struct mailbox *dst = &dest_task->mailbox;
    struct task    *src_task;

    spin_lock(&dst->send_lock);

    src_task = ipc_recv_lock(dst, dest_task->pid, from);
    if (src_task == NULL)
    {
        spin_unlock(&dst->send_lock);
        return -ENOENT;
    }
    struct mailbox *src = &src_task->mailbox;

    int ret = copy_from_other(msg, (uintptr_t)src->msg, src_task);

    src->send_err = ret;
    src->send_to  = PID_NULL;

    spin_unlock(&dst->send_lock);

    wake_up(&src_task->mailbox.send_wq);
    return ret;
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

int ipc_recv(pid_t src_pid, struct msg_head *msg)
{
    struct task    *dest_task = get_current_task();
    struct mailbox *dst       = &dest_task->mailbox;
    struct task    *src_task  = NULL;
    struct mailbox *src       = NULL;

    dst->recv_err = 0;

    int msg_status = check_message(dest_task, msg);
    if (msg_status < 0)
    {
        return msg_status;
    }

    dst->recv_from = src_pid;

    // 有效pid: 从特定任务接收消息
    if (check_pid_avaiability(src_pid))
    {
        int closed;

        src_task = pid_to_task(src_pid);
        if (src_task == NULL)
        {
            dst->recv_from = PID_NULL;
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
            dst->recv_from = PID_NULL;
            return -ESRCH;
        }
    }
    int ret = -ENOENT;

    struct ipc_recv_pack pack;
    pack.task = dest_task;
    pack.from = src_pid;

    while (1)
    {
        int recv_status = ipc_recv_try(dest_task, src_pid, msg);
        // recv_status:
        // 0      成功收到消息
        // ENOENT 没有消息
        // 其它   收到了但被丢弃(长度不足 -E2BIG / 源地址无效 -EFAULT), 继续等

        if (recv_status == 0)
        {
            ret = 0;
            break;
        }

        int wake = wait_event(&dst->recv_wq, &pack, TASK_RECEIVE);

        // 接收出现错误
        if (dst->recv_err != 0)
        {
            ret           = dst->recv_err;
            dst->recv_err = 0;
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


    // 无论是否接收成功,都从发送者的链表中移除
    if (src != NULL)
    {
        ipc_recv_leave(dst, src);
    }
    dst->recv_from = PID_NULL;
    return ret;
}
