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

static void *copy_to_user(void *dst, const void *src, size_t size)
{
    if ((uintptr_t)dst >= KERNEL_VMA_BASE)
    {
        return NULL;
    }
    return memcpy(dst, src, size);
}

void init_mailbox(struct mailbox *mailbox)
{
    memset(&mailbox->msg, 0, sizeof(mailbox->msg));
    memset(&mailbox->evt_msg, 0, sizeof(mailbox->evt_msg));
    mailbox->send_to   = PID_NULL;
    mailbox->recv_from = PID_NULL;
    mailbox->closed    = 0;
    init_list(&mailbox->send_list);
    init_spinlock(&mailbox->send_lock);
    return;
}

static struct mailbox *send_node_to_mailbox(struct list_node *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return CONTAINER_OF(struct mailbox, send_node, node);
}

static struct task *mailbox_to_task(struct mailbox *mailbox)
{
    return CONTAINER_OF(struct task, mailbox, mailbox);
}

void mailbox_cleanup(struct task *task)
{
    struct mailbox *dst = &task->mailbox;
    struct list     wake_list;
    init_list(&wake_list);

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
        task_unblock(src_task->pid);
    }
    return;
}

static int msg_match(pid_t from, pid_t dst_pid, pid_t src_pid)
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
    if (dest_task == NULL || dest_task->status == TASK_DIED)
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
        task_unblock(dst_pid);
    }
    return;
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
    if (dest_task == NULL || dest_task->status == TASK_DIED)
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
        task_unblock(dest_task->pid);
    }

    task_block(TASK_SEND);
    if (src->send_to != PID_NULL)
    {
        return -1;
    }
    return 0;
}

static int msg_event_lock(struct mailbox *dst, pid_t from)
{
    if (from != PID_ANY && from != PID_EVENT)
    {
        return 0;
    }

    int has_event = 0;
    int i;
    for (i = 0; i < EVT_NR; i++)
    {
        if (dst->evt_msg[i] == 0)
        {
            continue;
        }
        has_event = 1;
        break;
    }
    if (!has_event)
    {
        return 0;
    }

    struct message *evt = &dst->msg;
    memset(evt, 0, sizeof(*evt));
    evt->source = PID_EVENT;
    for (i = 0; i < EVT_NR; i++)
    {
        if (dst->evt_msg[i] == 0)
        {
            continue;
        }
        evt->type |= (1U << i);
        evt->m32[i]     = dst->evt_msg[i];
        dst->evt_msg[i] = 0;
    }
    return 1;
}

struct check_send_list_pack
{
    pid_t dst_pid;
    pid_t from;
};

static int check_send_list(struct list_node *node, void *arg)
{
    struct check_send_list_pack *pack = arg;

    struct mailbox *src      = send_node_to_mailbox(node);
    struct task    *src_task = mailbox_to_task(src);
    return msg_match(pack->from, pack->dst_pid, src_task->pid);
}

static struct task *
msg_recv_lock(struct mailbox *dst, pid_t dst_pid, pid_t from)
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

int msg_recv(pid_t from, struct message *msg)
{
    struct task    *dest_task     = get_current_task();
    struct mailbox *dst           = &dest_task->mailbox;
    struct task    *src_task      = NULL;
    int             has_event_msg = 0;

    while (1)
    {
        spin_lock(&dst->send_lock);
        has_event_msg = msg_event_lock(dst, from);
        if (!has_event_msg)
        {
            src_task = msg_recv_lock(dst, dest_task->pid, from);
        }
        spin_unlock(&dst->send_lock);

        if (has_event_msg)
        {
            copy_to_user(msg, &dst->msg, sizeof(*msg));
            return 0;
        }
        if (src_task != NULL)
        {
            copy_to_user(msg, &dst->msg, sizeof(*msg));
            task_unblock(src_task->pid);
            return 0;
        }
        task_block(TASK_RECEIVE);
    }
    return -1;
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