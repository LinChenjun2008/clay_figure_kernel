#include <kernel/global.h>
#include <device/cpu.h>
#include <device/pic.h>
#include <std/string.h>

#include <log.h>

PUBLIC void smp_start()
{
    // copy ap_boot
    size_t ap_boot_size = (uintptr_t)AP_BOOT_END - (uintptr_t)AP_BOOT_BASE;
    pr_log("\1 copy AP Boot to 0x7c00.size: %d \n",ap_boot_size);
    memcpy((void*)KADDR_P2V(0x7c000),AP_BOOT_BASE,ap_boot_size);

    pr_log("\2 Start-up IPI \n");

    // init IPI
    local_apic_write(0x300,0xc4500);

    local_apic_write(0x300,0xc467c);
    local_apic_write(0x300,0xc467c);
    pr_log("\2 Start-up IPI done. \n");
}