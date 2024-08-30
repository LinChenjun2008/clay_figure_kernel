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
#include <device/cpu.h> // rdmsr,wrmsr,
#include <device/pic.h> // local_apic_write,eoi,apic
#include <mem/mem.h>    // alloc_physical_page
#include <std/string.h> // memcpy
#include <intr.h>       // register_handle
#include <task/task.h>  // init_task_struct,spinlock,list
#include <std/stdio.h>  // sprintf

#include <log.h>

extern apic_t apic;
extern taskmgr_t *tm;

PUBLIC uint64_t make_icr(
    uint8_t  vector,
    uint8_t  deliver_mode,
    uint8_t  dest_mode,
    uint8_t  deliver_status,
    uint8_t  level,
    uint8_t  trigger,
    uint8_t  des_shorthand,
    uint32_t destination)
{
    uint64_t icr;
    icr =     (vector         & 0xff)
            | (deliver_mode   & 0x07) <<  8
            | (dest_mode      &    1) << 11
            | (deliver_status &    1) << 12
            | (level          &    1) << 14
            | (trigger        &    1) << 15
            | (des_shorthand  & 0x03) << 18
            | ((uint64_t)destination) << 32;
    return icr;
}


PRIVATE void ipi_timer_handler()
{
    eoi(0x80);
    do_schedule();
    return;
}

PRIVATE void ipi_panic_handler()
{
    eoi(0x81);
    while (1) __asm__ __volatile("cli\n\t""hlt":::);
    return;
}

PUBLIC status_t smp_start()
{
    // copy ap_boot
    size_t ap_boot_size = (addr_t)AP_BOOT_END - (addr_t)AP_BOOT_BASE;
    memcpy((void*)KADDR_P2V(0x10000),AP_BOOT_BASE,ap_boot_size);

    // allocate stack for apu
    status_t status;
    void *apu_stack_base;
    status = alloc_physical_page(
                ((NR_CPUS - 1) * KERNEL_STACK_SIZE) / PG_SIZE + 1,
                &apu_stack_base);
    if (ERROR(status))
    {
        pr_log("\3 fatal: can not alloc memory for apu. \n");
        return K_ERROR;
    }
    *(phy_addr_t*)AP_STACK_BASE_PTR = (phy_addr_t)apu_stack_base;

    int i;
    for (i = 1;i < NR_CPUS;i++)
    {
        char name[16];
        sprintf(name,"idle(%d)",i);
        pid_t ap_main_pid;
        status = task_alloc(&ap_main_pid);
        if (ERROR(status))
        {
            pr_log("\3 Alloc task for AP error.\n");
            free_physical_page(apu_stack_base,((NR_CPUS - 1) * KERNEL_STACK_SIZE) / PG_SIZE + 1);
            return K_ERROR;
        }
        task_struct_t *main_task   = pid2task(ap_main_pid);
        addr_t         kstack_base;
        kstack_base = (addr_t)
                      KADDR_P2V(apu_stack_base + (i - 1) * KERNEL_STACK_SIZE);
        init_task_struct(main_task,
                         name,
                         DEFAULT_PRIORITY,
                         kstack_base,
                         KERNEL_STACK_SIZE);
        tm->idle_task[i] = main_task->pid;
        spinlock_lock(&tm->task_list_lock[i]);
        list_append(&tm->task_list[i],&main_task->general_tag);
        spinlock_unlock(&tm->task_list_lock[i]);

    }

    // register IPI
    register_handle(0x80,ipi_timer_handler);
    register_handle(0x81,ipi_panic_handler);
    // init IPI
    uint64_t icr = make_icr(
        0,
        ICR_DELIVER_MODE_INIT,
        ICR_DEST_MODE_PHY,
        ICR_DELIVER_STATUS_IDLE,
        ICR_LEVEL_DE_ASSEST,
        ICR_TRIGGER_EDGE,
        ICR_ALL_EXCLUDE_SELF,
        0);
    send_IPI(icr);

    icr = make_icr(
        0x10,
        ICR_DELIVER_MODE_START_UP,
        ICR_DEST_MODE_PHY,
        ICR_DELIVER_STATUS_IDLE,
        ICR_LEVEL_DE_ASSEST,
        ICR_TRIGGER_EDGE,
        ICR_ALL_EXCLUDE_SELF,
        0);
    send_IPI(icr);
    send_IPI(icr);

    return K_SUCCESS;
}

PUBLIC void send_IPI(uint64_t icr)
{
    local_apic_write(0x310,icr >> 32);
    local_apic_write(0x300,icr & 0xffffffff);
}