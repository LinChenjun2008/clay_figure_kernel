/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#ifndef __CPU_H__
#define __CPU_H__

#define IA32_APIC_BASE        0x0000001b
#define IA32_APIC_BASE_BSP    (1 << 8)
#define IA32_APIC_BASE_ENABLE (1 << 11)

#define IA32_EFER     0xc0000080
#define IA32_EFER_SCE 1

#define IA32_STAR  0xc0000081
#define IA32_LSTAR 0xc0000082
#define IA32_FMASK 0xc0000084

#define NR_CPUS 256

#ifndef __ASM_INCLUDE__

extern uint64_t rdmsr(uint64_t address);
extern void     wrmsr(uint64_t address, uint64_t value);

extern void asm_cpuid(
    uint32_t  mop,
    uint32_t  sop,
    uint32_t *a,
    uint32_t *b,
    uint32_t *c,
    uint32_t *d
);

static __inline__ void cpu_name(char *s)
{
    uint32_t i;
    for (i = 0x80000002; i < 0x80000005; i++)
    {
        asm_cpuid(
            i,
            0,
            (uint32_t *)s + (i - 0x80000002) * 4,
            (uint32_t *)s + (i - 0x80000002) * 4 + 1,
            (uint32_t *)s + (i - 0x80000002) * 4 + 2,
            (uint32_t *)s + (i - 0x80000002) * 4 + 3
        );
    }
}

static __inline__ bool is_virtual_machine()
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

static __inline__ bool is_bsp()
{
    return rdmsr(IA32_APIC_BASE) & IA32_APIC_BASE_BSP;
}

static __inline__ uint32_t apic_id()
{
    uint32_t a, b, c, d;
    asm_cpuid(1, 0, &a, &b, &c, &d);
    return (b >> 24) & 0xff;
}

PUBLIC uint64_t make_icr(
    uint8_t  vector,
    uint8_t  deliver_mode,
    uint8_t  dest_mode,
    uint8_t  deliver_status,
    uint8_t  level,
    uint8_t  trigger,
    uint8_t  des_shorthand,
    uint32_t destination
);

PUBLIC status_t smp_init(void);
PUBLIC status_t smp_start(void);
PUBLIC void     send_IPI(uint64_t icr);

extern uint8_t AP_BOOT_BASE[];
extern uint8_t AP_BOOT_END[];

#endif /* __ASM_INCLUDE__ */

#endif