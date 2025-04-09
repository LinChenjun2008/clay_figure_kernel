/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <common.h>
#include <kernel/init.h>
#include <intr.h>
#include <task/task.h>
#include <kernel/syscall.h>
#include <device/cpu.h>
#include <service.h>
#include <std/stdio.h>
#include <io.h>

#include <log.h>

#include <ulib.h>

PUBLIC boot_info_t *g_boot_info = (boot_info_t*)0xffff800000310000;
PUBLIC graph_info_t *g_graph_info;
extern textbox_t g_tb;

PRIVATE void ktask(void)
{
    uint32_t color = 0x00000000;
    uint32_t xsize = 10;
    uint32_t ysize = 10;
    uint32_t *buf = allocate_page(xsize * ysize * sizeof(uint32_t) / PG_SIZE + 1);
    while (1)
    {
        fill(buf,xsize * ysize * sizeof(uint32_t),xsize,ysize,apic_id() * 10,0);
        color = color ? color << 8 : 0xff;
        uint32_t x,y;
        for (y = 0;y < ysize;y++)
        {
            for (x = 0;x < xsize;x++)
            {
                *(buf + y * xsize + x) = color;
            }
        }
    };
}

PUBLIC void kernel_main(void)
{
    g_graph_info = &g_boot_info->graph_info;

    g_tb.cur_pos.x = 0;
    g_tb.cur_pos.y = 0;
    g_tb.box_pos.x = g_graph_info->pixel_per_scanline - 100 * 9;
    g_tb.box_pos.y = 16;
    g_tb.xsize = 100 * 9;
    g_tb.ysize = g_graph_info->vertical_resolution / 2;
    g_tb.char_xsize = 9;
    g_tb.char_ysize = 16;

    pr_log(K_NAME " - " K_VERSION "\n");
    init_all();
    prog_execute("k task",DEFAULT_PRIORITY,4096,ktask);

    while(1)
    {
        task_block(TASK_BLOCKED);
        io_stihlt();
    };
}

PUBLIC void ap_kernel_main(void)
{
    ap_init_all();
    char name[31];
    sprintf(name,"k task %d",apic_id());
    prog_execute(name,DEFAULT_PRIORITY,4096,ktask);
    while (1)
    {
        task_block(TASK_BLOCKED);
        io_stihlt();
    };
}