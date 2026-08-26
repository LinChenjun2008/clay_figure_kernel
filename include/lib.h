// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIB_H__
#define __LIB_H__

#include <std/stdint.h>

typedef int32_t pid_t;

struct message
{
    int32_t  source;
    uint32_t type;
    union
    {
        uint32_t m32[14];
        uint64_t m64[7];
    };
};

// syscall
uint64_t syscall_0(uint64_t);
uint64_t syscall_1(uint64_t, uint64_t);
uint64_t syscall_2(uint64_t, uint64_t, uint64_t);
uint64_t syscall_3(uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t syscall_4(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t syscall_5(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

void  exit(int status);
pid_t fork(void);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait(pid_t pid, int *status);

pid_t send(pid_t dst, struct message *msg);
pid_t recv(pid_t src, struct message *msg);
pid_t both(pid_t src_dst, struct message *msg);

#endif /* __LIB_H__ */