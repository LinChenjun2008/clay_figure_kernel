// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
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

#define IA32_FS_BASE        0xc0000100
#define IA32_GS_BASE        0xc0000101
#define IA32_KERNEL_GS_BASE 0xc0000102

#define NR_CPUS 256

#ifndef __ASM_INCLUDE__

extern uint64_t rdmsr(uint64_t address);
extern void     wrmsr(uint64_t address, uint64_t value);

extern void ASMLINKAGE asm_cpuid(
    uint32_t  mop,
    uint32_t  sop,
    uint32_t *a,
    uint32_t *b,
    uint32_t *c,
    uint32_t *d
);

PUBLIC char    *cpu_name(char *s);
PUBLIC bool     is_virtual_machine(void);
PUBLIC bool     is_bsp(void);
PUBLIC uint32_t apic_id(void);

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
PUBLIC void     send_ipi(uint64_t icr);

extern uint8_t AP_BOOT_BASE[];
extern uint8_t AP_BOOT_END[];

#endif /* __ASM_INCLUDE__ */

#endif