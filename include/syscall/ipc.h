// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_IPC_H__
#define __SYSCALL_IPC_H__

#include <task/struct.h>

void init_mailbox(struct mailbox *mailbox);
void mailbox_cleanup(struct task *task);
void inform_event(pid_t dst_pid, uint32_t evt_type);
int  msg_send(pid_t dst_pid, struct message *msg);
int  msg_recv(pid_t from, struct message *msg);
int  msg_both(pid_t src_dst, struct message *msg);

#endif /* __SYSCALL_IPC_H__ */
