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