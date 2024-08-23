#include <kernel/global.h>
#include <task/task.h>
#include <mem/mem.h>
#include <std/string.h>
#include <service.h>

#include <log.h>

PRIVATE bool deadlock(pid_t src,pid_t dest)
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
    pid_t recv_from = receiver->recv_from;
    if (recv_from == RECV_FROM_ANY || recv_from == RECV_FROM_INT)
    {
        if (receiver->status == TASK_RECEIVING)
        {
            task_unblock(dest);
        }
    }
    return;
}

PUBLIC syscall_status_t msg_send(pid_t dest,message_t* msg)
{
    ASSERT(task_exist(dest));
    if (!task_exist(dest))
    {
        running_task()->send_to = MAX_TASK;
        return SYSCALL_DEST_NOT_EXIST;
    }
    task_struct_t *sender   = running_task();
    task_struct_t *receiver = pid2task(dest);
    sender->send_to = dest;
    msg->src = sender->pid;
    ASSERT(!deadlock(sender->pid,dest));
    if (deadlock(sender->pid,dest))
    {
        pr_log("\3'%s' -> '%s' dead lock\n",
               sender->name,
               receiver->name);
        return SYSCALL_DEADLOCK;
    }
    memcpy(&sender->msg,msg,sizeof(message_t));
    spinlock_lock(&receiver->send_lock);
    list_append(&receiver->sender_list,&sender->general_tag);
    if (receiver->status == TASK_RECEIVING)
    {
        if (receiver->recv_from == RECV_FROM_ANY
           || receiver->recv_from == sender->pid)
        {
            task_unblock(receiver->pid);
        }
    }
    spinlock_unlock(&receiver->send_lock);
    task_block(TASK_SENDING);
    sender->send_to = MAX_TASK;
    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t msg_recv(pid_t src,message_t *msg)
{
    task_struct_t *receiver = running_task();
    task_struct_t *sender;
    if (src == receiver->pid)
    {
        return SYSCALL_DEADLOCK;
    }
    receiver->recv_from = src;
    if (src == RECV_FROM_ANY || src == RECV_FROM_INT)
    {
        spinlock_lock(&receiver->send_lock);
        bool has_msg_received = !list_empty(&receiver->sender_list);
        spinlock_unlock(&receiver->send_lock);
        if (!has_msg_received && !receiver->has_intr_msg)
        {
            task_block(TASK_RECEIVING);
        }
        if (receiver->has_intr_msg)
        {
            msg->src  = RECV_FROM_INT;
            msg->type = RECV_FROM_INT;
            msg->m1.i1 = receiver->has_intr_msg;
            receiver->has_intr_msg = 0;
            return SYSCALL_SUCCESS;
        }
        spinlock_lock(&receiver->send_lock);
        ASSERT(!list_empty(&receiver->sender_list));
        list_node_t *src_node;
        src_node = list_pop(&receiver->sender_list);
        spinlock_unlock(&receiver->send_lock);
        sender = CONTAINER_OF(task_struct_t,general_tag,src_node);
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

        while (!list_find(&receiver->sender_list,
                          &sender->general_tag))
        {
            spinlock_unlock(&receiver->send_lock);
            task_block(TASK_RECEIVING);
            spinlock_lock(&receiver->send_lock);
        }
        list_remove(&sender->general_tag);
        spinlock_unlock(&receiver->send_lock);
    }
    memcpy(msg,&sender->msg,sizeof(message_t));
    sender->send_to = MAX_TASK;
    // ASSERT(sender->status == TASK_SENDING);
    task_unblock(sender->pid);
    receiver->recv_from = MAX_TASK;
    return SYSCALL_SUCCESS;
}
