// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <kernel/global.h>

#include <sync/atomic.h> // atomic_t

extern uint64_t ASMLINKAGE
asm_atomic_xchg(volatile uint64_t *atom, uint64_t value);

PUBLIC uint64_t atomic_set(atomic_t *atom, uint64_t value)
{
    return asm_atomic_xchg(&atom->value, value);
}

PUBLIC uint64_t atomic_read(atomic_t *atom)
{
    return atom->value;
}

extern void ASMLINKAGE asm_atomic_add(volatile uint64_t *atom, uint64_t value);
extern void ASMLINKAGE asm_atomic_sub(volatile uint64_t *atom, uint64_t value);
extern void ASMLINKAGE asm_atomic_inc(volatile uint64_t *atom);
extern void ASMLINKAGE asm_atomic_dec(volatile uint64_t *atom);
extern void ASMLINKAGE asm_atomic_mask(volatile uint64_t *atom, uint64_t mask);

PUBLIC void atomic_add(atomic_t *atom, uint64_t value)
{
    asm_atomic_add(&atom->value, value);
    return;
}

PUBLIC void atomic_sub(atomic_t *atom, uint64_t value)
{
    asm_atomic_sub(&atom->value, value);
    return;
}

PUBLIC void atomic_inc(atomic_t *atom)
{
    asm_atomic_inc(&atom->value);
    return;
}

PUBLIC void atomic_dec(atomic_t *atom)
{
    asm_atomic_dec(&atom->value);
    return;
}

PUBLIC void atomic_mask(atomic_t *atom, uint64_t mask)
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

PUBLIC uint64_t atomic_bts(atomic_t *atom, uint64_t bit)
{
    return asm_atomic_bts(&atom->value, bit);
}

PUBLIC uint64_t atomic_btr(atomic_t *atom, uint64_t bit)
{
    return asm_atomic_btr(&atom->value, bit);
}

PUBLIC uint64_t atomic_btc(atomic_t *atom, uint64_t bit)
{
    return asm_atomic_btc(&atom->value, bit);
}
