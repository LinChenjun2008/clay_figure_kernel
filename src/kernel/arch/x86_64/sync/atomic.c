#include <kernel/global.h>
#include <sync/atomic.h>

PUBLIC void atomic_add(IN(atomic_t *atom,uint64_t value))
{
    __asm__ __volatile__ ("lock addq %1,%0":"=m"(atom->value):"r"(value):"memory");
    return;
}

PUBLIC void atomic_sub(atomic_t *atom,uint64_t value)
{
    __asm__ __volatile__ ("lock subq %1,%0":"=m"(atom->value):"r"(value):"memory");
    return;
}

PUBLIC void atomic_inc(atomic_t *atom)
{
    __asm__ __volatile__ ("lock incq %0":"=m"(atom->value)::"memory");
    return;
}

PUBLIC void atomic_dec(atomic_t *atom)
{
    __asm__ __volatile__ ("lock decq %0":"=m"(atom->value)::"memory");
    return;
}