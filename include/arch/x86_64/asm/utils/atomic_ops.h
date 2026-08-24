// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_UTILS_ATOMIC_OPS_H__
#define __ASM_UTILS_ATOMIC_OPS_H__

uint64_t ASMLINKAGE arch_atomic_xchg(volatile uint64_t *atom, uint64_t value);
uint64_t ASMLINKAGE arch_atomic_add(volatile uint64_t *atom, uint64_t value);
uint64_t ASMLINKAGE arch_atomic_sub(volatile uint64_t *atom, uint64_t value);
uint64_t ASMLINKAGE arch_atomic_inc(volatile uint64_t *atom);
uint64_t ASMLINKAGE arch_atomic_dec(volatile uint64_t *atom);

uint64_t ASMLINKAGE arch_atomic_bts(volatile uint64_t *atom, uint64_t bit);
uint64_t ASMLINKAGE arch_atomic_btr(volatile uint64_t *atom, uint64_t bit);
uint64_t ASMLINKAGE arch_atomic_btc(volatile uint64_t *atom, uint64_t bit);

void ASMLINKAGE arch_pause(void);

#endif /* __ASM_UTILS_ATOMIC_OPS_H__ */
