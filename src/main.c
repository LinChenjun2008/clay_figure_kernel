// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <common.h>
#include <intr.h>
#include <io.h>
#include <kernel/init.h>
#include <kernel/syscall.h>
#include <mem/page.h>
#include <service.h>
#include <std/stdio.h>
#include <std/string.h>
#include <task/task.h>
#include <ulib.h>

extern textbox_t g_tb;

PRIVATE void ktask(void)
{
    uint32_t color = 0x00c5c5c5;
    uint32_t xsize = 100;
    uint32_t ysize = 16;

    textbox_t tb;
    tb.cur_pos.x  = 0;
    tb.cur_pos.y  = 0;
    tb.box_pos.x  = 0;
    tb.box_pos.y  = 0;
    tb.xsize      = xsize;
    tb.ysize      = ysize;
    tb.char_xsize = 9;
    tb.char_ysize = 16;

    graph_info_t gi;

    int i = 0;
    while (1)
    {
        uint32_t *buf =
            allocate_page(xsize * ysize * sizeof(uint32_t) / PG_SIZE + 1);
        gi.frame_buffer_base     = (addr_t)buf;
        gi.horizontal_resolution = xsize;
        gi.vertical_resolution   = ysize;
        gi.pixel_per_scanline    = xsize;
        char s[10];
        sprintf(s, "\n%d", i);
        basic_print(&gi, &tb, color, s);
        fill(
            buf,
            xsize * ysize * sizeof(uint32_t),
            xsize,
            ysize,
            (apic_id() - 1) * xsize,
            0
        );
        i++;
        free_page(buf, xsize * ysize * sizeof(uint32_t) / PG_SIZE + 1);
    };
}

PUBLIC void kernel_main(void)
{
    size_t bss_size = &_ebss[0] - &_bss[0];
    memset(&_bss, 0, bss_size);

    BOOT_INFO->initramfs = KADDR_P2V(BOOT_INFO->initramfs);

    graph_info_t *g_graph_info;
    g_graph_info    = &BOOT_INFO->graph_info;
    g_tb.cur_pos.x  = 0;
    g_tb.cur_pos.y  = 0;
    g_tb.box_pos.x  = 8;
    g_tb.box_pos.y  = 16;
    g_tb.xsize      = g_graph_info->pixel_per_scanline - 8;
    g_tb.ysize      = g_graph_info->vertical_resolution - 16;
    g_tb.char_xsize = 9;
    g_tb.char_ysize = 16;

    init_all();

    while (1)
    {
        task_block(TASK_BLOCKED);
        io_stihlt();
    };
}

PUBLIC void ap_kernel_main(void)
{
    ap_init_all();
    char name[31];
    sprintf(name, "k task %d", apic_id());
    prog_execute(name, DEFAULT_PRIORITY, 4096, ktask);
    while (1)
    {
        task_block(TASK_BLOCKED);
        io_stihlt();
    };
}