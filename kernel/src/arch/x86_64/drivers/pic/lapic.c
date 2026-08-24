// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/drivers/apic.h>
#include <asm/drivers/apic/lapic.h>
#include <asm/drivers/apic/struct.h>
#include <asm/page.h>
#include <asm/utils/barrier.h>
#include <asm/utils/regs.h>
#include <asm/x86.h>

extern struct apic apic;

uint32_t local_apic_read(uint16_t index)
{
    uint32_t ret;
    io_mfence();
    ret = *(volatile uint32_t *)PHYS_TO_VIRT(apic.local_apic_address + index);
    BARRIER();
    return ret;
}

void local_apic_write(uint16_t index, uint32_t value)
{
    io_mfence();
    *(volatile uint32_t *)PHYS_TO_VIRT(apic.local_apic_address + index) = value;
    io_mfence();
    BARRIER();
    return;
}

static void xapic_init()
{
    // enable SVR[8]
    uint32_t svr = local_apic_read(APIC_REG_SVR);
    svr |= 1 << 8;
    if (local_apic_read(APIC_REG_VERSION) >> 24 & 1)
    {
        svr |= 1 << 12;
    }
    local_apic_write(APIC_REG_SVR, svr);

    // Mask all LVT
    local_apic_write(0x2f0, 0x10000);
    local_apic_write(0x320, 0x10000);
    local_apic_write(0x330, 0x10000);
    local_apic_write(0x340, 0x10000);
    local_apic_write(0x350, 0x10000);
    local_apic_write(0x360, 0x10000);
    local_apic_write(0x370, 0x10000);
    return;
}

static void x2apic_init()
{
    // enable SVR[8]
    uint32_t svr = rdmsr(0x80f);
    svr |= 1 << 8;
    if (rdmsr(0x803) >> 24 & 1)
    {
        svr |= 1 << 12;
    }
    wrmsr(0x80f, svr);

    // Mask all LVT
    wrmsr(0x82f, 0x10000);
    wrmsr(0x832, 0x10000);
    wrmsr(0x833, 0x10000);
    wrmsr(0x834, 0x10000);
    wrmsr(0x835, 0x10000);
    wrmsr(0x836, 0x10000);
    wrmsr(0x837, 0x10000);
    return;
}

void local_apic_init()
{
    uint32_t a, b, c, d;
    arch_cpuid(1, 0, &a, &b, &c, &d);
    uint64_t ia32_apic_base = rdmsr(IA32_APIC_BASE);
    if ((c & (1 << 21)) && (ia32_apic_base & 0x400))
    {
        x2apic_init();
    }
    else
    {
        if ((d & (1 << 9)) && (ia32_apic_base & 0x800))
        {
            xapic_init();
        }
    }
    return;
}