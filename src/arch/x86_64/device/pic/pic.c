// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/pic.h> // apic_t
#include <io.h>         // io_out8

extern apic_t apic;

PUBLIC void pic_init(void)
{
#ifndef __PIC_8259A__
    apic_init();
#else
    init_8259a();
#endif
    return;
}

PUBLIC void send_eoi(uint8_t irq)
{
    UNUSED(irq);
#ifndef __PIC_8259A__
    local_apic_write(APIC_REG_EOI, 0);
#else
    io_out8(PIC_M_CTRL, 0x20);
#endif
}