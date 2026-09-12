// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <lib.h>
#include <syscall.h>

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
    pid_t ret = (pid_t)syscall_3(NR_WAIT, pid, (uint64_t)status, options);
    return ret;
}

pid_t wait(pid_t pid, int *status)
{
    pid_t ret = waitpid(pid, status, 0);
    return ret;
}

pid_t send(pid_t dst, struct message *msg)
{
    return (pid_t)syscall_2(NR_SEND, dst, (uint64_t)msg);
}

pid_t recv(pid_t src, struct message *msg)
{
    return (pid_t)syscall_2(NR_RECV, src, (uint64_t)msg);
}

pid_t both(pid_t src_dst, struct message *msg)
{
    return (pid_t)syscall_2(NR_BOTH, src_dst, (uint64_t)msg);
}

int kill(pid_t pid, int sig)
{
    return (int)syscall_2(NR_KILL, pid, sig);
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact)
{
    return (int)syscall_3(NR_SACT, sig, (uint64_t)act, (uint64_t)oldact);
}