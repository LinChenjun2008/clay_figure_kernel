// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYNC_ATOMIC_H__
#define __SYNC_ATOMIC_H__

struct atomic
{
    uint64_t value;
};

uint64_t atomic_set(struct atomic *atom, uint64_t value);
uint64_t atomic_read(struct atomic *atom);
uint64_t atomic_add(struct atomic *atom, uint64_t value);
uint64_t atomic_sub(struct atomic *atom, uint64_t value);
uint64_t atomic_inc(struct atomic *atom);
uint64_t atomic_dec(struct atomic *atom);

uint64_t atomic_bts(struct atomic *atom, uint64_t bit);
uint64_t atomic_btr(struct atomic *atom, uint64_t bit);
uint64_t atomic_btc(struct atomic *atom, uint64_t bit);

#endif /* __SYNC_ATOMIC_H__ */