// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIB_H__
#define __LIB_H__

#include <task/struct.h> // pid_t

// syscall
uint64_t syscall_0(uint64_t);
uint64_t syscall_1(uint64_t, uint64_t);
uint64_t syscall_2(uint64_t, uint64_t, uint64_t);
uint64_t syscall_3(uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t syscall_4(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t syscall_5(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

void  exit(int status);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait(pid_t pid, int *status);

#endif /* __LIB_H__ */