// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <kernel/syscall.h>
#include <service.h>     // is_service_id,service_id_to_pid
#include <std/string.h>  // memcpy
#include <sync/atomic.h> // atomic_inc,atomic_dec
#include <task/task.h>   // task_struct_t running_task,list

PUBLIC void inform_intr(pid_t dst)
{
    if (is_service_id(dst))
    {
        dst = service_id_to_pid(dst);
    }
    if (!task_exist(dst))
    {
        return;
    }
    task_struct_t *receiver = pid_to_task(dst);
    receiver->has_intr_msg  = 1;

    return;
}

/**
 * @brief 判断src向dst发送消息是否会死锁
 * @param src
 * @param dst
 * @return
 */
PRIVATE bool deadlock(pid_t src, pid_t dst)
{
    if (src == dst)
    {
        return TRUE;
    }
    task_struct_t *task_dst = pid_to_task(dst);
    while (1)
    {
        if (task_dst->status == TASK_SENDING)
        {
            if (task_dst->send_to == src)
            {
                return TRUE;
            }
            task_dst = pid_to_task(task_dst->send_to);
            if (task_dst == NULL)
            {
                return FALSE;
            }
        }
        else
        {
            break;
        }
    }
    return FALSE;
}

/**
 * @brief 等待消息被接收
 * @param
 * @return
 */
PRIVATE void wait_receviced(void)
{

    task_struct_t *sender   = running_task();
    task_struct_t *receiver = pid_to_task(sender->send_to);

    spinlock_lock(&receiver->send_lock);
    list_append(&receiver->sender_list, &sender->send_tag);
    spinlock_unlock(&receiver->send_lock);

    // 如果此刻receiver被唤醒,则sender->send_flag < 0

    atomic_inc(&sender->send_flag);
    task_yield();
    return;
}

PUBLIC syscall_status_t msg_send(pid_t dst, message_t *msg)
{
    ASSERT(task_exist(dst));
    if (!task_exist(dst))
    {
        running_task()->send_to = MAX_TASK;
        return SYSCALL_DEST_NOT_EXIST;
    }
    task_struct_t *sender   = running_task();
    task_struct_t *receiver = pid_to_task(dst);
    sender->send_to         = dst;
    msg->src                = sender->pid;
    ASSERT(!deadlock(sender->pid, dst));
    if (deadlock(sender->pid, dst))
    {
        PR_LOG(
            LOG_WARN, "'%s' -> '%s' dead lock\n", sender->name, receiver->name
        );
        return K_DEADLOCK;
    }
    memcpy(&sender->msg, msg, sizeof(message_t));
    wait_receviced();
    sender->send_to = MAX_TASK;
    return SYSCALL_SUCCESS;
}

/**
 * 接收消息
 */

/**
 * @brief 提醒消息发出者消息已被收到
 * @param pid
 * @return
 */
PRIVATE void inform_received(pid_t pid)
{
    task_struct_t *sender = pid_to_task(pid);
    atomic_dec(&sender->send_flag);
    return;
}

/**
 * @brief 检测是否收到了来自任意进程的消息
 * @param pid pid
 * @return
 */
PRIVATE int received_from_any_task(pid_t pid)
{
    task_struct_t *receiver = pid_to_task(pid);

    spinlock_lock(&receiver->send_lock);
    int ret = !list_empty(&receiver->sender_list);
    spinlock_unlock(&receiver->send_lock);

    return ret;
}

/**
 * @brief 检测是否收到了任意消息
 * @param pid pid
 * @return
 */
PRIVATE int received_any_message(pid_t pid)
{
    task_struct_t *receiver = pid_to_task(pid);
    return receiver->has_intr_msg || received_from_any_task(pid);
}

/**
 * @brief 检测是否收到了来自特定进程的消息
 * @param pid
 * @param src
 * @return
 */
