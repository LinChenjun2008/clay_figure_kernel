// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <std/string.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/schedule.h>

static void *copy_from_user(void *dst, const void *src, size_t size)
{
    if ((uintptr_t)src >= KERNEL_VMA_BASE)
    {
        return NULL;
    }
    return memcpy(dst, src, size);
}

static int msg_send_lock(struct mailbox *dst, struct mailbox *src)
{
    struct task *dest_task = mailbox_to_task(dst);
    struct task *src_task  = mailbox_to_task(src);
    int          need_wake = 1;

    if (dst->closed)
    {
        return -1;
    }

    src->send_to = dest_task->pid;
    list_append(&dst->send_list, &src->send_node);

    if (dst->recv_from == PID_NULL)
    {
        need_wake = 0;
    }
    if (!msg_match(dst->recv_from, dest_task->pid, src_task->pid))
    {
        need_wake = 0;
    }
    return need_wake;
}

int msg_send(pid_t dst_pid, struct message *msg)
{
    struct task *src_task  = get_current_task();
    struct task *dest_task = NULL;
    int          need_wake = 0;

    if (!check_pid_avaiability(dst_pid) || dst_pid == src_task->pid)
    {
        return -1;
    }
    dest_task = pid_to_task(dst_pid);
    if (dest_task == NULL || TASK_STATUS(dest_task->status) == TASK_DIED)
    {
        return -1;
    }

    struct mailbox *src = &src_task->mailbox;
    struct mailbox *dst = &dest_task->mailbox;

    copy_from_user(&src->msg, msg, sizeof(*msg));
    src->msg.source = src_task->pid;

    spin_lock(&dst->send_lock);
    need_wake = msg_send_lock(dst, src);
    spin_unlock(&dst->send_lock);

    if (need_wake < 0)
    {
        return -1;
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

        task_unblock(dest_task->pid, WAKE_NORMAL);
    }

    // 阻塞直到消息被接收或接收者退出.
    while (src->send_to == dest_task->pid)
    {
        enum task_wake_reason reason = task_block(TASK_SEND);
        // 被信号中断
        if (reason == WAKE_SIGNAL && src->send_to == dest_task->pid)
        {
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
            return -1;
        }
    }

    if (src->send_to != PID_NULL)
    {
        return -1;
    }
    return 0;
}

static int inform_event_lock(struct mailbox *dst, uint32_t evt_type)
{
    if (dst->closed)
    {
        return 0;
    }
    if (dst->evt_msg[evt_type] != 0xff)
    {
        dst->evt_msg[evt_type]++;
    }
    return msg_match(dst->recv_from, PID_NULL, PID_EVENT);
}

void inform_event(pid_t dst_pid, uint32_t evt_type)
{
    if (evt_type >= EVT_NR)
    {
        return;
    }
    struct task *dest_task = pid_to_task(dst_pid);
    if (dest_task == NULL || TASK_STATUS(dest_task->status) == TASK_DIED)
    {
        return;
    }
    int need_wake = 0;

    struct mailbox *dst = &dest_task->mailbox;
    spin_lock(&dst->send_lock);
    need_wake = inform_event_lock(dst, evt_type);
    spin_unlock(&dst->send_lock);

    if (need_wake)
    {
        task_unblock(dst_pid, WAKE_NORMAL);
    }
    return;
}
