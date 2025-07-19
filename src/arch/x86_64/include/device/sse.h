// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __SSE_H__
#define __SSE_H__

typedef uint8_t fxsave_region_t[512];

PUBLIC status_t check_sse(void);
extern void     sse_enable(void);
extern void     sse_init(void);

extern void asm_fxsave(fxsave_region_t *fxsave_region);
extern void asm_fxrstor(fxsave_region_t *fxsave_region);

#endif