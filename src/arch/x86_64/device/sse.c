/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/sse.h> // fxsave_region_t
#include <device/cpu.h> // cpuid

PUBLIC status_t check_sse()
{
    uint32_t a,b,c,d;
    asm_cpuid(1,0,&a,&b,&c,&d);
    // SSE & FXSR
    return d & (3 << 24) ? K_SUCCESS : K_ERROR;
}