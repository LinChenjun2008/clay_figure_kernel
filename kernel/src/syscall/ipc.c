// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <errno.h>
#include <panic.h>
#include <std/string.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/schedule.h>
#include <task/struct.h>

// 检查message结构是否正常设置
/// TODO: 检查msg所在页面是否可读写.
int check_message(struct task *task, struct msg_head *msg)
{
    // 1. msg必须在用户空间
    if (!USER_VMA_SPACE(msg))
    {
        return -EINVAL;
    }
    // 2. msg必须已映射(防止懒分配机制导致读取到脏数据)
    phys_addr_t msg_phys = to_physical_address(task->pg_dir, (uintptr_t)msg);
    if (msg_phys == 0)
    {
        return -EFAULT;
    }
    // 从此开始可以访问msg内部字段.
    // 3. 字段正确性保证
    if (msg->legnth > MAX_MESSAGE_LEGNTH)
    {
        return -EINVAL;
    }
    if (msg->header_legnth > msg->legnth)
    {
        return -EINVAL;
    }
    return 0;
}

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

int ipc_match(pid_t from, pid_t dst_pid, pid_t src_pid)
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
    ASSERT(dest_task != NULL && src_task != NULL);

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
    return ipc_match(pack->from, pack->dst_pid, src_task->pid);
}

// wait_event 条件: 有可接收的匹配消息/事件, 或来源已退出
int ipc_recv_wakeup_condition(void *arg)
{
    struct ipc_recv_pack *pack  = arg;
    struct mailbox       *dst   = &pack->task->mailbox;
    int                   ready = 0;

    spin_lock(&dst->send_lock);

    // // 事件(与 ipc_event_lock 的匹配规则一致)
    // if (pack->from == PID_ANY || pack->from == PID_EVENT)
    // {
    //     int i;
    //     for (i = 0; i < EVT_NR; i++)
    //     {
    //         if (dst->evt_msg[i] != 0)
    //         {
    //             ready = 1;
    //             break;
    //         }
    //     }
    // }
    // // 其他任务发来的消息
    // if (!ready)
    // {
    struct check_send_list_pack check;
    check.dst_pid = pack->task->pid;
    check.from    = pack->from;
    ready = list_traversal(&dst->send_list, check_send_list, &check) != NULL;
    // }

    spin_unlock(&dst->send_lock);

    if (dst->recv_err != 0) // 接收出错
    {
        ready = 1;
    }
    return ready;
}

// wait_event 条件: 本任务发出的消息已被接收, 或接收者已退出
int ipc_send_wakeup_condition(void *arg)
{
    struct ipc_send_pack *pack = arg;
    if (pack->task->mailbox.send_err != 0)
    {
        return 1;
    }
    return pack->task->mailbox.send_to != pack->dst_pid;
}

void init_mailbox(struct mailbox *mailbox)
{
    memset(&mailbox->msg, 0, sizeof(mailbox->msg));
    memset(&mailbox->evt_msg, 0, sizeof(mailbox->evt_msg));
    mailbox->send_to   = PID_NULL;
    mailbox->recv_from = PID_NULL;
    mailbox->closed    = 0;
    mailbox->send_err  = 0;
    mailbox->recv_err  = 0;
    init_list(&mailbox->send_list);
    init_spinlock(&mailbox->send_lock);
    init_list(&mailbox->recv_list);
    init_spinlock(&mailbox->recv_lock);
    init_wait_queue(&mailbox->recv_wq, ipc_recv_wakeup_condition);
    init_wait_queue(&mailbox->send_wq, ipc_send_wakeup_condition);
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
        dst_task->mailbox.recv_err = -ESRCH;
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
