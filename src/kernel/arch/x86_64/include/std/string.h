#ifndef __STRING_H__
#define __STRING_H__

static __inline__ void memset(void *dst,uint8_t value,size_t size)
{
    uint8_t *__dst = dst;
    int i = size;
    while(i--) {*__dst++ = value;}
    return;
}

static __inline__ void memcpy(void *dst,const void *src,size_t size)
{
    __asm__ __volatile__
    (
        "cld \n\t"
        "rep movsb \n\t"
        ::"rdi"(dst),"rsi"(src),"rcx"(size):"cc"
    );
}

static __inline__ int32_t memcmp(const void *p1__,const void *p2__,size_t size)
{
    uint8_t *p1 = (uint8_t*)p1__;
    uint8_t *p2 = (uint8_t*)p2__;
    while(--size && *p1++ == *p2++);
    return (*p1 < *p2 ? -1 : *p1 > *p2);
}

static __inline__ char* strcpy(char *dst__,const char *src__)
{
    char *r = dst__;
    while ((*dst__++ = *src__++));
    return r;
}

static __inline__ size_t strlen(const char *str)
{
    int len = 0;
    while (*(str++)){len++;}
    return len;
}

static __inline__ int32_t strcmp(const char *str1__,const char *str2__)
{
    while (*str1__ && *str1__++ == *str2__++);
    return (*str1__ < *str2__ ? -1 : *str1__ > *str2__);
}

static __inline__ int32_t strncmp(const char *str1__,const char *str2__,size_t n)
{
    while (--n && *str1__++ == *str2__++);
    return (*str1__ < *str2__ ? -1 : *str1__ > *str2__);
}

#endif