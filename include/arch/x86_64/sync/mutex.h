// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __SYNC_MUTEX_H__
#define __SYNC_MUTEX_H__

#include <sync/semaphore.h>
#include <task/struct.h>

struct mutex
{
    struct task     *holder;
    struct semaphore lock;
};

void init_mutex(struct mutex *mutex);
void mutex_lock(struct mutex *mutex);
void mutex_unlock(struct mutex *mutex);

#endif /* __SYNC_MUTEX_H__ */