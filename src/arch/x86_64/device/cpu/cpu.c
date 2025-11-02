// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <device/cpu.h>
#include <std/string.h>

PUBLIC char *cpu_name(char *s)
{
    uint32_t i;
    uint32_t j;
    uint32_t p[16];
    for (i = 0x80000002; i < 0x80000005; i++)
    {
        j = (i - 0x80000002) * 4;
        asm_cpuid(i, 0, &p[j], &p[j + 1], &p[j + 2], &p[j + 3]);
    }
    memcpy(s, p, sizeof(p));
    s[64] = '\0';
    return s;
}

PUBLIC bool is_virtual_machine(void)
{
    uint32_t eax, ebx, ecx, edx;
    char     s[13];
    asm_cpuid(0x40000000, 0, &eax, &ebx, &ecx, &edx);
    memcpy(s + 0, &ebx, 4);
    memcpy(s + 4, &ecx, 4);
    memcpy(s + 8, &edx, 4);
    s[12] = '\0';
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
