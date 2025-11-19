// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 Lin Chenjun
 */

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <intr.h>

typedef struct spinlock_s
{
    volatile uint64_t      lock;
    volatile intr_status_t intr_status;
} spinlock_t;

PUBLIC void init_spin(spinlock_t *spinlock);
PUBLIC void spin_lock(spinlock_t *spinlock);
PUBLIC void spin_unlock(spinlock_t *spinlock);

#endif