// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_IPC_H__
#define __SYSCALL_IPC_H__

#include <task/struct.h>

struct mailbox *send_node_to_mailbox(struct list_node *node);
struct task    *mailbox_to_task(struct mailbox *mailbox);
struct mailbox *recv_node_to_mailbox(struct list_node *node);
int             msg_match(pid_t from, pid_t dst_pid, pid_t src_pid);

void init_mailbox(struct mailbox *mailbox);
void mailbox_cleanup(struct task *task);
int  msg_both(pid_t src_dst, struct message *msg);

int  msg_send(pid_t dst_pid, struct message *msg);
void inform_event(pid_t dst_pid, uint32_t evt_type);

int msg_recv(pid_t src_pid, struct message *msg);

#endif /* __SYSCALL_IPC_H__ */
