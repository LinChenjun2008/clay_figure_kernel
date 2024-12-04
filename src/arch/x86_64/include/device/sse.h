/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __SSE_H__
#define __SSE_H__

typedef uint8_t fxsave_region_t[512];

PUBLIC status_t check_sse();
extern void sse_init();

extern void asm_fxsave(fxsave_region_t *fxsave_region);
extern void asm_fxrstor(fxsave_region_t *fxsave_region);

#endif