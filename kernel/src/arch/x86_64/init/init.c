// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/desc.h>
#include <asm/drivers/apic.h>
#include <asm/drivers/timer.h>
#include <asm/init.h>
#include <asm/interrupt.h>
#include <asm/mp.h>
#include <asm/page.h>

#include <mem.h>
#include <print.h>
#include <syscall.h>
#include <sysinfo.h>
#include <task.h>

void init_all(struct system_info *system_info)
{
    set_cpu_struct(system_info->cpu);

    struct boot_info *boot_info = system_info->boot_info;
    intr_disable();
    init_print(&boot_info->graphic_info);
    printk("Kernel initializing...\n");

    printk(MSG_INFO MSG_HIGHLIGHT("APIC") " initializing...\n");
    apic_init(boot_info);

    printk(MSG_INFO MSG_HIGHLIGHT("Segment") " initializing...\n");
    init_desc();

    printk(MSG_INFO MSG_HIGHLIGHT("Interrupt") " initializing...\n");
    intr_init();

    printk(MSG_INFO MSG_HIGHLIGHT("Timer") " initializing...\n");
    timer_init(boot_info);

    printk(MSG_INFO MSG_HIGHLIGHT("Memory management") " initializing...\n");
    mem_init(system_info);

    printk(MSG_INFO MSG_HIGHLIGHT("Task management") " initializing...\n");
    task_init(system_info, MAX_TASKS);

    printk(MSG_INFO MSG_HIGHLIGHT("System call") " initializing...\n");
    syscall_init();
    syscall_enable();

    printk(MSG_INFO MSG_HIGHLIGHT("MP") " initializing...\n");
    mp_init(system_info);

    mp_start(ap_main);

    *((uint64_t *)boot_info->page_table_pos) = 0;
    printk("\nWelcome to Clay Figure Neo!\n");
    intr_enable();

    process_execute("test", DEFAULT_PRIO, 1, 1, NULL);

    return;
}

void ap_init_all(struct system_info *system_info, uintptr_t stack)
{
    set_cpu_struct(system_info->cpu);

    intr_disable();
    ap_init_desc();

    struct cpu *cpu = get_curr_cpu_struct();
    set_cpu_struct(cpu);
    make_main_task(stack - PG_SIZE, 1);

    local_apic_init();
    apic_timer_init();

    syscall_enable();

    intr_enable();
}