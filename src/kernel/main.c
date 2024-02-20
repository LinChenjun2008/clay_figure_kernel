#include <kernel/global.h>
#include <common.h>
#include <kernel/init.h>
#include <intr.h>
#include <task/task.h>
#include <kernel/syscall.h>

#include <log.h>

boot_info_t *g_boot_info = (boot_info_t*)0xffff800000310000;

void ktask()
{
    uint32_t color = 0x00000000;
    while(1)
    {
        uint32_t x,y;
        for(y = 10;y < 20;y++)
        {
            uint32_t *buffer = \
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                + g_boot_info->graph_info.horizontal_resolution * y;
            for (x = 0;x < 10;x++)
            {
                *(buffer + x) = color++;
            }
        }
        __asm__ ("hlt");
    };
}

void ktask2()
{
    uint32_t color = 0x00000000;
    while(1)
    {
        uint32_t x,y;
        for(y = 20;y < 30;y++)
        {
            uint32_t *buffer = \
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                + g_boot_info->graph_info.horizontal_resolution * y;
            for (x = 0;x < 10;x++)
            {
                *(buffer + x) = color++;
            }
        }
    };
}

void kernel_main()
{
    pr_log("\1Kernel initializing.\n");
    init_all();
    pr_log("\1Kernel initializing done.\n");
    task_start("k task",3,65536,ktask,0);
    prog_execute(ktask2,"k task 2",65536);
    uint32_t color = 0;
    while(1)
    {
        uint32_t x,y;
        for(y = 0;y < 10;y++)
        {
            uint32_t *buffer = \
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                + g_boot_info->graph_info.horizontal_resolution * y;
            for (x = 0;x < 10;x++)
            {
                *(buffer + x) = color++;
            }
        }
        __asm__ ("hlt");
    };
}