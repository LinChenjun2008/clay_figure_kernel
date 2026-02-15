// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/driver/apic.h>
#include <asm/driver/pic.h>
#include <asm/interrupt.h>

void pic_init(boot_info_t *boot_info)
{
    apic_init(boot_info);
    return;
}

void send_eoi()
{
    apic_send_eoi();
    return;
}