// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/syscall.h>

#include <syscall.h>

void *syscall_table[NR_CONT];

void syscall_init(void)
{
    int i;
    for (i = 0; i < NR_CONT; i++)
    {
        syscall_table[i] = NULL;
    }
    return;
}

void syscall_enable(void)
{
    arch_syscall_enable();
    return;
}