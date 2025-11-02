// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 Lin Chenjun
 */
#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h>    // rdmsr,wrmsr,
#include <device/pic.h>    // local_apic_write,eoi,apic
#include <intr.h>          // register_handle
#include <io.h>            // io_hlt
#include <mem/allocator.h> // kmalloc,kfree
#include <mem/page.h>      // PHYS_TO_VIRT
#include <std/stdio.h>     // sprintf
#include <std/string.h>    // memcpy
#include <task/task.h>     // init_task_struct,spinlock,list

extern apic_t apic;

PUBLIC uint64_t make_icr(
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
    uint64_t icr;
    icr = (vector & 0xff) | (deliver_mode & 0x07) << 8 | (dest_mode & 1) << 11 |
          (deliver_status & 1) << 12 | (level & 1) << 14 | (trigger & 1) << 15 |
          (des_shorthand & 0x03) << 18 | ((uint64_t)destination) << 32;
    return icr;
}

PRIVATE void ipi_panic_handler(intr_stack_t *stack)
{
    send_eoi(stack->int_vector);
    intr_disable();
    while (1) io_hlt();
    return;
}

PUBLIC status_t smp_init(void)
{
    status_t status;

    // allocate stack for apu
    uint8_t *apu_stack_base;
    uint64_t apu_stack_size;
    apu_stack_size = (NR_CPUS * KERNEL_STACK_SIZE);
    status         = kmalloc(apu_stack_size, 0, 0, &apu_stack_base);
    if (ERROR(status))
    {
        PANIC(1, "Failed to alloc memory for apu. \n");
        return K_NOMEM;
    }
    *(uintptr_t *)AP_STACK_BASE_PTR = (uintptr_t)apu_stack_base;

    int i;
    for (i = 1; i < NR_CPUS; i++)
    {
        char name[16];
        sprintf(name, "Main task (%d)", i);
        task_struct_t *ap_main_task = task_alloc();
        if (ap_main_task == NULL)
        {
            PANIC(1, "Failed to alloc task for AP.\n");
            return K_NOMEM;
        }
        init_task_struct(
            ap_main_task,
            name,
            DEFAULT_PRIORITY,
            (uintptr_t)apu_stack_base + (i - 1) * KERNEL_STACK_SIZE,
            KERNEL_STACK_SIZE,
            0
        );
        ap_main_task->cpu_id = i;
        task_man_t *task_man = get_task_man(i);
        task_man->main_task  = ap_main_task;
    }

    // register IPI
    register_handle(0x81, ipi_panic_handler);

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
    return K_SUCCESS;
}

PUBLIC status_t smp_start(void)
{
    // copy ap_boot
    size_t ap_boot_size = (uintptr_t)AP_BOOT_END - (uintptr_t)AP_BOOT_BASE;
    memcpy((void *)PHYS_TO_VIRT(0x10000), AP_BOOT_BASE, ap_boot_size);

    *(uint64_t *)AP_FLAGS = 0;
    *(void **)AP_MAIN     = NULL;

    uint64_t icr;
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

    *(uint64_t *)AP_FLAGS = 1;
    while (*(uint64_t *)AP_FLAGS != apic.number_of_cores) io_mfence();

    *(void **)AP_MAIN = ap_kernel_main;
    return K_SUCCESS;
}

PUBLIC void send_ipi(uint64_t icr)
{
    local_apic_write(0x310, icr >> 32);
    local_apic_write(0x300, icr & 0xffffffff);
    return;
}