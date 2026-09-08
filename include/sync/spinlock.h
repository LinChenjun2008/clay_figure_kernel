// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_SYNC_SPINLOCK_H__
#define __ASM_SYNC_SPINLOCK_H__

#include <asm/interrupt.h>

struct spinlock
{
    volatile uint64_t lock;
    enum intr_status  intr_status;
};

void init_spinlock(struct spinlock *lk);
void spin_lock(struct spinlock *lk);
void spin_unlock(struct spinlock *lk);
void spin_lock_double(struct spinlock *a, struct spinlock *b);
void spin_unlock_double(struct spinlock *a, struct spinlock *b);

#endif /* __ASM_SYNC_SPINLOCK_H__ */