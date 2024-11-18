/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/global.h>
#include <kernel/init.h>
#include <intr.h>
#include <device/pic.h>
#include <device/cpu.h>
#include <device/sse.h>
#include <device/timer.h>
#include <device/pci.h>
#include <device/usb/xhci.h>
#include <mem/mem.h>
#include <task/task.h>
#include <kernel/syscall.h>
#include <service.h>

#include <log.h>

PUBLIC segmdesc_t make_segmdesc(uint32_t base,uint32_t limit,uint16_t access)
{
    segmdesc_t desc;
    desc.limit_low    = (limit  & 0x0000ffff);
    desc.base_low     = (base   & 0x0000ffff);
    desc.base_mid     = ((base  & 0x00ff0000) >> 16);
    desc.access_right = (access & 0x00ff); // TYPE,S,DPL,P
    desc.limit_high   = (((limit >> 16) & 0x0f) | ((access >> 8 ) & 0x00f0)); // AVL,L,D/B,G
    desc.base_high    = ((base >> 24) & 0x00ff);
    return desc;
}

PUBLIC segmdesc_t gdt_table[8192];

extern void asm_load_gdt(void *gdt_ptr,uint16_t code,uint16_t data);
PRIVATE void load_gdt()
{
    uint128_t gdt_ptr = (((uint128_t)0
                        + ((uint128_t)((uint64_t)gdt_table))) << 16)
                        | (sizeof(gdt_table) - 1);
    asm_load_gdt(&gdt_ptr,SELECTOR_CODE64_K,SELECTOR_DATA64_K);
    return;
}

extern void asm_ltr(uint64_t sel);
PRIVATE void load_tss(uint8_t nr_cpu)
{
    asm_ltr(SELECTOR_TSS(nr_cpu));
    return;
}

PRIVATE void init_desc()
{
    gdt_table[0] = make_segmdesc(0,      0,             0);
    gdt_table[1] = make_segmdesc(0,      0,     AR_CODE64);
    gdt_table[2] = make_segmdesc(0,      0,     AR_DATA64);
    gdt_table[3] = make_segmdesc(0,      0,AR_DATA64_DPL3);
    gdt_table[4] = make_segmdesc(0,      0,AR_CODE64_DPL3);
    init_tss(0);
    load_gdt();
    load_tss(0);
}

PRIVATE spinlock_t schedule_lock;

extern uint64_t global_ticks;

PUBLIC void init_all()
{
    init_spinlock(&schedule_lock);
    intr_disable();
    pr_log("\1 Segment initializing ...");
    init_desc();
    pr_log(" OK.\n");

    pr_log("\1 Interrrupt initializing ...");
    intr_init();
    pr_log(" OK.\n");

    pr_log("\1 SSE initializing ...");
    if (ERROR(check_sse()))
    {
        pr_log("\3 HW no support SSE.\n");
        while (1) continue;
    }

    sse_init();
    pr_log(" OK.\n");

    pr_log("\1 Memory initializing ...");
    mem_init();
    mem_allocator_init();
    pr_log(" OK.\n");

    pr_log("\1 Task initializing ...");
    task_init();
    pr_log(" OK.\n");

    spinlock_lock(&schedule_lock);
    pr_log("\1 SMP initializing ...");
    detect_cores();
    smp_init();
    pr_log(" OK.\n");

    pr_log("\1 PIC initializing ...");
    pic_init();
    pr_log(" OK.\n");

    pr_log("\1 Timer initializing ...");
    pit_init();
    pr_log(" OK.\n");

    pr_log("\1 PCI initializing ...");
    pci_scan_all_bus();
    pr_log(" OK.\n");

    pr_log("\1 System Call initializing ...");
    syscall_init();
    pr_log(" OK.\n");

    pr_log("\1 Service initializing ...");
    service_init();
    intr_enable();
    pr_log(" OK.\n");

    pr_log("\1 Memory: %d MB (%d GB), free: %d MB,using: %d%% .\n",
        total_memory() >> 20,
        total_memory() >> 30,
        total_free_pages() * 2,
        100 - (total_free_pages() * 100 / total_pages()));

    spinlock_unlock(&schedule_lock);

    do_schedule();

    message_t msg;
    msg.m3.p1 = (void*)g_boot_info->graph_info.frame_buffer_base;
    msg.m3.i1 = g_boot_info->graph_info.horizontal_resolution;
    msg.m3.i2 = g_boot_info->graph_info.vertical_resolution;
    sys_send_recv(NR_SEND,VIEW,&msg);

    smp_start();

    return;
}

extern taskmgr_t *tm;
PUBLIC void ap_init_all()
{
    intr_disable();
    init_tss(apic_id());
    load_gdt();
    load_tss(apic_id());

    ap_intr_init();
    local_apic_init();
    sse_init();
    syscall_init();
    intr_enable();
    return;
}