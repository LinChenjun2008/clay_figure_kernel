// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_UTILS_H__
#define __ASM_UTILS_H__

extern SYSV_ABI void asm_load_gdt(void *gdt_ptr, uint16_t code, uint16_t data);
extern SYSV_ABI void asm_lidt(void *idt_ptr);
extern SYSV_ABI void asm_ltr(uint64_t sel);

extern SYSV_ABI void io_cli(void);
extern SYSV_ABI void io_sti(void);
extern SYSV_ABI void io_hlt(void);
extern SYSV_ABI void io_stihlt(void);
extern SYSV_ABI void io_lfence(void);
extern SYSV_ABI void io_sfence(void);
extern SYSV_ABI void io_mfence(void);

extern SYSV_ABI uint32_t io_in8(uint32_t port);
extern SYSV_ABI uint32_t io_in16(uint32_t port);
extern SYSV_ABI uint32_t io_in32(uint32_t port);

extern SYSV_ABI void io_out8(uint32_t port, uint32_t data);
extern SYSV_ABI void io_out16(uint32_t port, uint32_t data);
extern SYSV_ABI void io_out32(uint32_t port, uint32_t data);

extern SYSV_ABI uint64_t get_flags(void);
extern SYSV_ABI uint64_t get_rsp(void);
extern SYSV_ABI uint64_t get_cr0(void);
extern SYSV_ABI uint64_t get_cr2(void);
extern SYSV_ABI uint64_t get_cr3(void);
extern SYSV_ABI void     set_cr3(uint64_t cr3);
extern SYSV_ABI uint64_t get_cr4(void);

extern SYSV_ABI uint64_t rdmsr(uint64_t address);
extern SYSV_ABI void     wrmsr(uint64_t address, uint64_t value);

extern SYSV_ABI void asm_cpuid(
    uint32_t  mop,
    uint32_t  sop,
    uint32_t *a,
    uint32_t *b,
    uint32_t *c,
    uint32_t *d
);

extern uint64_t asm_atomic_xchg(volatile uint64_t *atom, uint64_t value);
extern void     asm_atomic_add(volatile uint64_t *atom, uint64_t value);
extern void     asm_atomic_sub(volatile uint64_t *atom, uint64_t value);
extern void     asm_atomic_inc(volatile uint64_t *atom);
extern void     asm_atomic_dec(volatile uint64_t *atom);
extern void     asm_atomic_mask(volatile uint64_t *atom, uint64_t mask);

extern uint64_t asm_atomic_bts(volatile uint64_t *atom, uint64_t bit);
extern uint64_t asm_atomic_btr(volatile uint64_t *atom, uint64_t bit);
extern uint64_t asm_atomic_btc(volatile uint64_t *atom, uint64_t bit);

extern void asm_panic(void);

#endif /* __ASM_UTILS_H__ */