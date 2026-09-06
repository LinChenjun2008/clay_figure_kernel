// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYNC_SEMAPHORE_H__
#define __SYNC_SEMAPHORE_H__

#include <lib/linked_list.h>
#include <sync/spinlock.h>

struct semaphore
{
    struct spinlock lock;
    uint64_t        value;
    struct list     wait_list;
};

void init_semaphore(struct semaphore *sema, uint64_t value);
void sema_down(struct semaphore *sema);
void sema_up(struct semaphore *sema);

#endif /* __SYNC_SEMAPHORE_H__ */