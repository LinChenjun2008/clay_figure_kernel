#include <kernel/global.h>
#include <device/cpu.h>
#include <device/pic.h>
#include <mem/mem.h>
#include <std/string.h>
#include <intr.h>
#include <log.h>
#include <task/task.h>

extern apic_t apic;
extern taskmgr_t tm;

PUBLIC uint64_t make_icr
(
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
    task_struct_t *cur_task = running_task();
    if (cur_task == NULL)
    {
        pr_log("\3 cur_task == NULL.(%d)",apic_id());
        return;
    }
    cur_task->elapsed_ticks++;
    if (cur_task->ticks == 0)
    {
        schedule();
    }
    else
    {
        cur_task->ticks--;
    }
    return;
}


PUBLIC status_t smp_start()
{
    // copy ap_boot
    size_t ap_boot_size = (addr_t)AP_BOOT_END - (addr_t)AP_BOOT_BASE;
    memcpy((void*)KADDR_P2V(0x7c000),AP_BOOT_BASE,ap_boot_size);

    // allocate stack for apu
    status_t status;
    void *apu_stack_base;
    status = alloc_physical_page
                        (IN(((NR_CPUS - 1) * KERNEL_STACK_SIZE) / PG_SIZE + 1),
                         OUT(&apu_stack_base));
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
        status = task_alloc(OUT(&ap_main_pid));
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

        spinlock_lock(&tm.task_lock);
        list_append(&tm.task_list[i],&main_task->general_tag);
        spinlock_unlock(&tm.task_lock);

    }

    // register IPI
    register_handle(0x80,ipi_timer_handler);

    // init IPI
    uint64_t icr = make_icr
    (
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

    icr = make_icr
    (
        0x7c,
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

    return K_SUCCESS;
}

PUBLIC void send_IPI(uint64_t icr)
{
    local_apic_write(0x310,icr >> 32);
    local_apic_write(0x300,icr & 0xffffffff);
}