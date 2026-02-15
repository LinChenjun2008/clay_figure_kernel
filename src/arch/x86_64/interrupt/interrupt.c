// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/interrupt.h>
#include <asm/utils.h>

intr_status_t intr_get_status(void)
{
    uint64_t flags = get_flags();
    return (flags & 0x00000200) ? INTR_ON : INTR_OFF;
}

intr_status_t intr_enable(void)
{
    intr_status_t intr_status;
    intr_status = intr_get_status();
    if (intr_status == INTR_OFF)
    {
        io_sti();
    }
    return intr_status;
}

intr_status_t intr_disable(void)
{
    intr_status_t intr_status;
    intr_status = intr_get_status();
    if (intr_status == INTR_ON)
    {
        io_cli();
    }
    return intr_status;
}

intr_status_t intr_set_status(intr_status_t status)
{
    return (status == INTR_ON ? intr_enable() : intr_disable());
}

void intr_init(void)
{
    intr_handler_init();
    return;
}