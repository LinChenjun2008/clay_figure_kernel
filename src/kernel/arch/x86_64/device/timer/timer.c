#include <kernel/global.h>
#include <io.h>
#include <intr.h>
#include <device/pic.h>
#include <task/task.h>

PRIVATE void irq_timer_handler()
{
    eoi();
    task_struct_t *cur_task = running_task();
    cur_task->elapsed_ticks++;
    if (cur_task->ticks == 0)
    {
        schedule();
    }
    else
    {
        cur_task->ticks--;
    }
    return;
}

PUBLIC void pit_init()
{
    register_handle(0x20,irq_timer_handler);
    #if defined __TIMER_HPET__ && defined __TIMER_8254__
        "Only one timer can be selected."
    #elif defined __TIMER_HPET__
    uint8_t *HPET_addr = (uint8_t *)0xfed00000;

    *(uint64_t*)(HPET_addr +  0x10) = 3;
    *(uint64_t*)(HPET_addr + 0x100) = 0x004c;
    // 1000000 / 69.841279 = 1000000 / 70 = 1ms
    *(uint64_t*)(HPET_addr + 0x108) = 1000000 / 70;
    *(uint64_t*)(HPET_addr + 0xf0) = 0;
    #elif defined __TIMER_8254__
    io_out8(PIT_CTRL,0x34);
    io_out8(PIT_CNT0,0x9c);
    io_out8(PIT_CNT0,0x2e);
    #else
        "You must select a timer."
    #endif
}