/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/pic.h> // apic_t
#include <io.h>         // io_out8

#include <log.h>

PUBLIC void init_8259a();
PUBLIC void apic_init();

extern apic_t apic;

PUBLIC void pic_init()
{
#ifndef __PIC_8259A__
    apic_init();
#else
    init_8259a();
#endif
    return;
}

PUBLIC void eoi(uint8_t irq)
{
    (void)irq;
#ifndef __PIC_8259A__
    local_apic_write(0x0b0,0);
#else
    io_out8(PIC_M_CTRL,0x20);
#endif
}