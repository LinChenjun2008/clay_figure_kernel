#ifndef __STRING_H__
#define __STRING_H__

extern void    memset(void *dst, uint8_t value, size_t size);
extern void   *memcpy(void *dst, const void *src, size_t size);
extern int32_t memcmp(const void *p1, const void *p2, size_t size);

extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, size_t n);

extern int32_t strcmp(const char *str1__, const char *str2__);
extern int32_t strncmp(const char *str1__, const char *str2__, size_t n);

extern size_t strlen(const char *str);

#endif