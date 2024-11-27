/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/spinlock.h>
#include <task/task.h> // running_task

#include <log.h>

PUBLIC void init_spinlock(spinlock_t *spinlock)
{
    spinlock->lock = 1;
    return;
}

extern void asm_spinlock_lock(volatile uint64_t *lock);
PUBLIC void spinlock_lock(spinlock_t *spinlock)
{
    running_task()->spinlock_count++;
    asm_spinlock_lock(&spinlock->lock);
    return;
}

PUBLIC void spinlock_unlock(spinlock_t *spinlock)
{
    spinlock->lock = 1;
    running_task()->spinlock_count--;
    return;
}