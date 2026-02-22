// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/syscall.h>

#include <syscall.h>

// TEST
#include <print.h>
#include <task.h>

void *syscall_table[NR_CONT];

static int syscall_test(void)
{
    static int   i    = 0;
    struct task *task = get_current_task();
    printk(
        MSG_INFO "%d: syscall test: name='%s',pid=%-2d, cpu=%d.\n",
        i++,
        task->name,
        task->pid,
        task->cpu->id
    );
    return 0;
}

void syscall_init(void)
{
    arch_syscall_init();
    int i;
    for (i = 0; i < NR_CONT; i++)
    {
        syscall_table[i] = NULL;
    }
    syscall_table[0] = syscall_test;
    return;
}

void ap_syscall_init(void)
{
    arch_syscall_init();
    return;
}