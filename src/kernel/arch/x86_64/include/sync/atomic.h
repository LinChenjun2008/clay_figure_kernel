#ifndef __ATOMIC_H__
#define __ATOMIC_H__

typedef struct atomic_s
{
    volatile uint64_t value;
} atomic_t;

PUBLIC void atomic_add(atomic_t *atom,uint64_t value);
PUBLIC void atomic_sub(atomic_t *atom,uint64_t value);
PUBLIC void atomic_inc(atomic_t *atom);
PUBLIC void atomic_dec(atomic_t *atom);

#endif