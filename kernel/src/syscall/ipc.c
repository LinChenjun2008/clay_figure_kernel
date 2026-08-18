// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <std/string.h>
#include <syscall.h>
#include <task.h>

static int msg_match(pid_t from, pid_t recv, pid_t send)
{
    struct task *recv_task = pid_to_task(recv);
    struct task *send_task = pid_to_task(send);
    switch (from)
    {
        case PID_ANY:
            return 1;
        case PID_CHILD:
            return send_task->ppid == recv_task->pid;
        case PID_EVENT:
            return send == PID_EVENT;
        default:
            return from >= 0 && send_task->pid == from;
    }
    return 0;
}

static int msg_send_lock(struct task *dest, struct task *curr)
{
    int need_wake = 1;

    curr->send_to = dest->pid;
    list_append(&dest->send_list, &curr->send_node);

    if (dest->recv_from == PID_NULL)
    {
        need_wake = 0;
    }
    if (!msg_match(dest->recv_from, curr->send_to, curr->pid))
    {
        need_wake = 0;
    }
    return need_wake;
}

int msg_send(pid_t dst, struct message *msg)
{
    struct task *curr      = get_current_task();
    struct task *dest      = NULL;
    int          need_wake = 0;

    if (!check_pid_avaiability(dst) || dst == curr->pid)
    {
        return -1;
    }
    dest = pid_to_task(dst);
    ASSERT(dest != NULL && dest->status != TASK_DIED);

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

static int inform_event_lock(struct task *task, uint32_t evt_type)
{
    if (task->evt_msg[evt_type] != 0xff)
    {
        task->evt_msg[evt_type]++;
    }
    return msg_match(task->recv_from, PID_NULL, PID_EVENT);
}

void inform_event(pid_t pid, uint32_t evt_type)
{
    if (evt_type >= EVT_NR)
    {
        return;
    }
    struct task *task = pid_to_task(pid);
    if (task == NULL || task->status == TASK_DIED)
    {
        return;
    }
    int need_wake = 0;

    spin_lock(&task->send_lock);
    need_wake = inform_event_lock(task, evt_type);
    spin_unlock(&task->send_lock);

    if (need_wake)
    {
        task_unblock(pid);
    }
    return;
}

static int msg_event_lock(struct task *self, pid_t from)
{
    if (from != PID_ANY && from != PID_EVENT)
    {
        return 0;
    }

    int has_event = 0;
    int i;
    for (i = 0; i < EVT_NR; i++)
    {
        if (self->evt_msg[i] == 0)
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

    struct message *evt = &self->msg;
    memset(evt, 0, sizeof(*evt));
    evt->source = PID_EVENT;
    for (i = 0; i < EVT_NR; i++)
    {
        if (self->evt_msg[i] == 0)
        {
            continue;
        }
        evt->type |= (1U << i);
        evt->m32[i]      = self->evt_msg[i];
        self->evt_msg[i] = 0;
    }
    return 1;
}

struct check_send_list_pack
{
    struct task *self;
    pid_t        from;
};

static int check_send_list(struct list_node *node, void *arg)
{
    struct check_send_list_pack *pack = arg;

    struct task *send = CONTAINER_OF(struct task, send_node, node);

    return msg_match(pack->from, pack->self->pid, send->pid);
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
    struct task *self          = get_current_task();
    struct task *send          = NULL;
    int          has_event_msg = 0;

    while (1)
    {
        spin_lock(&self->send_lock);
        has_event_msg = msg_event_lock(self, from);
        if (!has_event_msg)
        {
            send = msg_recv_lock(self, from);
        }
        spin_unlock(&self->send_lock);

        if (has_event_msg)
        {
            memcpy(msg, &self->msg, sizeof(*msg));
            return PID_EVENT;
        }
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