#include <kernel/global.h>
#include <common.h>
#include <kernel/init.h>
#include <intr.h>
#include <log.h>
#include <task/task.h>

boot_info_t *g_boot_info = (boot_info_t*)0xffff800000310000;

void ktask()
{
    uint32_t color = 0x00000000;
    pr_log("\1I am \"%s\",my pid is %d. My kstack is: %p\n",running_task()->name,running_task()->pid,running_task()->kstack_base);
    pr_log("\1pid2task(%d)->name is \"%s\".\n",running_task()->pid,pid2task(running_task()->pid)->name);
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
    };
}

void ktask2()
{
    uint32_t color = 0x00000000;
    pr_log("\1I am \"%s\",my pid is %d. My kstack is: %p\n",running_task()->name,running_task()->pid,running_task()->kstack_base);
    pr_log("\1pid2task(%d)->name is \"%s\".\n",running_task()->pid,pid2task(running_task()->pid)->name);
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
    intr_disable();
    pr_log("\1Kernel initializing.\n");
    init_all();
    intr_enable();
    pr_log("\1Kernel initializing done.\n");
    task_start("k task",31,65536,ktask,0);
    task_start("k task2",31,65536,ktask2,0);
    uint32_t color = 0x0fffffff;
    while(color--);
    pr_log("\1I am \"%s\",my pid is %d. My kstack is: %p\n",running_task()->name,running_task()->pid,running_task()->kstack_base);
    pr_log("\1pid2task(%d)->name is \"%s\".\n",running_task()->pid,pid2task(running_task()->pid)->name);
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
    };
}