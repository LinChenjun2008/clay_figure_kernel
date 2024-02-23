#include <kernel/global.h>
#include <kernel/init.h>
#include <intr.h>
#include <device/pic.h>
#include <device/timer.h>
#include <device/pci.h>
#include <device/usb/hci/xhci.h>
#include <mem/mem.h>
#include <task/task.h>
#include <kernel/syscall.h>
#include <service.h>

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

PUBLIC segmdesc_t gdt_table[7];

PRIVATE void init_desctrib()
{
    gdt_table[0] = make_segmdesc(0,      0,             0);
    gdt_table[1] = make_segmdesc(0,      0,     AR_CODE64);
    gdt_table[2] = make_segmdesc(0,      0,     AR_DATA64);
    gdt_table[3] = make_segmdesc(0,      0,AR_DATA64_DPL3);
    gdt_table[4] = make_segmdesc(0,      0,AR_CODE64_DPL3);
    init_tss();
    uint128_t gdt_ptr = (((uint128_t)0
                        + ((uint128_t)((uint64_t)gdt_table))) << 16)
                        | (sizeof(gdt_table) - 1);
    __asm__ __volatile__
    (
        "lgdtq %[gdt_ptr] \n\t"
        "movw %%ax,%%ds \n\t"
        "movw %%ax,%%es \n\t"
        "movw %%ax,%%fs \n\t"
        "movw %%ax,%%gs \n\t"
        "movw %%ax,%%ss \n\t"

        "pushq %[SELECTOR_CODE64] \n\t"
        "leaq .next(%%rip),%%rax \n\t"
        "pushq %%rax \n\t"
        "lretq \n\t"// == jmp SELECTOR_CODE64:.next
        ".next: \n\t"
        "ltr %w[TSS] \n\t"
        :
        :[gdt_ptr]"m"(gdt_ptr),[SELECTOR_CODE64]"i"(SELECTOR_CODE64_K),
         [SELECTOR_DATA64]"ax"(SELECTOR_DATA64_K),[TSS]"r"(SELECTOR_TSS)
        :"memory"
    );
}

/**
 * @brief 初始化所有内容
*/
PUBLIC void init_all()
{
    intr_disable();

    init_desctrib();
    intr_init();
    mem_init();
    mem_alloctor_init();
    detect_cores();
    pic_init();
    pit_init();
    pci_scan_all_bus();
    task_init();
    syscall_init();
    service_init();


    // xhci_init();
    intr_enable();

    extern uint64_t global_ticks;
    uint64_t ticks = global_ticks;
    while(global_ticks <= ticks + running_task()->priority * 2);

    message_t msg;
    msg.m3.p1 = (void*)g_boot_info->graph_info.frame_buffer_base;
    msg.m3.i1 = g_boot_info->graph_info.horizontal_resolution;
    msg.m3.i2 = g_boot_info->graph_info.vertical_resolution;
    sys_send_recv(NR_SEND,VIEW,&msg);

    return;
}