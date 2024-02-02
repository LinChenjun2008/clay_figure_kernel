#ifndef __CPU_H__
#define __CPU_H__

#include <kernel/global.h>

static __inline__ uint64_t rdmsr(uint64_t address)
{
    uint64_t edx = 0;
    uint64_t eax = 0;
    __asm__ __volatile__ ("rdmsr":"=d"(edx),"=a"(eax):"c"(address):"memory");
    return (edx << 32) | eax;
}

static __inline__ void wrmsr(uint64_t address,uint64_t value)
{
    __asm__ __volatile__ ("wrmsr":
                :"d"(value >> 32),"a"(value & 0xffffffff),"c"(address):"memory");
    return;
}

static __inline__ void cpuid(uint32_t mop,uint32_t sop,uint32_t* a,uint32_t* b,
                             uint32_t* c,uint32_t* d)
{
    __asm__ __volatile__
    (
        "cpuid"
        :"=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d)
        :"a"(mop),"b"(sop)
        :"memory"
    );
    return;
}

#endif