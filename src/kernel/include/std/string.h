#ifndef __STRING_H__
#define __STRING_H__

#include <kernel/def.h>

static __inline__ void memset(void *dst,uint8_t value,size_t size)
{
    uint8_t *__dst = dst;
    int i = size;
    while(i--) {*__dst++ = value;}
    return;
}

static __inline__ void memcpy(void *dst,void *src,size_t size)
{
    uint8_t *__dst = dst;
    uint8_t *__src = src;
    int i = size;
    while(i--) {*__dst++ = *__src++;}
    return;
}

#endif