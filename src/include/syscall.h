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

void syscall_init(void);

#endif /* __SYSCALL_H__ */