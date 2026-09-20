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
/// TODO: 检查msg所在页面是否可读写(目前暂无只读/只写页面).
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
    if (msg->legnth == 0 || msg->header_legnth == 0)
    {
        return -EINVAL;
    }
    if (msg->legnth > MAX_MESSAGE_LEGNTH)
    {
        return -EINVAL;
    }
    if (msg->header_legnth > msg->legnth)
    {
        return -EINVAL;
    }
    if (msg->header_legnth < sizeof(*msg))
    {
        return -EINVAL;
    }
    // msg必须在同一页面
    uintptr_t start = (uintptr_t)msg;
    uintptr_t end   = start + msg->legnth - 1;

    // 用PFN辅助判断
    if (ADDR_TO_PFN(start) != ADDR_TO_PFN(end))
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

// wait_event 条件: 有可接收的消息
// 这里不加锁读链表: 发送方先把自己挂进send_list再唤醒, 且等待者被唤醒后
// 会在send_lock内重新确认, 所以读到旧值最多多循环一次
int ipc_recv_wakeup_condition(void *arg)
{
    struct mailbox *dst = arg;

    return !list_empty(&dst->send_list);
}

// wait_event 条件: 本任务发出的消息已被接收, 或接收者已退出
// 根据send_status判断:
//      send_status > 0 发送中
//      send_status = 0 完成
//      send_status < 0 错误码
int ipc_send_wakeup_condition(void *arg)
{
    struct mailbox *src = arg;

    int status = src->send_status;
    return status <= 0;
}

void init_mailbox(struct mailbox *mailbox)
{
    memset(&mailbox->msg, 0, sizeof(mailbox->msg));
    mailbox->send_to     = PID_NULL;
    mailbox->closed      = 0;
    mailbox->send_status = 0;
    init_list(&mailbox->send_list);
    init_spinlock(&mailbox->send_lock);
    init_wait_queue(&mailbox->recv_wq, ipc_recv_wakeup_condition);
    init_wait_queue(&mailbox->send_wq, ipc_send_wakeup_condition);
    return;
}

static int mailbox_cleanup_send(struct list_node *node, void *arg)
{
    (void)arg;
    struct mailbox *src = send_node_to_mailbox(node);
    src->send_status    = -ESRCH;
    return 0;
}

// 回收本任务发出去、目标还没接收的消息.
// 同一时刻一个任务最多只有一条在途消息(ipc_send是同步的), 所以按send_to回收一条即可.
// 不回收的话, 发送方退出后目标仍可能去读它的地址空间(消息正文在发送方的用户态).
// 调用时不能持有cur的邮箱锁: 这里要拿目标邮箱的锁.
static void mailbox_send_reclaim(struct mailbox *cur)
{
    pid_t dst_pid = cur->send_to;

    if (dst_pid < 0)
    {
        // 没有在途消息
        return;
    }
    struct task *dst_task = pid_to_task(dst_pid);
    if (dst_task == NULL)
    {
        // 目标已经退出, 它的邮箱随之失效
        return;
    }
    struct mailbox *dst = &dst_task->mailbox;

    spin_lock(&dst->send_lock);
    // 消息可能已经被目标取走了
    if (list_find(&dst->send_list, &cur->send_node))
    {
        list_remove(&cur->send_node);
    }
    spin_unlock(&dst->send_lock);
    return;
}

void mailbox_cleanup(struct task *task)
{
    struct mailbox *cur = &task->mailbox;

    // 回收自己发出去但还没被接收的消息
    mailbox_send_reclaim(cur);

    spin_lock(&cur->send_lock);
    // 关闭邮箱,并移除所有正在发送的任务
    // 然后设置对方的send_status为异常值.
    cur->closed = 1;
    list_traversal(&cur->send_list, mailbox_cleanup_send, NULL);
    spin_unlock(&cur->send_lock);

    wake_up(&cur->send_wq);

    // 等待对方自己出列
    while (1)
    {
        spin_lock(&cur->send_lock);
        int len = list_len(&cur->send_list);
        spin_unlock(&cur->send_lock);
        if (len == 0)
        {
            break;
        }
        task_yield();
    }
    return;
}
