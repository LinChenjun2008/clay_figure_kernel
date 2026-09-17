// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_IPC_H__
#define __SYSCALL_IPC_H__

struct list_node;
struct mailbox;
struct message;
struct task;

// 检查 send_list 中是否有匹配的消息来源
struct check_send_list_pack
{
    pid_t dst_pid;
    pid_t from;
};

struct mailbox *send_node_to_mailbox(struct list_node *node);
struct task    *mailbox_to_task(struct mailbox *mailbox);
struct mailbox *recv_node_to_mailbox(struct list_node *node);
int             ipc_match(pid_t from, pid_t dst_pid, pid_t src_pid);
int             check_send_list(struct list_node *node, void *arg);

struct ipc_recv_pack
{
    struct task *task;
    pid_t        from;
};
int ipc_recv_wakeup_condition(void *arg);

struct ipc_send_pack
{
    struct task *task;
    pid_t        dst_pid;
};
int ipc_send_wakeup_condition(void *arg);

void init_mailbox(struct mailbox *mailbox);
void mailbox_cleanup(struct task *task);

int ipc_send(pid_t dst_pid, struct message *msg);
// void inform_event(pid_t dst_pid, uint32_t evt_type);

int ipc_recv(pid_t src_pid, struct message *msg);

#endif /* __SYSCALL_IPC_H__ */
