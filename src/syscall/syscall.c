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
    printk(MSG_INFO "syscall test\n");
    return 0;
}

void syscall_init(void)
{
    int i;
    for (i = 0; i < NR_CONT; i++)
    {
        syscall_table[i] = NULL;
    }
    syscall_table[0] = syscall_test;
    return;
}

void syscall_enable(void)
{
    arch_syscall_enable();
    return;
}