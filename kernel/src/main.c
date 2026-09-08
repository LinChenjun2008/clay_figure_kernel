// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/init.h>
#include <asm/task.h>

int main(struct system_info *system_info)
{
    init_all(system_info);

    while (1)
    {
        idle_task();
    }
    return 0;
}

int ap_main(struct system_info *system_info, uintptr_t stack)
{
    ap_init_all(system_info, stack);

    while (1)
    {
        idle_task();
    }
    return 0;
}