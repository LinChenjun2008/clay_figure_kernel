#include <kernel/global.h>
#include <device/sse.h>
#include <device/cpu.h>
#include <task/task.h>

PUBLIC status_t check_sse()
{
    uint32_t a,b,c,d;
    cpuid(1,0,&a,&b,&c,&d);
    // SSE & FXSR
    return d & (3 << 24) ? K_SUCCESS : K_ERROR;
}

PUBLIC void sse_init()
{
    __asm__ __volatile__
    (
        "movq %%cr0,%%rax \n\t"
        "andw $0xfffb,%%ax \n\t"
        "orw $0x2,%%ax \n\t"
        "movq %%rax,%%cr0 \n\t"
        "movq %%cr4,%%rax \n\t"
        "orw $(3 << 9),%%ax \n\t"
        "movq %%rax,%%cr4 \n\t"
    :
    :
    :
    );
    return;
}

PUBLIC void fxsave(fxsave_region_t *fxsave_region)
{
    __asm__ __volatile__ ("fxsave %0"::"m"(*fxsave_region));
}

PUBLIC void fxrstor(fxsave_region_t *fxsave_region)
{
    __asm__ __volatile__ ("fxrstor %0"::"m"(*fxsave_region));
}