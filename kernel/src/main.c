// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/init.h>

#include <task.h>

int main(struct system_info *system_info)
{
    init_all(system_info->boot_info);

    while (1)
    {
        task_waitpid(-1, NULL, WNOHANG);
    }
    return 0;
}

int ap_main(uintptr_t stack)
{
    ap_init_all(stack);

    while (1)
    {
        task_waitpid(-1, NULL, WNOHANG);
    }
    return 0;
}