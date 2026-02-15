// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYNC_SPINLOCK_H__
#define __SYNC_SPINLOCK_H__

#include <asm/interrupt.h>

typedef struct
{
    volatile uint64_t lock;
    intr_status_t     intr_status;
} spinlock_t;

void init_spinlock(spinlock_t *lk);
void spin_lock(spinlock_t *lk);
void spin_unlock(spinlock_t *lk);

#endif /* __SYNC_SPINLOCK_H__ */