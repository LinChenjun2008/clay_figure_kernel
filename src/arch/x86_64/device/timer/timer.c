/*
   Copyright 2024 LinChenjun

   本文件是Clay Figure Kernel的一部分。
   修改和/或再分发遵循GNU GPL version 3 (or any later version)
  
*/

#include <kernel/global.h>
#include <io.h>             // io_out8
#include <intr.h>           // register_handle
#include <device/pic.h>     // eoi
#include <device/cpu.h>     // make_icr,send_IPI
#include <task/task.h>      // do_schedule
#include <kernel/syscall.h> // inform_intr
#include <device/timer.h>   // COUNTER0_VALUE_LO,COUNTER0_VALUE_HI

PUBLIC volatile uint64_t global_ticks;

PRIVATE void irq_timer_handler()
{
    eoi(0x20);
    inform_intr(TICK);
    global_ticks++;
    // send IPI
    uint64_t icr;
    icr = make_icr(
        0x80,
        ICR_DELIVER_MODE_FIXED,
        ICR_DEST_MODE_PHY,
        ICR_DELIVER_STATUS_IDLE,
        ICR_LEVEL_DE_ASSEST,
        ICR_TRIGGER_EDGE,
        ICR_ALL_EXCLUDE_SELF,
        0);
    send_IPI(icr);

    schedule();
    return;
}

PUBLIC void pit_init()
{
    global_ticks = 0;
    register_handle(0x20,irq_timer_handler);
    ioapic_enable(2,0x20);
#if defined __TIMER_HPET__
    uint8_t *HPET_addr = (uint8_t *)0xfed00000;

    *(uint64_t*)(HPET_addr +  0x10) = 3;
    *(uint64_t*)(HPET_addr + 0x100) = 0x004c;
    *(uint64_t*)(HPET_addr + 0x108) = 1428571;
    *(uint64_t*)(HPET_addr + 0xf0) = 0;
#else
    io_out8(PIT_CTRL,0x34);
    io_out8(PIT_CNT0,COUNTER0_VALUE_LO);
    io_out8(PIT_CNT0,COUNTER0_VALUE_HI);
#endif
}