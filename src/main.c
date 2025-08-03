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
    gi.horizontal_resolution = xsize;
    gi.vertical_resolution   = ysize;
    gi.pixel_per_scanline    = xsize;

    int i = 0;
    while (1)
    {
        uint32_t *buf = allocate_page();
        if (buf == NULL)
        {
            continue;
        }
        gi.frame_buffer_base = (uintptr_t)buf;

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
        free_page(buf);
    };
}

PUBLIC void kernel_main(void)
{
    init_all();
    while (1)
    {
        task_block(TASK_BLOCKED);
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
    };
}