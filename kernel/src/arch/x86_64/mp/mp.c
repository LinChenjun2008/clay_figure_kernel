// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/drivers/timer.h>
#include <asm/mp.h>
#include <asm/page.h> // PHYS_TO_VIRT
#include <asm/utils.h>
#include <asm/x86.h>

#include <print.h>
#include <std/string.h>

extern uint8_t AP_BOOT_START[];
extern uint8_t AP_BOOT_END[];

static uint64_t make_icr(
    uint8_t  vector,
    uint8_t  deliver_mode,
    uint8_t  dest_mode,
    uint8_t  deliver_status,
    uint8_t  level,
    uint8_t  trigger,
    uint8_t  des_shorthand,
    uint32_t destination
)
{
    uint64_t icr = 0;
    icr |= (vector & 0xff);
    icr |= (deliver_mode & 0x07) << 8;
    icr |= (dest_mode & 1) << 11;
    icr |= (deliver_status & 1) << 12;
    icr |= (level & 1) << 14;
    icr |= (trigger & 1) << 15;
    icr |= (des_shorthand & 0x03) << 18;
    icr |= ((uint64_t)destination) << 32;
    return icr;
}

static void send_ipi(uint64_t icr)
{
    local_apic_write(APIC_REG_ICR_HI, icr >> 32);
    local_apic_write(APIC_REG_ICR_LO, icr & 0xffffffff);
    return;
}

void mp_init(struct boot_info *boot_info)
{
    size_t ap_boot_size = (uintptr_t)AP_BOOT_END - (uintptr_t)AP_BOOT_START;
    printk("mp_init: copy AP_BOOT to %p, size=%d.\n", AP_START, ap_boot_size);
    memcpy(PHYS_TO_VIRT(AP_START), AP_BOOT_START, ap_boot_size);
    *(void **)PHYS_TO_VIRT(AP_PAGE_TABLE_PTR) = boot_info->page_table_pos;
    *(uint64_t *)PHYS_TO_VIRT(AP_BOOT_FLAG)   = 0;
    *(volatile void **)PHYS_TO_VIRT(AP_STACK) = NULL;
    *(void **)PHYS_TO_VIRT(AP_ENTRY)          = NULL;

    printk("mp_init: send init ipi to secondary cpu(s).\n");
    // init IPI
    uint64_t icr = make_icr(
        0,
        ICR_DELIVER_MODE_INIT,
        ICR_DEST_MODE_PHY,
        ICR_DELIVER_STATUS_IDLE,
        ICR_LEVEL_DE_ASSEST,
        ICR_TRIGGER_EDGE,
        ICR_ALL_EXCLUDE_SELF,
        0
    );
    send_ipi(icr);

    printk("mp_init: send start-up ipi to secondary cpu(s).\n");
    icr = make_icr(
        0x10,
        ICR_DELIVER_MODE_START_UP,
        ICR_DEST_MODE_PHY,
        ICR_DELIVER_STATUS_IDLE,
        ICR_LEVEL_DE_ASSEST,
        ICR_TRIGGER_EDGE,
        ICR_ALL_EXCLUDE_SELF,
        0
    );
    send_ipi(icr);
    send_ipi(icr);

    *(volatile uint64_t *)PHYS_TO_VIRT(AP_BOOT_FLAG) = 1;

    uint64_t cpu_count = apic_cpu_count();
    while (*(volatile uint64_t *)PHYS_TO_VIRT(AP_BOOT_FLAG) < cpu_count);

    cpu_count = *(volatile uint64_t *)PHYS_TO_VIRT(AP_BOOT_FLAG);
    printk("mp_init: %d cpu(s) started.\n", cpu_count);

    uintptr_t stack = 0;

    uint64_t i;
    for (i = 1; i < cpu_count; i++)
    {
        stack = (uintptr_t)allocate_pages(1);
        ASSERT(stack != 0);
        *(volatile uintptr_t *)PHYS_TO_VIRT(AP_STACK) = stack + PG_SIZE;
        while (*(volatile uint64_t *)PHYS_TO_VIRT(AP_STACK) == stack + PG_SIZE);
    }
    printk("mp_init: secondary cpu(s) are ready.\n");

    return;
}

void mp_start(void *mp_entry)
{
    printk(MSG_INFO "secondary cpu(s) starting...\n");
    *(void **)PHYS_TO_VIRT(AP_ENTRY) = mp_entry;
    return;
}