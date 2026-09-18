// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define NR_EXIT 0 // exit
#define NR_FORK 1 // fork
#define NR_EXEC 2 // execve (reserved)
#define NR_WAIT 3 // waitpid
#define NR_SEND 4 // send a message
#define NR_RECV 5 // receive a message
#define NR_ADDR 6 // allocate a page
#define NR_FREE 7 // free a page

#define NR_KILL 8  // kill
#define NR_SACT 9  // signal action
#define NR_SRET 10 // signal return

#define NR_CONT 11

#ifndef __ASSEMBLER__

void syscall_init(void);
void syscall_enable(void);
void register_syscall(uint64_t num, void *func);

#endif /* __ASSEMBLER__ */

#endif /* __SYSCALL_H__ */