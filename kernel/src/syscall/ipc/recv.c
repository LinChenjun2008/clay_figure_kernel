// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <errno.h>
#include <lib/string.h>
#include <panic.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/schedule.h>
#include <task/struct.h>

// 把发送方地址空间中addr处的地址换为内核可直接访问的地址(按页转换)
static void *to_kernel_address(struct task *src_task, uintptr_t addr)
{
    if (IS_KERN_TASK(src_task))
    {
        return (void *)addr;
    }
    uintptr_t   page = addr & ~(PG_SIZE - 1);
    phys_addr_t phys = to_physical_address(src_task->pg_dir, page);
    if (phys == 0)
    {
        return NULL;
    }
    return (void *)((uintptr_t)PHYS_TO_VIRT(phys) + (addr - page));
}

// 把发送方的数据按页复制到接收方的缓冲区
static int copy_data(void *dst, struct task *src_task, void *src, size_t size)
{
    size_t done = 0;
    while (done < size)
    {
        uintptr_t src_addr = (uintptr_t)src + done;
        size_t chunk = MIN(size - done, PG_SIZE - (src_addr & (PG_SIZE - 1)));
        void  *src_page = to_kernel_address(src_task, src_addr);

        if (src_page == NULL)
        {
            return -EFAULT;
        }
        memcpy((uint8_t *)dst + done, src_page, chunk);
        done += chunk;
    }
    return 0;
}

// 把发送方的消息复制到接收方的落地缓冲区
static int copy_from_other(
    struct msg_head *dst_msg,
    struct msg_head *src_msg,
    struct task     *src_task
)
{
    // 接收方在接收前设置的落地缓冲区及其容量, 不能被发送方的消息覆盖
    void  *dst_data     = dst_msg->data;
    size_t dst_capacity = dst_msg->size;

    if (src_msg->size > dst_capacity)
    {
        return -E2BIG;
    }

    *dst_msg      = *src_msg;
    dst_msg->data = dst_data;

    // 发送方的数据区属于发送方的地址空间, 按页转换后复制
    int ret = copy_data(dst_data, src_task, src_msg->data, src_msg->size);
    if (ret < 0)
    {
        dst_msg->size = 0;
        return ret;
    }
    dst_msg->size = src_msg->size; // 实际收到的数据大小
    return 0;
}

// 从自己的等待队列摘取队首消息
static struct mailbox *ipc_recv_get_msg(struct task *dst_task)
{
    struct list_node *node = list_pop(&dst_task->mailbox.send_list);

    if (node == NULL)
    {
        return NULL;
    }
    struct mailbox *src = send_node_to_mailbox(node);
    return src;
}

// 把消息放回等待队列队首
static void ipc_recv_put_msg(struct task *dst_task, struct mailbox *src)
{
    list_push(&dst_task->mailbox.send_list, &src->send_node);
    return;
}

// 回写投递结果, 使发送方可以返回(调用时必须持dst->send_lock)
static void ipc_recv_inform(struct mailbox *src, int ret)
{
    src->send_status = ret;
    src->send_to     = PID_NULL;
    return;
}

int ipc_recv(struct msg_head *msg, int option)
{
    struct task    *dst_task    = get_current_task();
    struct mailbox *dst         = &dst_task->mailbox;
    int             status      = 0;
    int             recv_status = 0;
    int             wake_status = 0;

    while (1)
    {
        recv_status = 0;

        struct mailbox *src      = NULL;
        struct task    *src_task = NULL;

        // 取队首消息
        spin_lock(&dst->send_lock);
        src = ipc_recv_get_msg(dst_task);
        spin_unlock(&dst->send_lock);

        // 处理没有消息的情况
        if (src == NULL)
        {
            if (option & IPC_NOWAIT)
            {
                status = -EAGAIN;
                break;
            }
            // 等待收到消息
            wake_status = wait_event(&dst->recv_wq, dst, TASK_RECEIVE);
            // 被信号或其他原因打断
            if (wake_status != 0)
            {
                status = wake_status;
                break;
            }
            // 重新取消息
            continue;
        }

        // 读取消息
        src_task    = mailbox_to_task(src);
        recv_status = copy_from_other(msg, src->msg, src_task);

        if (recv_status == -E2BIG && (option & IPC_PEEK))
        {
            // 若需要PEEK则回写容量并放回消息
            void *origin_data = msg->data;
            *msg              = *src->msg;
            msg->data         = origin_data;

            spin_lock(&dst->send_lock);
            ipc_recv_put_msg(dst_task, src);
            spin_unlock(&dst->send_lock);
            status = -E2BIG;
            break;
        }

        // 消息已被成功取走,通知发送方
        spin_lock(&dst->send_lock);
        ipc_recv_inform(src, recv_status);
        spin_unlock(&dst->send_lock);

        wake_up(&dst->send_wq);

        // 根据status判断接收情况
        if (recv_status == -EFAULT || recv_status == -E2BIG)
        {
            // 发送方数据区问题: 丢弃该消息继续等待
            continue;
        }
        else
        {
            // 其他情况一律结束接收
            status = recv_status;
            break;
        }
    }
    return status;
}
