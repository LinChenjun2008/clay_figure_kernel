// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc/desc.h>
#include <asm/drivers/apic.h>
#include <asm/drivers/timer.h>
#include <asm/interrupt.h>
#include <asm/mem/page.h>
#include <asm/mp.h>

#include <init/init.h>
#include <mem.h>
#include <print.h>
#include <syscall.h>
#include <task.h>

static void ap_init(uint64_t stack)
{
    intr_disable();
    ap_init_desc();

    make_main_task(stack - PG_SIZE, 1);

    local_apic_init();
    apic_timer_init();

    syscall_init();

    intr_enable();
    while (1);
    return;
}

void init_all(boot_info_t *boot_info)
{
    intr_disable();
    init_print(&boot_info->graphic_info);

    printk(
        "initramfs at %p,size=%d.\n",
        boot_info->initramfs,
        boot_info->initramfs_size
    );
    printk("Page table at %p.\n", boot_info->page_table_pos);
    printk("Kernel relocate base: %p.\n", boot_info->relocate_base);

    graphic_info_t *graphic_info = &boot_info->graphic_info;
    printk("Frame buffer base: %p.\n", graphic_info->frame_buffer_base);
    printk(
        "Resolution: %dx%d.\n",
        graphic_info->horizontal_resolution,
        graphic_info->vertical_resolution
    );
    printk("Pixel per scanline: %d.\n", graphic_info->pixel_per_scanline);

    printk(MSG_INFO "Kernel initializing...\n");

    printk(MSG_INFO MSG_HIGHLIGHT("Segment") " initializing...\n");
    init_desc();

    printk(MSG_INFO MSG_HIGHLIGHT("Interrupt") " initializing...\n");
    intr_init();

    printk(MSG_INFO MSG_HIGHLIGHT("APIC") " initializing...\n");
    apic_init(boot_info);

    printk(MSG_INFO MSG_HIGHLIGHT("Memory management") " initializing...\n");
    mem_init(boot_info);

    printk(MSG_INFO MSG_HIGHLIGHT("Task management") " initializing...\n");
    task_init(boot_info, MAX_TASKS);

    printk(MSG_INFO MSG_HIGHLIGHT("Timer") " initializing...\n");
    timer_init(boot_info);

    printk(MSG_INFO MSG_HIGHLIGHT("System call") " initializing...\n");
    syscall_init();

    printk(MSG_INFO MSG_HIGHLIGHT("MP") " initializing...\n");
    mp_init(boot_info);

    mp_start(ap_init);

    printk(MSG_INFO "Kernel initializing done.\n");
    intr_enable();
    return;
}