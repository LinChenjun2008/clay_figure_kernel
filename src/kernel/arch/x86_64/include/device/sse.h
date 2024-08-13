#ifndef __SSE_H__
#define __SSE_H__

typedef uint8_t fxsave_region_t[512];

PUBLIC status_t check_sse();
PUBLIC void sse_init();

PUBLIC void fxsave(fxsave_region_t *fxsave_region);
PUBLIC void fxrstor(fxsave_region_t *fxsave_region);

#endif