// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

typedef struct atomic_s
{
    volatile uint64_t value;
} atomic_t;

PUBLIC void     atomic_set(atomic_t *atom, uint64_t value);
PUBLIC uint64_t atomic_read(atomic_t *atom);
PUBLIC void     atomic_add(atomic_t *atom, uint64_t value);
PUBLIC void     atomic_sub(atomic_t *atom, uint64_t value);
PUBLIC void     atomic_inc(atomic_t *atom);
PUBLIC void     atomic_dec(atomic_t *atom);
PUBLIC void     atomic_mask(atomic_t *atom, uint64_t mask);

PUBLIC uint64_t atomic_bts(atomic_t *atom, uint64_t bit);
PUBLIC uint64_t atomic_btr(atomic_t *atom, uint64_t bit);
PUBLIC uint64_t atomic_btc(atomic_t *atom, uint64_t bit);

#endif