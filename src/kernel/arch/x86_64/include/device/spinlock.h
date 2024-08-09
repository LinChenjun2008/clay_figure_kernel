#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef struct
{
    volatile uint64_t lock;
    volatile pid_t    holder;
    volatile uint64_t holder_repeat_nr;
} spinlock_t;

PUBLIC void init_spinlock(spinlock_t *spinlock);
PUBLIC void spinlock_lock(spinlock_t *spinlock);
PUBLIC void spinlock_unlock(spinlock_t *spinlock);

#endif