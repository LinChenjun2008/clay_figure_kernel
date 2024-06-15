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
    pid2task(dest)->has_intr_msg++;
    pid_t recv_from = pid2task(dest)->recv_from;
    if (recv_from == RECV_FROM_ANY || recv_from == RECV_FROM_INT)
    {
        if (pid2task(dest)->status == TASK_RECEIVING)
        {
            task_unblock(dest);
        }
    }
    return;
}

PUBLIC syscall_status_t msg_send(pid_t dest,message_t* msg)
{
    running_task()->send_to = dest;

    if (!task_exist(dest))
    {
        running_task()->send_to = MAX_TASK;
        return SYSCALL_DEST_NOT_EXIST;
    }
    msg->src = running_task()->pid;
    if (deadlock(running_task()->pid,dest))
    {
        pr_log("\3'%s' -> '%s' dead lock\n",running_task()->name,pid2task(dest)->name);
        return SYSCALL_DEADLOCK;
    }
    memcpy(&running_task()->msg,msg,sizeof(message_t));
    spinlock_lock(&pid2task(dest)->send_lock);
    list_append(&pid2task(dest)->sender_list,&running_task()->general_tag);
    if (pid2task(dest)->status == TASK_RECEIVING)
    {
        if (pid2task(dest)->recv_from == RECV_FROM_ANY
           || pid2task(dest)->recv_from == running_task()->pid)
        {
            task_unblock(dest);
        }
    }
    spinlock_unlock(&pid2task(dest)->send_lock);
    task_block(TASK_SENDING);
    running_task()->send_to = MAX_TASK;
    return SYSCALL_SUCCESS;
}

PUBLIC syscall_status_t msg_recv(pid_t src,message_t *msg)
{
    if (src == running_task()->pid)
    {
        return SYSCALL_DEADLOCK;
    }
    running_task()->recv_from = src;
    if (src == RECV_FROM_ANY || src == RECV_FROM_INT)
    {
        if (list_empty(&running_task()->sender_list) && !running_task()->has_intr_msg)
        {
            task_block(TASK_RECEIVING);
        }
        if (running_task()->has_intr_msg)
        {
            msg->src  = RECV_FROM_INT;
            msg->type = RECV_FROM_INT;
            msg->m1.i1 = running_task()->has_intr_msg;
            running_task()->has_intr_msg = 0;
            return SYSCALL_SUCCESS;
        }
        spinlock_lock(&running_task()->send_lock);
        src = CONTAINER_OF(task_struct_t,general_tag,list_pop(&running_task()->sender_list))->pid;
        spinlock_unlock(&running_task()->send_lock);
    }
    else
    {
        if (!task_exist(src))
        {
            running_task()->recv_from = MAX_TASK;
            return SYSCALL_SRC_NOT_EXIST;
        }
        if (pid2task(src) == NULL)
        {
            running_task()->recv_from = MAX_TASK;
            return SYSCALL_SRC_NOT_EXIST;
        }
        while (!list_find(&running_task()->sender_list,&pid2task(src)->general_tag))
        {
            task_block(TASK_RECEIVING);
        }
        spinlock_lock(&running_task()->send_lock);
        list_remove(&pid2task(src)->general_tag);
        spinlock_unlock(&running_task()->send_lock);
    }
    memcpy(msg,&pid2task(src)->msg,sizeof(message_t));
    pid2task(src)->send_to = MAX_TASK;
    if (pid2task(src)->status == TASK_SENDING)
    {
        task_unblock(src);
    }
    running_task()->recv_from = MAX_TASK;
    return SYSCALL_SUCCESS;
}
