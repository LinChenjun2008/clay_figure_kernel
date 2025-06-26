/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#include <kernel/global.h>
#include <task/task.h>   // task_struct_t running_task,task block/unblock,list
#include <service.h>     // is_service_id,service_id_to_pid
#include <std/string.h>  // memcpy
#include <intr.h>        // intr_disable,intr_enable
#include <sync/atomic.h> // atomic_inc,atomic_dec
#include <kernel/syscall.h>

#include <log.h>

PRIVATE bool deadlock(pid_t src, pid_t dest)
{
    if (src == dest)
    {
        return TRUE;
    }
    task_struct_t *task_dest = pid2task(dest);
    while (1)
    {
        if (task_dest->status == TASK_SENDING)
        {
            if (task_dest->send_to == src)
            {
                return TRUE;
            }
            task_dest = pid2task(task_dest->send_to);
            if (task_dest == NULL)
            {
                return FALSE;
            }
        }
        break;
    }
    return FALSE;
}

PUBLIC void inform_intr(pid_t dest)
{
    if (is_service_id(dest))
    {
        dest = service_id_to_pid(dest);
    }
    if (!task_exist(dest))
    {
        return;
    }
    task_struct_t *receiver = pid2task(dest);
    receiver->has_intr_msg++;
    // update_vruntime(receiver);
    atomic_set(&receiver->recv_flag, 0);
    return;
}

PRIVATE void wait_recevice(void)
{

    task_struct_t *sender   = running_task();
    task_struct_t *receiver = pid2task(sender->send_to);

    spinlock_lock(&receiver->send_lock);
    list_append(&receiver->sender_list, &sender->send_tag);
    // update_vruntime(receiver);
    atomic_set(&receiver->recv_flag, 0);
    spinlock_unlock(&receiver->send_lock);

    atomic_inc(&sender->send_flag);

    intr_status_t intr_status = intr_enable();
    task_yield();
    intr_set_status(intr_status);
    return;
}

PUBLIC syscall_status_t msg_send(pid_t dest, message_t *msg)
{
    ASSERT(task_exist(dest));
    if (!task_exist(dest))
    {
        running_task()->send_to = MAX_TASK;
        return SYSCALL_DEST_NOT_EXIST;
    }
    task_struct_t *sender   = running_task();
    task_struct_t *receiver = pid2task(dest);
    sender->send_to         = dest;
    msg->src                = sender->pid;
    ASSERT(!deadlock(sender->pid, dest));
    if (deadlock(sender->pid, dest))
    {
        pr_log(
            LOG_WARN, "\3'%s' -> '%s' dead lock\n", sender->name, receiver->name
        );
        return K_DEADLOCK;
    }
    memcpy(&sender->msg, msg, sizeof(message_t));
    wait_recevice();
    return SYSCALL_SUCCESS;
}

PRIVATE void inform_receive(pid_t sender_pid)
{
    task_struct_t *sender = pid2task(sender_pid);
    sender->send_to       = MAX_TASK;
    // update_vruntime(sender);
    atomic_dec(&sender->send_flag);
    return;
}

PUBLIC syscall_status_t msg_recv(pid_t src, message_t *msg)
{
    task_struct_t *receiver = running_task();
    task_struct_t *sender;
    if (src == receiver->pid)
    {
        return K_DEADLOCK;
    }
    receiver->recv_from = src;
    if (src == RECV_FROM_ANY || src == RECV_FROM_INT)
    {
        spinlock_lock(&receiver->send_lock);
        bool has_msg_received = !list_empty(&receiver->sender_list);
        spinlock_unlock(&receiver->send_lock);
        if (!has_msg_received && !receiver->has_intr_msg)
        {
            atomic_set(&receiver->recv_flag, 1);
            task_yield();
        }
        if (receiver->has_intr_msg)
        {
            msg->src               = RECV_FROM_INT;
            msg->type              = RECV_FROM_INT;
            msg->m1.i1             = receiver->has_intr_msg;
            receiver->has_intr_msg = 0;
            return SYSCALL_SUCCESS;
        }
        spinlock_lock(&receiver->send_lock);
        ASSERT(!list_empty(&receiver->sender_list));
        list_node_t *src_node;
        src_node = list_pop(&receiver->sender_list);
        spinlock_unlock(&receiver->send_lock);
        sender = CONTAINER_OF(task_struct_t, send_tag, src_node);
    }
    else
    {
        ASSERT(task_exist(src));
        if (!task_exist(src))
        {
            receiver->recv_from = MAX_TASK;
            return SYSCALL_SRC_NOT_EXIST;
        }
        sender = pid2task(src);
        spinlock_lock(&receiver->send_lock);

        while (!list_find(&receiver->sender_list, &sender->send_tag))
        {
            spinlock_unlock(&receiver->send_lock);
            atomic_set(&receiver->recv_flag, 1);
            task_yield();
            spinlock_lock(&receiver->send_lock);
        }
        list_remove(&sender->send_tag);
        spinlock_unlock(&receiver->send_lock);
    }
    memcpy(msg, &sender->msg, sizeof(message_t));
    receiver->recv_from = MAX_TASK;
    inform_receive(sender->pid);
    return SYSCALL_SUCCESS;
}
