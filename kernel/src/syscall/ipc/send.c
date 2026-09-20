// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <errno.h>
#include <panic.h>
#include <std/string.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/schedule.h>
#include <task/struct.h>

// 发送消息
static int
ipc_do_send(struct task *dst_task, struct task *src_task, struct msg_head *msg)
{
    struct mailbox *dst = &dst_task->mailbox;
    struct mailbox *src = &src_task->mailbox;

    if (dst->closed)
    {
        return -ESRCH;
    }
    src->send_status = 1;
    src->send_to     = dst_task->pid;
    src->msg         = msg;
    msg->source      = src_task->pid;
    list_append(&dst->send_list, &src->send_node);
    return 0;
}

static int ipc_check_send_status(int wake_status, int send_status)
{
    // 成功
    if (wake_status == 0 && send_status == 0)
    {
        return 0;
    }
    // 被信号打断,导致消息没有被发送.
    if (wake_status == -EINTR && send_status > 0)
    {
        return -EINTR;
    }

    // 错误情况
    PANIC("Should not be here.\n");
    return 0;
}

int ipc_send(pid_t dst_pid, struct msg_head *msg)
{
    struct task    *src_task    = get_current_task();
    struct mailbox *src         = &src_task->mailbox;
    int             status      = 0;
    int             send_status = 0;
    int             wake_status = 0;

    // 检查参数状态
    status = check_message(src_task, msg);
    if (status < 0)
    {
        return status;
    }
    if (dst_pid == src_task->pid)
    {
        return -EDEADLK;
    }

    // 获取接收方相关信息
    struct task    *dst_task = NULL;
    struct mailbox *dst      = NULL;

    dst_task = pid_to_task(dst_pid);
    if (dst_task == NULL || TASK_STATUS(dst_task->status) == TASK_DIED)
    {
        return -ESRCH;
    }
    dst = &dst_task->mailbox;

    // 发送消息
    spin_lock(&dst->send_lock);
    send_status = ipc_do_send(dst_task, src_task, msg);
    spin_unlock(&dst->send_lock);

    if (send_status < 0)
    {
        status = send_status;
        goto end;
    }

    // 唤醒接收方,等待消息被取走
    wake_up(&dst->recv_wq);
    wake_status = wait_event(&dst->send_wq, &src_task->mailbox, TASK_SEND);
    send_status = src->send_status;

    status = ipc_check_send_status(wake_status, send_status);

end:
    spin_lock(&dst->send_lock);
    if (list_find(&dst->send_list, &src->send_node))
    {
        list_remove(&src->send_node);
    }
    spin_unlock(&dst->send_lock);

    src->msg         = NULL;
    src->send_status = 0;
    src->send_to     = PID_NULL;
    return status;
}
