/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

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
    (void)irq;
#ifndef __PIC_8259A__
    local_apic_write(APIC_REG_EOI, 0);
#else
    io_out8(PIC_M_CTRL, 0x20);
#endif
}