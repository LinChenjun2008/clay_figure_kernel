// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <sync/atomic.h>

extern uint64_t ASMLINKAGE
asm_atomic_xchg(volatile uint64_t *atom, uint64_t value);

uint64_t atomic_set(struct atomic *atom, uint64_t value)
{
    return asm_atomic_xchg(&atom->value, value);
}

uint64_t atomic_read(struct atomic *atom)
{
    return atom->value;
}

extern void ASMLINKAGE asm_atomic_add(volatile uint64_t *atom, uint64_t value);
extern void ASMLINKAGE asm_atomic_sub(volatile uint64_t *atom, uint64_t value);
extern void ASMLINKAGE asm_atomic_inc(volatile uint64_t *atom);
extern void ASMLINKAGE asm_atomic_dec(volatile uint64_t *atom);
extern void ASMLINKAGE asm_atomic_mask(volatile uint64_t *atom, uint64_t mask);

void atomic_add(struct atomic *atom, uint64_t value)
{
    asm_atomic_add(&atom->value, value);
    return;
}

void atomic_sub(struct atomic *atom, uint64_t value)
{
    asm_atomic_sub(&atom->value, value);
    return;
}

void atomic_inc(struct atomic *atom)
{
    asm_atomic_inc(&atom->value);
    return;
}

void atomic_dec(struct atomic *atom)
{
    asm_atomic_dec(&atom->value);
    return;
}

void atomic_mask(struct atomic *atom, uint64_t mask)
{
    asm_atomic_mask(&atom->value, mask);
    return;
}

extern uint64_t ASMLINKAGE
asm_atomic_bts(volatile uint64_t *atom, uint64_t bit);
extern uint64_t ASMLINKAGE
asm_atomic_btr(volatile uint64_t *atom, uint64_t bit);
extern uint64_t ASMLINKAGE
asm_atomic_btc(volatile uint64_t *atom, uint64_t bit);

uint64_t atomic_bts(struct atomic *atom, uint64_t bit)
{
    return asm_atomic_bts(&atom->value, bit);
}

uint64_t atomic_btr(struct atomic *atom, uint64_t bit)
{
    return asm_atomic_btr(&atom->value, bit);
}

uint64_t atomic_btc(struct atomic *atom, uint64_t bit)
{
    return asm_atomic_btc(&atom->value, bit);
}
