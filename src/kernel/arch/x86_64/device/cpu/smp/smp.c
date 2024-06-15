#include <kernel/global.h>
#include <device/cpu.h>
#include <device/pic.h>
#include <mem/mem.h>
#include <std/string.h>

#include <log.h>

extern apic_t apic;

PUBLIC void smp_start()
{
    // copy ap_boot
    size_t ap_boot_size = (uintptr_t)AP_BOOT_END - (uintptr_t)AP_BOOT_BASE;
    pr_log("\1 copy AP Boot to 0x7c000.size: %d.\n",ap_boot_size);
    memcpy((void*)KADDR_P2V(0x7c000),AP_BOOT_BASE,ap_boot_size);

    // allocate stack for apu
    void *apu_stack_base = alloc_physical_page(((apic.number_of_cores - 1) * KERNEL_STACK_SIZE) / PG_SIZE + 1);
    if (apu_stack_base == NULL)
    {
        pr_log("\3 fatal: can not alloc memory for apu. \n");
        return;
    }
    pr_log("\1 ap_stack: %p \n",apu_stack_base);
    *(uintptr_t*)AP_STACK_BASE_PTR = (uintptr_t)apu_stack_base;

    pr_log("\2 Start-up IPI \n");

    // init IPI
    local_apic_write(0x300,0xc4500);

    local_apic_write(0x300,0xc467c);
    local_apic_write(0x300,0xc467c);
    pr_log("\2 Start-up IPI done. \n");
}