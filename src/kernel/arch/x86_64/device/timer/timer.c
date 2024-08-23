#include <kernel/global.h>
#include <io.h>
#include <intr.h>
#include <device/pic.h>
#include <device/cpu.h>
#include <task/task.h>
#include <kernel/syscall.h>
#include <device/timer.h>

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

    do_schedule();
    return;
}

PUBLIC void pit_init()
{
    global_ticks = 0;
    register_handle(0x20,irq_timer_handler);
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