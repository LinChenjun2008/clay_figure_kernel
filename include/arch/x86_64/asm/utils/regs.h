// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_UTILS_REGS_H__
#define __ASM_UTILS_REGS_H__

uint64_t ASMLINKAGE get_flags(void);
uint64_t ASMLINKAGE get_rsp(void);
uint64_t ASMLINKAGE get_cr0(void);
uint64_t ASMLINKAGE get_cr2(void);
uint64_t ASMLINKAGE get_cr3(void);
void ASMLINKAGE     set_cr3(uint64_t cr3);
uint64_t ASMLINKAGE get_cr4(void);

uint64_t ASMLINKAGE rdmsr(uint64_t address);
void ASMLINKAGE     wrmsr(uint64_t address, uint64_t value);

void ASMLINKAGE arch_cpuid(
    uint32_t  mop,
    uint32_t  sop,
    uint32_t *a,
    uint32_t *b,
    uint32_t *c,
    uint32_t *d
);

#endif /* __ASM_UTILS_REGS_H__ */
