// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <init/init.h>

int kernel_main(boot_info_t *boot_info);
int kernel_main(boot_info_t *boot_info)
{
    init_all(boot_info);
    while (1);
    return 0;
}
