// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/syscall.h>

#include <syscall.h>

// syscall functions
#include <task.h>

void *syscall_table[NR_CONT];

void syscall_init(void)
{
    int i;
    for (i = 0; i < NR_CONT; i++)
    {
        syscall_table[i] = NULL;
    }
    register_syscall(NR_EXIT, process_exit);
    register_syscall(NR_WAIT, task_waitpid);
    register_syscall(NR_SEND, msg_send);
    register_syscall(NR_RECV, msg_recv);
    register_syscall(NR_BOTH, msg_both);
    return;
}

void syscall_enable(void)
{
    arch_syscall_enable();
    return;
}

void register_syscall(uint64_t num, void *func)
{
    if (num >= NR_CONT)
    {
        return;
    }
    syscall_table[num] = func;
    return;
}