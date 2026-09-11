// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <std/string.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/schedule.h>

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
        task_unblock(src_task->pid, WAKE_NORMAL);
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
        task_unblock(dst_task->pid, WAKE_NORMAL);
    }
    return;
}

int msg_both(pid_t src_dst, struct message *msg)
{
    int ret = 0;
    ret     = msg_send(src_dst, msg);
    if (ret < 0)
    {
        return -1;
    }
    ret = msg_recv(src_dst, msg);
    if (ret < 0)
    {
        return -2;
    }
    return 0;
}