#include <kernel/global.h>
#include <common.h>
#include <kernel/init.h>
#include <intr.h>
#include <log.h>

boot_info_t *g_boot_info = (boot_info_t*)0xffff800000310000;

void kernel_main()
{
    intr_disable();
    init_all();
    intr_enable();
    uint32_t color = 0x00207090;
    uint32_t x,y;
    for(y = 0;y < 30;y++)
    {
        uint32_t *buffer = \
            (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
            + g_boot_info->graph_info.horizontal_resolution * y;
        for (x = 0;x < g_boot_info->graph_info.horizontal_resolution;x++)
        {
            *(buffer + x) = color;
        }
    }
    while(1);
}