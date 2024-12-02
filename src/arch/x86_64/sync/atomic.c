/*
   Copyright 2024 LinChenjun

   本文件是Clay Figure Kernel的一部分。
   修改和/或再分发遵循GNU GPL version 3 (or any later version)
  
*/

#include <kernel/global.h>
#include <sync/atomic.h> // atomic_t

PUBLIC void atomic_set(atomic_t *atom,uint64_t value)
{
    atom->value = value;
    return;
}

PUBLIC uint64_t atomic_read(atomic_t *atom)
{
    return atom->value;
}

extern void asm_atomic_add(volatile uint64_t *atom,uint64_t value);
extern void asm_atomic_sub(volatile uint64_t *atom,uint64_t value);
extern void asm_atomic_inc(volatile uint64_t *atom);
extern void asm_atomic_dec(volatile uint64_t *atom);
extern void asm_atomic_mask(volatile uint64_t *atom,uint64_t mask);

PUBLIC void atomic_add(atomic_t *atom,uint64_t value)
{
    asm_atomic_add(&atom->value,value);
    return;
}

PUBLIC void atomic_sub(atomic_t *atom,uint64_t value)
{
    asm_atomic_sub(&atom->value,value);
    return;
}

PUBLIC void atomic_inc(atomic_t *atom)
{
    asm_atomic_inc(&atom->value);
    return;
}

PUBLIC void atomic_dec(atomic_t *atom)
{
    asm_atomic_dec(&atom->value);
    return;
}

PUBLIC void atomic_mask(atomic_t *atom,uint64_t mask)
{
    asm_atomic_mask(&atom->value,mask);
    return;
}