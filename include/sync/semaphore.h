// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYNC_SEMAPHORE_H__
#define __SYNC_SEMAPHORE_H__

#include <sync/spinlock.h>
#include <task/schedule.h>

struct semaphore
{
    struct spinlock   lock;
    long              value;
    struct wait_queue wq;
};

void init_semaphore(struct semaphore *sema, long value);
void sema_down(struct semaphore *sema);
void sema_up(struct semaphore *sema);

#endif /* __SYNC_SEMAPHORE_H__ */