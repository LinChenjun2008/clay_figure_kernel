// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <device/cpu.h>

PUBLIC char *cpu_name(char *s)
{
    uint32_t  i;
    uint32_t *p;
    for (i = 0x80000002; i < 0x80000005; i++)
    {
        p = (uint32_t *)s + (i - 0x80000002) * sizeof(uint32_t);
        asm_cpuid(i, 0, p, p + 1, p + 2, p + 3);
    }
    return s;
}

PUBLIC bool is_virtual_machine(void)
{
    uint32_t eax, ebx, ecx, edx;
    char     s[13];
    asm_cpuid(0x40000000, 0, &eax, &ebx, &ecx, &edx);
    ((uint32_t *)s)[0] = ebx;
    ((uint32_t *)s)[1] = ecx;
    ((uint32_t *)s)[2] = edx;
    s[12]              = '\0';
    int i;
    for (i = 0; i < 13; i++)
    {
        if (s[i] == "TCGTCGTCGTCG"[i] || s[i] == "KVMKVMKVM\0\0\0"[i])
        {
            continue;
        }
        return FALSE;
    }
    return TRUE;
}

PUBLIC bool is_bsp(void)
{
    return rdmsr(IA32_APIC_BASE) & IA32_APIC_BASE_BSP;
}

PUBLIC uint32_t apic_id(void)
{
    uint32_t a, b, c, d;
    asm_cpuid(1, 0, &a, &b, &c, &d);
    return (b >> 24) & 0xff;
}