PRIVATE int received_from(pid_t pid, pid_t src)
{
    task_struct_t *receiver = pid_to_task(pid);
    task_struct_t *sender   = pid_to_task(src);

    spinlock_lock(&receiver->send_lock);
    int ret = list_find(&receiver->sender_list, &sender->send_tag);
    spinlock_unlock(&receiver->send_lock);

    return ret;
}

PUBLIC syscall_status_t msg_recv(pid_t src, message_t *msg)
{
    task_struct_t *receiver = running_task();
    task_struct_t *sender   = NULL;
    if (src == receiver->pid)
    {
        return K_DEADLOCK;
    }

    if (src != RECV_FROM_ANY && src != RECV_FROM_INT)
    {
        // 从特定进程接收消息 - 确保对应进程存在
        ASSERT(task_exist(src));
        if (!task_exist(src))
        {
            receiver->recv_from = MAX_TASK;
            return SYSCALL_SRC_NOT_EXIST;
        }
        sender = pid_to_task(src);
    }
    receiver->recv_from = src;

    atomic_set(&receiver->recv_flag, 1);
    task_yield();

    if (src == RECV_FROM_ANY || src == RECV_FROM_INT)
    {
        if (receiver->has_intr_msg)
        {
            receiver->recv_from    = MAX_TASK;
            receiver->has_intr_msg = 0;
            msg->src               = RECV_FROM_INT;
            msg->type              = RECV_FROM_INT;
            return SYSCALL_SUCCESS;
        }
        spinlock_lock(&receiver->send_lock);
        list_node_t *src_node;
        src_node = list_pop(&receiver->sender_list);
        spinlock_unlock(&receiver->send_lock);
        sender = CONTAINER_OF(task_struct_t, send_tag, src_node);
    }
    else
    {
        sender = pid_to_task(src);
        spinlock_lock(&receiver->send_lock);
        list_remove(&sender->send_tag);
        spinlock_unlock(&receiver->send_lock);
    }
    memcpy(msg, &sender->msg, sizeof(message_t));
    receiver->recv_from = MAX_TASK;
    inform_received(sender->pid);
    return SYSCALL_SUCCESS;
}

PUBLIC int task_ipc_check(pid_t pid)
{
    task_struct_t *task = pid_to_task(pid);

    if (atomic_read(&task->recv_flag) == 1)
    {
        // 没有收到任何消息
        if (!received_any_message(task->pid))
        {
            return FALSE;
        }
        pid_t recv_from = task->recv_from;
        if (recv_from != RECV_FROM_ANY)
        {
            if (recv_from == RECV_FROM_INT && !task->has_intr_msg)
            {
                return FALSE;
            }
            if (!received_from(task->pid, recv_from))
            {
                return FALSE;
            }
        }
        atomic_set(&task->recv_flag, 0);
        return TRUE;
    }

    // send_flag用于标记消息发送的状态
    // 消息被发出后,send_flag++
    // 消息被收到后,send_flag--
    // send_flag > 0 消息已发出,但未被收到 - 唤醒接收者并等待消息被收到(不可运行)
    // send_flag = 0 没有发送消息或消息已发出且被收到 - 可运行
    // send_flag < 0 消息已发出,在send_flag++前已被收到(小概率) - 可运行

    /// NOTE: 导致 send_flag < 0的情况:
    // sender                       | receiver
    // -----------------------------|----------------------------------
    // wait_receviced()             |
    //  +->append(list, send_tag);  | while (!received_from(src))
    //                              |  +-> received_from()
    //                              |       +-> list_find(list,send_tag)
    //                              |           ^ false
    //                              | ...
    // ---------------------------- | inform_received()
    // [interrupt]                  |  +-> dec(send_flag);
    //                              |                |
    // schedule()                   |                |
    //  +-> task_ipc_check()        |                |
    //       +-> read(send_flag)    |   <------------+
    //           ^ send_flag < 0    |
    //                              |
    // ---------------------------- |
    //                              |
    // wait_received()              |
    //  +->inc(send_flag);          |
    //         ^ send_flag = 0      |

    if ((int64_t)atomic_read(&task->send_flag) > 0)
    {
        return FALSE;
    }
    return TRUE;
}
