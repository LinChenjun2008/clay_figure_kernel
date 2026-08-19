// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define NR_EXIT 0
#define NR_FORK 1
#define NR_EXEC 2
#define NR_WAIT 3
#define NR_SEND 4
#define NR_RECV 5
#define NR_BOTH 6
#define NR_MMAP 7
#define NR_UMAP 8

#define NR_CONT 9

#ifndef __ASSEMBLER__

#    include <task/struct.h>

void syscall_init(void);
void syscall_enable(void);
void register_syscall(uint64_t num, void *func);

// ipc.c
void init_mailbox(struct mailbox *mailbox);
void inform_event(pid_t pid, uint32_t evt_type);
int  msg_send(pid_t dst, struct message *msg);
int  msg_recv(pid_t from, struct message *msg);
int  msg_both(pid_t src_dst, struct message *msg);

#endif /* __ASSEMBLER__ */

#endif /* __SYSCALL_H__ */