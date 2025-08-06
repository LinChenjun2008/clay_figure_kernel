// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <config.h>
#include <device/cpu.h>
#include <device/pci.h>
#include <device/pic.h>
#include <device/sse.h>
#include <device/timer.h>
#include <intr.h>
#include <kernel/init.h>
#include <kernel/syscall.h>
#include <mem/mem.h>  // mem_init,total_pages,total_free_pages
#include <mem/page.h> // KERNEL_PAGE_TABLE_POS,set_page_table
#include <ramfs.h>
#include <service.h>
#include <softirq.h>
#include <task/task.h>

extern textbox_t g_tb;

PUBLIC segmdesc_t make_segmdesc(uint32_t base, uint32_t limit, uint16_t access)
{
    segmdesc_t desc;
    desc.limit_low    = (limit & 0x0000ffff);
    desc.base_low     = (base & 0x0000ffff);
    desc.base_mid     = ((base & 0x00ff0000) >> 16);
    desc.access_right = (access & 0x00ff); // TYPE,S,DPL,P
    desc.limit_high =
        (((limit >> 16) & 0x0f) | ((access >> 8) & 0x00f0)); // AVL,L,D/B,G
    desc.base_high = ((base >> 24) & 0x00ff);
    return desc;
}

PUBLIC segmdesc_t gdt_table[8192];

extern void ASMLINKAGE
asm_load_gdt(void *gdt_ptr, uint16_t code, uint16_t data);

PRIVATE void load_gdt(void)
{
    uint64_t gdt_ptr[2];
    gdt_ptr[0] = (((uint64_t)gdt_table) << 16) | (sizeof(gdt_table) - 1);
    gdt_ptr[1] = (((uint64_t)gdt_table) >> 48) & 0xffff;
    asm_load_gdt(&gdt_ptr, SELECTOR_CODE64_K, SELECTOR_DATA64_K);
    return;
}

extern void ASMLINKAGE asm_ltr(uint64_t sel);

PRIVATE void load_tss(uint8_t cpu_id)
{
    asm_ltr(SELECTOR_TSS(cpu_id));
    return;
}

PRIVATE void init_desc(void)
{
    gdt_table[0] = make_segmdesc(0, 0, 0);
    gdt_table[1] = make_segmdesc(0, 0, AR_CODE64);
    gdt_table[2] = make_segmdesc(0, 0, AR_DATA64);
    gdt_table[3] = make_segmdesc(0, 0, AR_DATA64_DPL3);
    gdt_table[4] = make_segmdesc(0, 0, AR_CODE64_DPL3);
    init_tss(0);
    load_gdt();
    load_tss(0);
}


PUBLIC void init_all(void)
{
    intr_disable();

    BOOT_INFO->initramfs = PHYS_TO_VIRT(BOOT_INFO->initramfs);

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

    status_t status = ramfs_check(BOOT_INFO->initramfs);
    PANIC(ERROR(status), "initramfs check failed.\n");

    ramfs_file_t fp;
    if (ERROR(ramfs_open(BOOT_INFO->initramfs, "config", &fp)))
    {
        pr_log(LOG_FATAL, "Can not read cinfig.\n");
        while (1) continue;
    }
    parse_config(&fp);

    PR_LOG(LOG_INFO, "Segment initializing ...\n");
    init_desc();

    PR_LOG(LOG_INFO, "Interrrupt initializing ...\n");
    intr_init();
    softirq_init();

    PR_LOG(LOG_INFO, "SSE initializing ...\n");
    if (ERROR(check_sse()))
    {
        PR_LOG(LOG_FATAL, "HW no support SSE.\n");
        while (1) continue;
    }
    sse_enable();
    sse_init();

    PR_LOG(LOG_INFO, "Memory initializing ...\n");
    mem_init();
    size_t total_pages = get_total_free_pages();

    PR_LOG(LOG_INFO, "Task initializing ...\n");
    task_init();

    PR_LOG(LOG_INFO, "SMP initializing ...\n");
    detect_cores();
    if (ERROR(smp_init()))
    {
        PR_LOG(LOG_FATAL, "Failed to initialize SMP.\n");
        while (1) continue;
    }

    PR_LOG(LOG_INFO, "PIC initializing ...\n");
    pic_init();

    PR_LOG(LOG_INFO, "Timer initializing ...\n");
    pit_init();

    PR_LOG(LOG_INFO, "PCI initializing ...\n");
    pci_scan_all_bus();

    PR_LOG(LOG_INFO, "System Call initializing ...\n");
    syscall_init();

    intr_enable();

    // Call apic_timer_init() after intr_enable()
    // then we can use global_ticks to calibrate apic timer.
    PR_LOG(LOG_INFO, "Setting up APIC timer ...\n");
    apic_timer_init();
    PR_LOG(LOG_INFO, "Kernel initializing done.\n");
    clear_textbox(&BOOT_INFO->graph_info, &g_tb);

    //  _____   _       _____   _____
    // /  ___| | |     |  ___| /  ___|
    // | |     | |     | |__   | | __
    // | |     | |     |  __|  | ||_ |
    // | |___  | |___  | |     | |_| |
    // \_____| |_____| |_|     \_____/
    const char *logo[] = {
        "                                ",
        " _____   _       _____   _____  ",
        "/  ___| | |     |  ___| /  ___| ",
        "| |     | |     | |__   | | __  ",
        "| |     | |     |  __|  | ||_ | ",
        "| |___  | |___  | |     | |_| | ",
        "\\_____| |_____| |_|     \\_____/ ",
    };
    char     s[64];
    uint32_t horz = BOOT_INFO->graph_info.horizontal_resolution;
    uint32_t vert = BOOT_INFO->graph_info.vertical_resolution;

    pr_msg("%s System Informations \n", logo[0]);
    pr_msg("%s -----------------\n", logo[1]);
    pr_msg("%s Kernel: %s (%s) %s\n", logo[2], K_NAME, K_NAME_S, K_VERSION);
    pr_msg("%s Resolution: %dx%d\n", logo[3], horz, vert);
    pr_msg("%s CPU: %s\n", logo[4], cpu_name(s));
    pr_msg("%s Memory: %d MiB\n", logo[5], total_pages * 2);
    pr_msg("%s\n", logo[6]);

    pr_msg("\n");
    pr_msg("Copyright (C) 2024-2025 " K_NAME " Developers.\n\n");

    pr_log(
        0,
        "This is free software; see the source for copying conditions.  There "
        "is NO\n"
        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR "
        "PURPOSE.\n\n"
    );
    PR_LOG(LOG_INFO, "Service initializing ...\n");
    service_init();

    smp_start();

    *((uint64_t *)KERNEL_PAGE_DIR_TABLE_POS) = 0;
    page_table_activate(running_task());
    return;
}

PUBLIC void ap_init_all(void)
{
    uint32_t cpu_id = apic_id();
    intr_disable();
    init_tss(cpu_id);
    load_gdt();
    load_tss(cpu_id);

    // ap_main_task存储在idle_task中
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)get_task_man(cpu_id)->idle_task);
    running_task()->status = TASK_RUNNING;
    // 创建真正的idle_task
    create_idle_task();

    ap_intr_init();
    local_apic_init();
    apic_timer_init();

    sse_enable();
    syscall_init();

    intr_enable();

    return;
}