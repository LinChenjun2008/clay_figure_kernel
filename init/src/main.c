// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <user/lib.h>

int main(void)
{
    while (1)
    {
        pid_t pid = wait(-1, NULL);
        if (pid >= 0)
        {
            dprintf("init: task exit with status:\n", pid);
        }
    }
    return 0;
}