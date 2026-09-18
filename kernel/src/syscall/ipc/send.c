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

static void *copy_from_user(void *dst, const void *src, size_t size)
{
    if ((uintptr_t)src >= KERNEL_VMA_BASE)
    {
        return NULL;
    }
    return memcpy(dst, src, size);
}

static int ipc_send_lock(struct mailbox *dst, struct mailbox *src)
{
    struct task *dest_task = mailbox_to_task(dst);
    struct task *src_task  = mailbox_to_task(src);
    int          need_wake = 1;

    if (dst->closed)
    {
        return -ESRCH;
    }

    src->send_to = dest_task->pid;
    list_append(&dst->send_list, &src->send_node);

    if (dst->recv_from == PID_NULL)
    {
        need_wake = 0;
    }
    if (!ipc_match(dst->recv_from, dest_task->pid, src_task->pid))
    {
        need_wake = 0;
    }
    return need_wake;
}

int ipc_send(pid_t dst_pid, struct message *msg)
{
    struct task *src_task  = get_current_task();
    struct task *dest_task = NULL;
    int          need_wake = 0;

    if (!check_pid_avaiability(dst_pid))
    {
        return -ESRCH;
    }
    if (dst_pid == src_task->pid)
    {
        return -EINVAL;
    }
    dest_task = pid_to_task(dst_pid);
    if (dest_task == NULL || TASK_STATUS(dest_task->status) == TASK_DIED)
    {
        return -ESRCH;
    }

    struct mailbox *src = &src_task->mailbox;
    struct mailbox *dst = &dest_task->mailbox;

    memset(&src->msg, 0, sizeof(src->msg));
    copy_from_user(&src->msg, msg, sizeof(*msg));
    src->msg.source = src_task->pid;

    spin_lock(&dst->send_lock);
    need_wake = ipc_send_lock(dst, src);
    spin_unlock(&dst->send_lock);

    if (need_wake < 0)
    {
        return need_wake;
    }
    if (need_wake)
    {
        // 接收者在等待: 从自己的 recv_list 出队并唤醒
        spin_lock(&src->recv_lock);
        if (list_find(&src->recv_list, &dst->recv_node))
        {
            list_remove(&dst->recv_node);
        }
        spin_unlock(&src->recv_lock);

        wake_up(&dst->recv_wq);
    }

    // 等到消息被接收, 或接收者退出
    struct ipc_send_pack pack;
    pack.task    = src_task;
    pack.dst_pid = dest_task->pid;

    int wake = wait_event(&src->send_wq, &pack, TASK_SEND);

    // 信号打断
    if (wake == -EINTR && src->send_to == dest_task->pid)
    {
        // 撤回本次发送登记
        spin_lock(&dst->send_lock);
        if (src->send_to == dest_task->pid)
        {
            if (list_find(&dst->send_list, &src->send_node))
            {
                list_remove(&src->send_node);
            }
            src->send_to = PID_NULL;
        }
        spin_unlock(&dst->send_lock);
        return -EINTR;
    }

    if (src->send_to != PID_NULL)
    {
        return -ESRCH;
    }
    return 0;
}

// static int inform_event_lock(struct mailbox *dst, uint32_t evt_type)
// {
//     if (dst->closed)
//     {
//         return 0;
//     }
//     if (dst->evt_msg[evt_type] != 0xff)
//     {
//         dst->evt_msg[evt_type]++;
//     }
//     return ipc_match(dst->recv_from, PID_NULL, PID_EVENT);
// }

// void inform_event(pid_t dst_pid, uint32_t evt_type)
// {
//     if (evt_type >= EVT_NR)
//     {
//         return;
//     }
//     struct task *dest_task = pid_to_task(dst_pid);
//     if (dest_task == NULL || TASK_STATUS(dest_task->status) == TASK_DIED)
//     {
//         return;
//     }
//     int need_wake = 0;

//     struct mailbox *dst = &dest_task->mailbox;
//     spin_lock(&dst->send_lock);
//     need_wake = inform_event_lock(dst, evt_type);
//     spin_unlock(&dst->send_lock);

//     if (need_wake)
//     {
//         wake_up(&dst->recv_wq);
//     }
//     return;
// }
