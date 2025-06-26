/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

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

#endif