// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <syscall.h>
#include <user/lib.h>

void lib_init(void)
{
    lib_allocator_init();
    return;
}

void exit(int status)
{
    syscall_1(NR_EXIT, status);
    return;
}

pid_t fork(void)
{
    pid_t ret = (pid_t)syscall_0(NR_FORK);
    return ret;
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    pid_t ret = (pid_t)syscall_3(NR_WAIT, pid, (word_t)status, options);
    return ret;
}

pid_t wait(pid_t pid, int *status)
{
    pid_t ret = waitpid(pid, status, 0);
    return ret;
}

int send(struct msg_head *msg, pid_t dst)
{
    return (int)syscall_2(NR_SEND, (word_t)msg, dst);
}

int recv(struct msg_head *msg, int option)
{
    return (int)syscall_2(NR_RECV, (word_t)msg, (word_t)option);
}

void *allocate_pages(void *addr, size_t pages)
{
    return (void *)syscall_2(NR_ADDR, (word_t)addr, pages);
}

void free_pages(void *addr, size_t pages)
{
    syscall_2(NR_FREE, (word_t)addr, pages);
    return;
}

int kill(pid_t pid, int sig)
{
    return (int)syscall_2(NR_KILL, pid, sig);
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact)
{
    return (int)syscall_3(NR_SACT, sig, (word_t)act, (word_t)oldact);
}