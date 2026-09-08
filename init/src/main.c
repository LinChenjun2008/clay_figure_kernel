// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <lib.h>

int main(void);
int main(void)
{
    while (1)
    {
        waitpid(-1, (void *)0, 1);
    }
    return 0;
}