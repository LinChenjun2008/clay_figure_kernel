// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/timer.h>

#include <drivers/timer.h>

int run_timeout(int (*func)(void *), void *arg, uint64_t timeout)
{
    uint64_t timeout_ticks = get_ticks() + timeout;
    while (get_ticks() < timeout_ticks)
    {
        if (func(arg) == 0)
        {
            return 0;
        }
    }
    return 1;
}