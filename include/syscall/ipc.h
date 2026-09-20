// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_IPC_H__
#define __SYSCALL_IPC_H__

#define MAX_MESSAGE_LEGNTH PG_SIZE

// ipc_recv的option: 没有消息时立即返回-EAGAIN
#define IPC_NOWAIT 1

struct list_node;
struct mailbox;
struct msg_head;
struct task;

int             check_message(struct task *task, struct msg_head *msg);
struct mailbox *send_node_to_mailbox(struct list_node *node);
struct task    *mailbox_to_task(struct mailbox *mailbox);

int ipc_recv_wakeup_condition(void *arg);
int ipc_send_wakeup_condition(void *arg);

void init_mailbox(struct mailbox *mailbox);
void mailbox_cleanup(struct task *task);

int ipc_send(pid_t dst_pid, struct msg_head *msg);
// void inform_event(pid_t dst_pid, uint32_t evt_type);

int ipc_recv(struct msg_head *msg, int option);

#endif /* __SYSCALL_IPC_H__ */
