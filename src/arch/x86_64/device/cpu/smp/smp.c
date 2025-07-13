// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright 2024-2025 LinChenjun
 */
#include <kernel/global.h>

#include <log.h>

#include <device/cpu.h> // rdmsr,wrmsr,
#include <device/pic.h> // local_apic_write,eoi,apic
#include <intr.h>       // register_handle
#include <io.h>         // io_hlt
#include <mem/page.h>   // alloc_physical_page
#include <std/stdio.h>  // sprintf
#include <std/string.h> // memcpy
#include <task/task.h>  // init_task_struct,spinlock,list

extern apic_t     apic;
extern taskmgr_t *tm;

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
    // copy ap_boot
    size_t ap_boot_size = (addr_t)AP_BOOT_END - (addr_t)AP_BOOT_BASE;
    memcpy((void *)KADDR_P2V(0x10000), AP_BOOT_BASE, ap_boot_size);

    // allocate stack for apu
    status_t status;
    uint8_t *apu_stack_base;
    status = alloc_physical_page(
        ((NR_CPUS - 1) * KERNEL_STACK_SIZE) / PG_SIZE + 1, &apu_stack_base
    );
    if (ERROR(status))
    {
        PR_LOG(LOG_FATAL, "can not alloc memory for apu. \n");
        return K_NOMEM;
    }
    *(phy_addr_t *)AP_STACK_BASE_PTR = (phy_addr_t)apu_stack_base;

    int i;
    for (i = 1; i < NR_CPUS; i++)
    {
        char name[16];
        sprintf(name, "idle(%d)", i);
        pid_t ap_main_pid;
        status = task_alloc(&ap_main_pid);
        if (ERROR(status))
        {
            PR_LOG(LOG_FATAL, "Alloc task for AP error.\n");
            free_physical_page(
                apu_stack_base,
                ((NR_CPUS - 1) * KERNEL_STACK_SIZE) / PG_SIZE + 1
            );
            return K_NOMEM;
        }
        task_struct_t *idle_task = pid2task(ap_main_pid);
        addr_t         kstack_base;
        kstack_base =
            (addr_t)KADDR_P2V(apu_stack_base + (i - 1) * KERNEL_STACK_SIZE);
        init_task_struct(
            idle_task, name, DEFAULT_PRIORITY, kstack_base, KERNEL_STACK_SIZE
        );
        tm->core[i].idle_task = idle_task->pid;
        spinlock_lock(&tm->core[i].task_list_lock);
        list_append(&tm->core[i].task_list, &idle_task->general_tag);
        spinlock_unlock(&tm->core[i].task_list_lock);
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
    send_IPI(icr);
    return K_SUCCESS;
}

PUBLIC status_t smp_start(void)
{
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
    send_IPI(icr);
    send_IPI(icr);

    // waiting for AP start.
    uint64_t cores = apic.number_of_cores;
    while (*(uint64_t *)AP_START_FLAG != cores - 1);

    return K_SUCCESS;
}

PUBLIC void send_IPI(uint64_t icr)
{
    local_apic_write(0x310, icr >> 32);
    local_apic_write(0x300, icr & 0xffffffff);
}