#include <kernel/global.h>
#include <device/spinlock.h>
#include <task/task.h>

#include <log.h>

PUBLIC void init_spinlock(spinlock_t *spinlock)
{
    spinlock->lock = 1;
    return;
}

PUBLIC void spinlock_lock(spinlock_t *spinlock)
{
    running_task()->spinlock_count++;
    __asm__ __volatile__
    (
        ".try_lock: \n\t"
        "lock decq %0 \n\t"
        "jz done \n\t"
        "wait: \n\t"
        "pause \n\t"
        "cmpq $1,%0 \n\t"
        "jne wait \n\t"
        "jmp .try_lock \n\t"
        "done: \n\t"
        :
        :"m"(spinlock->lock)
    );
}

PUBLIC void spinlock_unlock(spinlock_t *spinlock)
{
    spinlock->lock = 1;
    running_task()->spinlock_count--;
    return;
}