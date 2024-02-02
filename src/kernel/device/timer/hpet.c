#include <kernel/global.h>
#include <kernel/io.h>
#include <intr.h>
#include <device/pic.h>

#include <log.h>

PRIVATE void irq_HPETtimer_handler()
{
    eoi();
    // task_struct_t* cur_thread = running_thread();
    // cur_thread->elapsed_ticks++;
    // if (cur_thread->ticks == 0)
    // {
    //     schedule();
    // }
    // else
    // {
    //     cur_thread->ticks--;
    // }
    uint32_t x,y;
    for(y = 0;y < 30;y++)
    {
        uint32_t *buffer = \
            (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
            + g_boot_info->graph_info.horizontal_resolution * y;
        for (x = 0;x < g_boot_info->graph_info.horizontal_resolution;x++)
        {
            *(buffer + x) = *(buffer + x) + 1;
        }
    }
    return;
}

PUBLIC void pit_init()
{
    uint8_t *HPET_addr = (uint8_t *)0xfed00000;

    *(uint64_t*)(HPET_addr +  0x10) = 3;
    *(uint64_t*)(HPET_addr + 0x100) = 0x004c;
    // 1000000 / 69.841279 = 1000000 / 70 = 1ms
    *(uint64_t*)(HPET_addr + 0x108) = 1000000 / 70;
    *(uint64_t*)(HPET_addr + 0xf0) = 0;
    register_handle(0x20,irq_HPETtimer_handler);
}