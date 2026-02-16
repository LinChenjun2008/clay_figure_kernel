// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/syscall.h>

#include <syscall.h>

void syscall_init(void)
{
    arch_syscall_init();
    return;
}