// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_IPC_H__
#define __SYSCALL_IPC_H__

#define IPC_NOWAIT (1 << 0) // 没有消息时立即返回-EAGAIN
#define IPC_PEEK   (1 << 1) // 缓冲区太小则回填所需大小并返回,未设置则丢弃消息

struct list_node;
struct mailbox;
struct msg_head;
struct task;

struct mailbox *send_node_to_mailbox(struct list_node *node);
struct task    *mailbox_to_task(struct mailbox *mailbox);

int ipc_recv_wakeup_condition(void *arg);
int ipc_send_wakeup_condition(void *arg);

void init_mailbox(struct mailbox *mailbox);
void mailbox_cleanup(struct task *task);

int ipc_send(struct msg_head *msg, pid_t dst_pid);
int ipc_recv(struct msg_head *msg, int option);

#endif /* __SYSCALL_IPC_H__ */
