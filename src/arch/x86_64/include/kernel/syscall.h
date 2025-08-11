// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define FUNC_SEND (1 << 0)
#define FUNC_RECV (1 << 1)

#define NR_SEND (0x80000000 | FUNC_SEND)
#define NR_RECV (0x80000000 | FUNC_RECV)
#define NR_BOTH (0x80000000 | FUNC_SEND | FUNC_RECV)

#define SEND_TO_KERNEL -1
#define RECV_FROM_INT  -2
#define RECV_FROM_ANY  -3

#define SYSCALL_SUCCESS        0
#define SYSCALL_ERROR          1
#define SYSCALL_NO_SYSCALL     2
#define SYSCALL_DEADLOCK       3
#define SYSCALL_DEST_NOT_EXIST 4
#define SYSCALL_SRC_NOT_EXIST  5

typedef struct message_s
{
    volatile pid_t    src;
    volatile uint32_t type;
    uint64_t          m[8];
} message_t;

typedef uint32_t syscall_status_t;

PUBLIC void syscall_init(void);

PUBLIC syscall_status_t ASMLINKAGE
send_recv(uint32_t function, pid_t src_dst, void *msg);

PUBLIC syscall_status_t ASMLINKAGE
sys_send_recv(uint32_t function, pid_t src_dst, message_t *msg);

/**
 * @brief 通知接收到中断消息
 * @param dst
 * @return
 */
PUBLIC void inform_intr(pid_t dst);

PUBLIC syscall_status_t msg_send(pid_t dst, message_t *msg);
PUBLIC syscall_status_t msg_recv(pid_t src, message_t *msg);

/**
 * @brief 检查task的ipc状态
 * @param pid
 * @return
 */
PUBLIC int task_ipc_check(pid_t pid);

#endif