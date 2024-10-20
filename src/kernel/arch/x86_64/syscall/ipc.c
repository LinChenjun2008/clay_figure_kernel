/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <task/task.h>  // task_struct_t running_task,task block/unblock,list
#include <service.h>    // is_service_id,service_id_to_pid
#include <std/string.h> // memcpy

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
    receiver->recv_flag = 0;
    return;
}

PRIVATE void wait_recevice()
{
    task_struct_t *sender = running_task();
    sender->send_flag++;
    while(sender->send_flag) task_yield();
    // task_block(TASK_SENDING);
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
    list_append(&receiver->sender_list,&sender->send_tag);
    receiver->recv_flag = 0;
    spinlock_unlock(&receiver->send_lock);
    
    wait_recevice();
    return SYSCALL_SUCCESS;
}

PRIVATE void inform_receive(pid_t sender_pid)
{
    task_struct_t *sender = pid2task(sender_pid);
    sender->send_to = MAX_TASK;
    sender->send_flag--;
    // task_unblock(sender_pid);
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
            receiver->recv_flag = 1;
            while (receiver->recv_flag)
            {
                task_yield();
            }
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
        sender = CONTAINER_OF(task_struct_t,send_tag,src_node);
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
                          &sender->send_tag))
        {
            spinlock_unlock(&receiver->send_lock);
            receiver->recv_flag = 1;
            task_yield();
            spinlock_lock(&receiver->send_lock);
        }
        list_remove(&sender->send_tag);
        spinlock_unlock(&receiver->send_lock);
    }
    memcpy(msg,&sender->msg,sizeof(message_t));
    receiver->recv_from = MAX_TASK;
    inform_receive(sender->pid);
    return SYSCALL_SUCCESS;
}
