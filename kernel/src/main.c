// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/init.h>

// TEST
#include <asm/drivers/apic.h>

#include <print.h>
#include <task.h>

void        syscall(int, int);
static void proc()
{
    syscall(0, 1234);
    while (1);
}

int main(struct boot_info *boot_info)
{
    init_all(boot_info);
    process_execute("process", DEFAULT_PRIO, 1, 1, proc);
    while (1);
    return 0;
}

int ap_main(uintptr_t stack)
{
    ap_init_all(stack);
    while (1);
}