// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <common.h>
#include <config.h>
#include <intr.h>
#include <io.h>
#include <kernel/init.h>
#include <kernel/syscall.h>
#include <mem/page.h>
#include <ramfs.h>
#include <service.h>
#include <std/stdio.h>
#include <std/string.h>
#include <task/task.h>
#include <ulib.h>

extern textbox_t g_tb;

PRIVATE void ktask(void)
{
    uint32_t  color = 0x00000000;
    uint32_t  xsize = 10;
    uint32_t  ysize = 10;
    uint32_t *buf =
        allocate_page(xsize * ysize * sizeof(uint32_t) / PG_SIZE + 1);
    while (1)
    {
        fill(
            buf,
            xsize * ysize * sizeof(uint32_t),
            xsize,
            ysize,
            (apic_id() - 1) * 10,
            0
        );
        color = color ? color << 8 : 0xff;
        uint32_t x, y;
        for (y = 0; y < ysize; y++)
        {
            for (x = 0; x < xsize; x++)
            {
                *(buf + y * xsize + x) = color;
            }
        }
    };
}

PUBLIC void kernel_main(void)
{
    size_t bss_size = &_ebss[0] - &_bss[0];
    memset(&_bss, 0, bss_size);
    graph_info_t *g_graph_info;
    g_graph_info    = &G_BOOT_INFO->graph_info;
    g_tb.cur_pos.x  = 0;
    g_tb.cur_pos.y  = 0;
    g_tb.box_pos.x  = 8;
    g_tb.box_pos.y  = 16;
    g_tb.xsize      = g_graph_info->pixel_per_scanline - 8;
    g_tb.ysize      = g_graph_info->vertical_resolution - 16;
    g_tb.char_xsize = 9;
    g_tb.char_ysize = 16;

    G_BOOT_INFO->initramfs = KADDR_P2V(G_BOOT_INFO->initramfs);
    PR_LOG(0, "initramfs: %p.\n", G_BOOT_INFO->initramfs);
    status_t status = ramfs_check(G_BOOT_INFO->initramfs);
    PANIC(ERROR(status), "initramfs check failed.\n");

    ramfs_file_t fp;
    if (ERROR(ramfs_open(G_BOOT_INFO->initramfs, "config", &fp)))
    {
        pr_log(LOG_FATAL, "Can not read cinfig.\n");
        while (1) io_stihlt();
    }
    parse_config(&fp);

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