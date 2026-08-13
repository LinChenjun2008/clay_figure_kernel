// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/drivers/pic.h>

void pic_init(struct boot_info *boot_info)
{
    apic_init(boot_info);
    return;
}

void send_eoi()
{
    apic_send_eoi();
    return;
}