// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_UTILS_H__
#define __ASM_UTILS_H__

void ASMLINKAGE asm_load_gdt(void *gdt_ptr, uint16_t code, uint16_t data);
void ASMLINKAGE asm_lidt(void *idt_ptr);
void ASMLINKAGE asm_ltr(uint64_t sel);

void ASMLINKAGE io_cli(void);
void ASMLINKAGE io_sti(void);
void ASMLINKAGE io_hlt(void);
void ASMLINKAGE io_stihlt(void);
void ASMLINKAGE io_lfence(void);
void ASMLINKAGE io_sfence(void);
void ASMLINKAGE io_mfence(void);

uint32_t ASMLINKAGE io_in8(uint32_t port);
uint32_t ASMLINKAGE io_in16(uint32_t port);
uint32_t ASMLINKAGE io_in32(uint32_t port);

void ASMLINKAGE io_out8(uint32_t port, uint32_t data);
void ASMLINKAGE io_out16(uint32_t port, uint32_t data);
void ASMLINKAGE io_out32(uint32_t port, uint32_t data);

uint64_t ASMLINKAGE get_flags(void);
uint64_t ASMLINKAGE get_rsp(void);
uint64_t ASMLINKAGE get_cr0(void);
uint64_t ASMLINKAGE get_cr2(void);
uint64_t ASMLINKAGE get_cr3(void);
void ASMLINKAGE     set_cr3(uint64_t cr3);
uint64_t ASMLINKAGE get_cr4(void);

uint64_t ASMLINKAGE rdmsr(uint64_t address);
void ASMLINKAGE     wrmsr(uint64_t address, uint64_t value);

void ASMLINKAGE asm_cpuid(
    uint32_t  mop,
    uint32_t  sop,
    uint32_t *a,
    uint32_t *b,
    uint32_t *c,
    uint32_t *d
);

uint64_t ASMLINKAGE asm_atomic_xchg(volatile uint64_t *atom, uint64_t value);
void ASMLINKAGE     asm_atomic_add(volatile uint64_t *atom, uint64_t value);
void ASMLINKAGE     asm_atomic_sub(volatile uint64_t *atom, uint64_t value);
void ASMLINKAGE     asm_atomic_inc(volatile uint64_t *atom);
void ASMLINKAGE     asm_atomic_dec(volatile uint64_t *atom);
void ASMLINKAGE     asm_atomic_mask(volatile uint64_t *atom, uint64_t mask);

uint64_t ASMLINKAGE asm_atomic_bts(volatile uint64_t *atom, uint64_t bit);
uint64_t ASMLINKAGE asm_atomic_btr(volatile uint64_t *atom, uint64_t bit);
uint64_t ASMLINKAGE asm_atomic_btc(volatile uint64_t *atom, uint64_t bit);

void ASMLINKAGE asm_panic(void);

#endif /* __ASM_UTILS_H__ */