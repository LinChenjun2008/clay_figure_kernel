#include <kernel/global.h>
#include <common.h>
#include <kernel/init.h>
#include <intr.h>
#include <task/task.h>
#include <kernel/syscall.h>

#include <service.h>
#include <log.h>

#include <ulib.h>

boot_info_t *g_boot_info = (boot_info_t*)0xffff800000310000;

extern uint64_t global_ticks;

void ktask()
{
    uint32_t color = 0x00000000;
    uint32_t xsize = 20;
    uint32_t ysize = 20;
    uint32_t *buf = allocate_page(xsize * ysize * sizeof(uint32_t) / PG_SIZE + 1);
    while(1)
    {
        fill(buf,xsize * ysize * sizeof(uint32_t),xsize,ysize,0,0);
        color ++;
        uint32_t x,y;
        for(y = 0;y < ysize;y++)
        {
            for (x = 0;x < xsize;x++)
            {
                *(buf + y * xsize + x) = color;
            }
        }
    };
}

void kernel_main()
{
    pr_log("\1Kernel initializing.\n");
    init_all();
    pr_log("\1Kernel initializing done.\n");

    prog_execute("k task",DEFAULT_PRIORITY,65536,ktask);
    uint32_t color = 0;
    while(1)
    {
        uint32_t x,y;
        for(y = 0;y < 20;y++)
        {
            uint32_t *buffer = \
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                + g_boot_info->graph_info.horizontal_resolution * y;
            for (x = 20;x < 40;x++)
            {
                *(buffer + x) = color;
            }
        }
        color ++;
        __asm__ ("hlt");
    };
}