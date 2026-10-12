// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <user/lib.h>

int main(void)
{
    while (1)
    {
        int   status = 0;
        pid_t pid    = wait(-1, &status);
        if (pid >= 0)
        {
            dprintf("init: exit{ pid=%d, status=%d }\n", pid, status);
        }
    }
    return 0;
}