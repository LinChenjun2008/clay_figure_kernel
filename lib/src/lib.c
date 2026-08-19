// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib.h>
#include <syscall.h>

void exit(int status)
{
    syscall_1(NR_EXIT, status);
    return;
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