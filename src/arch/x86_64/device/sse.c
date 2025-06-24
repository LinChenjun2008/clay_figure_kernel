/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/sse.h> // fxsave_region_t
#include <device/cpu.h> // cpuid

PUBLIC status_t check_sse(void)
{
    uint32_t a, b, c, d;
    asm_cpuid(1, 0, &a, &b, &c, &d);
    // SSE & FXSR
    return d & (3 << 24) ? K_SUCCESS : K_HW_NOSUPPORT;
}