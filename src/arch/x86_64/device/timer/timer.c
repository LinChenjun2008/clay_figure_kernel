/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <io.h>             // io_out8
#include <intr.h>           // register_handle
#include <device/pic.h>     // eoi
#include <device/cpu.h>     // make_icr,send_IPI
#include <task/task.h>      // do_schedule
#include <kernel/syscall.h> // inform_intr
#include <device/timer.h>   // COUNTER0_VALUE_LO,COUNTER0_VALUE_HI,
                            // IRQ0_FREQUENCY

#include <log.h>

PUBLIC volatile uint64_t global_ticks;

PRIVATE void pit_timer_handler()
{
    eoi(0x20);
    inform_intr(TICK);
    global_ticks++;
#ifdef __DISABLE_APIC_TIMER__
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
#endif
    return;
}

PRIVATE void apic_timer_handler()
{
    eoi(0x80);
    schedule();
    return;
}

PUBLIC void pit_init()
{
    global_ticks = 0;
    register_handle(0x20,pit_timer_handler);
    register_handle(0x80,apic_timer_handler);
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
    return;
}

PUBLIC void apic_timer_init()
{
#ifndef __DISABLE_APIC_TIMER__
    local_apic_write(APIC_REG_TPR,0); // Set TPR
    local_apic_write(APIC_REG_TIMER_DIV,0x00000003);

    // map APIC timer to an interrupt, and by that enable it in one-shot mode
    // Interrupt 0x27 is ignored vector.
    local_apic_write(APIC_REG_LVT_TIMER,0x27);
    local_apic_write(APIC_REG_TIMER_ICNT,0xffffffff);
    uint32_t apic_ticks = 0;
    volatile uint32_t ticks = global_ticks + (1 << 7);
    while(ticks >= global_ticks) continue;

    // Stop APIC timer
    local_apic_write(APIC_REG_LVT_TIMER,0x10000);
    apic_ticks = -local_apic_read(APIC_REG_TIMER_CCNT);

    // 1000 Hz
    apic_ticks >>= 7;

    local_apic_write(APIC_REG_LVT_TIMER,0x80 | 0x20000);
    local_apic_write(APIC_REG_TIMER_DIV,0x00000003);
    local_apic_write(APIC_REG_TIMER_ICNT,apic_ticks);
#endif
    return;
}