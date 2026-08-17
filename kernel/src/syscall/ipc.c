// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <std/string.h>
#include <syscall.h>
#include <task.h>

static int msg_match(pid_t from, struct task *recv, struct task *send)
{
    switch (from)
    {
        case PID_ANY:
            return 1;
        case PID_CHILD:
            return send->ppid == recv->pid;
        default:
            return from >= 0 && send->pid == from;
    }
    return 0;
}

static int msg_send_lock(struct task *dest, struct task *curr)
{
    int wake = 0;

    curr->send_to = dest->pid;
    list_append(&dest->send_list, &curr->send_node);

    if (dest->recv_from != PID_NULL && msg_match(dest->recv_from, dest, curr))
    {
        wake = 1;
    }
    return wake;
}

int msg_send(pid_t dst, struct message *msg)
{
    struct task *curr      = get_current_task();
    struct task *dest      = NULL;
    int          need_wake = 0;

    if (dst < 0 || dst == curr->pid)
    {
        return -1;
    }
    dest = pid_to_task(dst);
    if (dest == NULL || dest->status == TASK_DIED)
    {
        return -1;
    }

    memcpy(&curr->msg, msg, sizeof(*msg));
    curr->msg.source = curr->pid;

    spin_lock(&dest->send_lock);
    need_wake = msg_send_lock(dest, curr);
    spin_unlock(&dest->send_lock);

    if (need_wake)
    {
        task_unblock(dest->pid);
    }

    task_block(TASK_SEND);
    return 0;
}

struct check_send_list_pack
{
    struct task *self;
    pid_t        from;
};

static int check_send_list(struct list_node *node, void *arg)
{
    struct check_send_list_pack *pack = arg;

    struct task *send = NULL;
    send              = CONTAINER_OF(struct task, send_node, node);
    if (msg_match(pack->from, pack->self, send))
    {
        return 1;
    }
    return 0;
}

static struct task *msg_recv_lock(struct task *self, pid_t from)
{
    struct list_node *node = NULL;

    struct check_send_list_pack pack;
    pack.self = self;
    pack.from = from;

    node = list_traversal_remove(&self->send_list, check_send_list, &pack);

    if (node == NULL)
    {
        self->recv_from = from;
        return NULL;
    }

    struct task *send = CONTAINER_OF(struct task, send_node, node);
    send->send_to     = PID_NULL;
    self->recv_from   = PID_NULL;
    memcpy(&self->msg, &send->msg, sizeof(self->msg));
    return send;
}

int msg_recv(pid_t from, struct message *msg)
{
    struct task *self = get_current_task();
    struct task *send = NULL;

    while (1)
    {
        spin_lock(&self->send_lock);
        send = msg_recv_lock(self, from);
        spin_unlock(&self->send_lock);

        if (send != NULL)
        {
            memcpy(msg, &self->msg, sizeof(*msg));
            task_unblock(send->pid);
            return send->pid;
        }
        task_block(TASK_RECEIVE);
    }
    return PID_NULL;
}