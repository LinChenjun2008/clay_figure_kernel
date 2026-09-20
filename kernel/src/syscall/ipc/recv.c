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

// 从自己的等待队列取出队首消息并投递
// 返回: -ENOENT 没有消息
//       0       投递成功
//       其它    这条消息被丢弃(长度不足 / 源地址无效), 已写入发送方的send_status
static int ipc_do_recv(struct task *dst_task, struct msg_head *msg)
{
    struct mailbox   *dst  = &dst_task->mailbox;
    struct list_node *node = list_pop(&dst->send_list);

    if (node == NULL)
    {
        return -ENOENT;
    }
    struct mailbox *src      = send_node_to_mailbox(node);
    struct task    *src_task = mailbox_to_task(src);

    int ret = copy_from_other(msg, (uintptr_t)src->msg, src_task);

    src->send_status = ret;
    src->send_to     = PID_NULL;
    return ret;
}

/**
 * ipc_recv流程
 * 1. 检查参数合法性
 * 2. 从自己的等待队列取出队首消息并投递:
 * 2.1 摘除发送方的send_node
 * 2.2 跨进程复制消息到msg
 * 2.3 把复制结果写入发送方的send_status,随后唤醒发送方
 * 3. 没有消息可接收时:
 * 3.1 option带IPC_NOWAIT,立即返回-EAGAIN
 * 3.2 否则在recv_wq上等待发送方,被信号打断则返回-EINTR
 */
int ipc_recv(struct msg_head *msg, int option)
{
    struct task    *dst_task    = get_current_task();
    struct mailbox *dst         = &dst_task->mailbox;
    int             status      = 0;
    int             recv_status = 0;
    int             wake_status = 0;

    status = check_message(dst_task, msg);
    if (status < 0)
    {
        return status;
    }

    while (1)
    {
        // 尝试获取消息
        spin_lock(&dst->send_lock);
        recv_status = ipc_do_recv(dst_task, msg);
        spin_unlock(&dst->send_lock);

        wake_up(&dst->send_wq);

        // 根据status判断接收情况

        if (recv_status == 0)
        {
            // 顺利接收
            status = 0;
            break;
        }
        else if (recv_status == -E2BIG || recv_status == -EFAULT)
        {
            // 重新接收
            continue;
        }
        else if (recv_status == -ENOENT)
        {
            // 没有消息
            if (option & IPC_NOWAIT)
            {
                status = -EAGAIN;
                break;
            }
        }
        else
        {
            // 其他错误
            status = recv_status;
            break;
        }

        // 被信号或其他原因打断
        if (wake_status != 0)
        {
            status = wake_status;
            break;
        }
        wake_status = wait_event(&dst->recv_wq, dst, TASK_RECEIVE);
    }
    return recv_status;
}
