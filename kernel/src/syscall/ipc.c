// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <std/string.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/schedule.h>
#include <task/struct.h>

struct mailbox *send_node_to_mailbox(struct list_node *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return CONTAINER_OF(struct mailbox, send_node, node);
}

struct task *mailbox_to_task(struct mailbox *mailbox)
{
    return CONTAINER_OF(struct task, mailbox, mailbox);
}

struct mailbox *recv_node_to_mailbox(struct list_node *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return CONTAINER_OF(struct mailbox, recv_node, node);
}

int msg_match(pid_t from, pid_t dst_pid, pid_t src_pid)
{
    if (from == PID_ANY)
    {
        return 1;
    }
    if (src_pid == PID_EVENT)
    {
        return from == PID_EVENT;
    }

    struct task *dest_task = pid_to_task(dst_pid);
    struct task *src_task  = pid_to_task(src_pid);
    if (from == PID_CHILD)
    {
        return src_task->ppid == dest_task->pid;
    }

    return from >= 0 && src_task->pid == from;
}

// 检查 send_list 中是否有匹配的消息来源
int check_send_list(struct list_node *node, void *arg)
{
    struct check_send_list_pack *pack = arg;

    struct mailbox *src      = send_node_to_mailbox(node);
    struct task    *src_task = mailbox_to_task(src);
    return msg_match(pack->from, pack->dst_pid, src_task->pid);
}

// wait_event 条件: 有可接收的匹配消息/事件, 或来源已退出
int msg_recv_wakeup_condition(void *arg)
{
    struct msg_recv_pack *pack  = arg;
    struct mailbox       *dst   = &pack->task->mailbox;
    int                   ready = 0;

    spin_lock(&dst->send_lock);

    // 事件(与 msg_event_lock 的匹配规则一致)
    if (pack->from == PID_ANY || pack->from == PID_EVENT)
    {
        int i;
        for (i = 0; i < EVT_NR; i++)
        {
            if (dst->evt_msg[i] != 0)
            {
                ready = 1;
                break;
            }
        }
    }
    // 其他任务发来的消息
    if (!ready)
    {
        struct check_send_list_pack check;
        check.dst_pid = pack->task->pid;
        check.from    = pack->from;
        ready =
            list_traversal(&dst->send_list, check_send_list, &check) != NULL;
    }

    spin_unlock(&dst->send_lock);

    if (dst->recv_err) // 消息来源已退出
    {
        ready = 1;
    }
    return ready;
}

// wait_event 条件: 本任务发出的消息已被接收, 或接收者已退出
int msg_send_wakeup_condition(void *arg)
{
    struct msg_send_pack *pack = arg;
    return pack->task->mailbox.send_to != pack->dst_pid;
}

void init_mailbox(struct mailbox *mailbox)
{
    memset(&mailbox->msg, 0, sizeof(mailbox->msg));
    memset(&mailbox->evt_msg, 0, sizeof(mailbox->evt_msg));
    mailbox->send_to   = PID_NULL;
    mailbox->recv_from = PID_NULL;
    mailbox->closed    = 0;
    mailbox->recv_err  = 0;
    init_list(&mailbox->send_list);
    init_spinlock(&mailbox->send_lock);
    init_list(&mailbox->recv_list);
    init_spinlock(&mailbox->recv_lock);
    init_wait_queue(&mailbox->recv_wq, msg_recv_wakeup_condition);
    init_wait_queue(&mailbox->send_wq, msg_send_wakeup_condition);
    return;
}

void mailbox_cleanup(struct task *task)
{
    struct mailbox *dst = &task->mailbox;
    struct list     wake_list;
    init_list(&wake_list);

    // 关闭邮箱,并移除所有正在发送的任务
    spin_lock(&dst->send_lock);
    dst->closed = 1;
    while (!list_empty(&dst->send_list))
    {
        struct list_node *node    = list_pop(&dst->send_list);
        struct task *src_task     = mailbox_to_task(send_node_to_mailbox(node));
        src_task->mailbox.send_to = PID_ERROR;
        list_append(&wake_list, node);
    }
    spin_unlock(&dst->send_lock);

    while (!list_empty(&wake_list))
    {
        struct list_node *node = list_pop(&wake_list);
        struct task *src_task  = mailbox_to_task(send_node_to_mailbox(node));

        wake_up(&src_task->mailbox.send_wq);
    }

    // 移除所有正在等待的任务
    spin_lock(&dst->recv_lock);
    while (!list_empty(&dst->recv_list))
    {
        struct list_node *node = list_pop(&dst->recv_list);
        struct task *dst_task  = mailbox_to_task(recv_node_to_mailbox(node));
        dst_task->mailbox.recv_err = 1;
        list_append(&wake_list, node);
    }
    spin_unlock(&dst->recv_lock);

    while (!list_empty(&wake_list))
    {
        struct list_node *node = list_pop(&wake_list);
        struct task *dst_task  = mailbox_to_task(recv_node_to_mailbox(node));

        wake_up(&dst_task->mailbox.recv_wq);
    }
    return;
}
