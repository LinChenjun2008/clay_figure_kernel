/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef struct spinlock_s
{
    volatile uint64_t lock;
} spinlock_t;

PUBLIC void init_spinlock(spinlock_t *spinlock);
PUBLIC void spinlock_lock(spinlock_t *spinlock);
PUBLIC void spinlock_unlock(spinlock_t *spinlock);

#endif