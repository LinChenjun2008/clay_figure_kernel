/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <kernel/init.h>
#include <intr.h>
#include <device/pic.h>
#include <device/cpu.h>
#include <device/sse.h>
#include <device/timer.h>
#include <device/pci.h>
#include <mem/mem.h>
#include <task/task.h>
#include <kernel/syscall.h>
#include <service.h>
#include <io.h>              // get_cr3,set_cr3

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
PRIVATE void load_gdt(void)
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

PRIVATE void init_desc(void)
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

extern uint64_t global_ticks;

PUBLIC void init_all(void)
{
    pr_log(0,K_NAME " - " K_VERSION "\n");

    intr_disable();

    pr_log(LOG_TRACE,"Segment initializing ...\n");
    init_desc();

    pr_log(LOG_TRACE,"Interrrupt initializing ...\n");
    intr_init();

    pr_log(LOG_TRACE,"SSE initializing ...\n");
    if (ERROR(check_sse()))
    {
        pr_log(LOG_FATAL,"HW no support SSE.\n");
        while (1) continue;
    }
    sse_enable();
    sse_init();

    pr_log(LOG_TRACE,"Memory initializing ...\n");
    mem_init();
    mem_allocator_init();

    pr_log(LOG_TRACE,"Task initializing ...\n");
    task_init();

    pr_log(LOG_TRACE,"SMP initializing ...\n");
    detect_cores();
    if (ERROR(smp_init()))
    {
        pr_log(LOG_FATAL,"Failed to initialize SMP.\n");
        while (1) continue;
    }

    pr_log(LOG_TRACE,"PIC initializing ...\n");
    pic_init();

    pr_log(LOG_TRACE,"Timer initializing ...\n");
    pit_init();

    pr_log(LOG_TRACE,"PCI initializing ...\n");
    pci_scan_all_bus();

    pr_log(LOG_TRACE,"System Call initializing ...\n");
    syscall_init();

    intr_enable();

    // Call apic_timer_init() after intr_enable()
    // then we can use global_ticks to calibrate apic timer.
    pr_log(LOG_TRACE,"Setting up APIC timer ...\n");
    apic_timer_init();
    pr_log(LOG_TRACE,"Kernel initializing done.\n");

    pr_log(LOG_TRACE,"Service initializing ...\n");
    service_init();

    smp_start();

    *((uint64_t*)KERNEL_PAGE_DIR_TABLE_POS) = 0;
    set_cr3(get_cr3());
    return;
}

extern taskmgr_t *tm;
PUBLIC void ap_init_all(void)
{
    intr_disable();
    init_tss(apic_id());
    load_gdt();
    load_tss(apic_id());

    ap_intr_init();
    local_apic_init();
    apic_timer_init();

    sse_enable();
    syscall_init();
    intr_enable();
    return;
}