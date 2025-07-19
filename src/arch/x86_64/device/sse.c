// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <device/cpu.h> // cpuid
#include <device/sse.h> // fxsave_region_t

PUBLIC status_t check_sse(void)
{
    uint32_t a, b, c, d;
    asm_cpuid(1, 0, &a, &b, &c, &d);
    // SSE & FXSR
    return d & (3 << 24) ? K_SUCCESS : K_HW_NOSUPPORT;
}