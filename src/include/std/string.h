#ifndef __STRING_H__
#define __STRING_H__


PUBLIC void  *memset(void *dst, int value, size_t size);
PUBLIC void  *memcpy(void *dst, const void *src, size_t size);
PUBLIC int    memcmp(const void *p1, const void *p2, size_t size);
PUBLIC char  *strcpy(char *dst, const char *src);
PUBLIC char  *strncpy(char *dst, const char *src, size_t n);
PUBLIC int    strcmp(const char *str1, const char *str2);
PUBLIC int    strncmp(const char *str1, const char *str2, size_t n);
PUBLIC size_t strlen(const char *str);

#endif