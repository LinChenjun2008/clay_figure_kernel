// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/page.h>

#include <errno.h>
#include <mem.h>
#include <std/string.h>
#include <syscall/ipc.h>
#include <task.h>
#include <task/schedule.h>
#include <task/struct.h>

// 取得src_task地址空间中addr处msg_head的可访问指针
static struct msg_head *msg_from_other(struct task *src_task, uintptr_t addr)
{
    if (IS_KERNEL_TASK(src_task))
    {
        return (struct msg_head *)addr;
    }
    phys_addr_t src_phys = to_physical_address(src_task->pg_dir, addr);
    if (src_phys == 0)
    {
        return NULL;
    }
    return (struct msg_head *)PHYS_TO_VIRT(src_phys);
}

// 将用户进程的地址转为内核空间的地址
static void *to_kernel_address(struct task *src_task, uintptr_t addr)
{
    if (IS_KERNEL_TASK(src_task))
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

static int copy_data(void *dst, struct task *src_task, void *src, size_t size)
{
    size_t done = 0;
    while (done < size)
    {
        uintptr_t src_addr = (uintptr_t)src + done;
        size_t chunk = MIN(size - done, PG_SIZE - (src_addr & (PG_SIZE - 1)));

        void *src_page = to_kernel_address(src_task, src_addr);
        if (src_page == NULL)
        {
            return -EFAULT;
        }
        memcpy((uint8_t *)dst + done, src_page, chunk);
        done += chunk;
    }
    return 0;
}

static int
copy_from_other(struct msg_head *dst_msg, uintptr_t src, struct task *src_task)
{
    struct msg_head *src_msg = msg_from_other(src_task, src);
    if (src_msg == NULL)
    {
        return -EFAULT;
    }

    // 接收方在接收前设置的落地缓冲区及其容量, 不能被发送方的消息覆盖
    void  *dst_data     = dst_msg->data;
    size_t dst_capacity = dst_msg->size;

    if (src_msg->size > dst_capacity)
    {
        // 接收方缓冲区不够: 回写自身容量, 供发送方重整后重试
        src_msg->size = dst_capacity;
        return -E2BIG;
    }

    *dst_msg      = *src_msg;
    dst_msg->data = dst_data;

    // 把发送方的数据复制到data指针处
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
static struct mailbox *
ipc_get_msg(struct task *dst_task, struct msg_head **msg_from_src)
{
    struct list_node *node = list_pop(&dst_task->mailbox.send_list);

    if (node == NULL)
    {
        return NULL;
    }
    struct mailbox *src = send_node_to_mailbox(node);

    *msg_from_src = src->msg;
    return src;
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

    status = check_message(dst_task, msg);
    if (status < 0)
    {
        return status;
    }
    // 接收方的数据区只要求地址已分配(未映射页由内核态缺页处理自动补页)
    if (IS_USER_TASK(dst_task))
    {
        status = mm_check_addr(dst_task, msg->data, msg->size, ADDR_ALLOCATED);
        if (status < 0)
        {
            return status;
        }
    }

    while (1)
    {
        struct mailbox  *src      = NULL;
        struct task     *src_task = NULL;
        struct msg_head *src_msg  = NULL;

        // 1. 锁内摘取队首消息
        spin_lock(&dst->send_lock);
        src = ipc_get_msg(dst_task, &src_msg);
        spin_unlock(&dst->send_lock);

        if (src != NULL)
        {
            // 2. 锁外拷贝
            src_task    = mailbox_to_task(src);
            recv_status = copy_from_other(msg, (uintptr_t)src_msg, src_task);
            // 3. 锁内回写结果
            spin_lock(&dst->send_lock);
            ipc_recv_inform(src, recv_status);
            spin_unlock(&dst->send_lock);
        }
        else
        {
            recv_status = -ENOENT;
        }

        wake_up(&dst->send_wq);

        // 根据status判断接收情况

        if (recv_status == 0)
        {
            // 顺利接收
            status = 0;
            break;
        }
        else if (recv_status == -E2BIG || recv_status == -EFAULT)
        {
            // 发送方数据区/容量问题: 丢弃该消息继续等待
            continue;
        }
        else if (recv_status == -ENOENT)
        {
            // 没有消息
            if (option & IPC_NOWAIT)
            {
                status = -EAGAIN;
                break;
            }
        }
        else
        {
            // 其他错误
            status = recv_status;
            break;
        }

        // 被信号或其他原因打断
        if (wake_status != 0)
        {
            status = wake_status;
            break;
        }
        wake_status = wait_event(&dst->recv_wq, dst, TASK_RECEIVE);
    }
    return status;
}
