// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <std/string.h>

PUBLIC void *memset(void *dst, int value, size_t size)
{
    uint8_t *p          = dst;
    uint8_t  byte_value = (uint8_t)value;
    while (size-- > 0) *p++ = byte_value;
    return dst;
}

PUBLIC void *memcpy(void *dst, const void *src, size_t size)
{
    uint8_t *d = dst;
    uint8_t *s = (uint8_t *)src;
    while (size-- > 0) *d++ = *s++;
    return dst;
}

PUBLIC int memcmp(const void *p1, const void *p2, size_t size)
{
    const uint8_t *s1 = p1;
    const uint8_t *s2 = p2;
    while (size-- > 0)
    {
        if (*s1 != *s2) return *s1 - *s2;
        s1++;
        s2++;
    }
    return 0;
}

PUBLIC char *strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *(char *)src++) != '\0');
    return ret;
}

PUBLIC char *strncpy(char *dst, const char *src, size_t n)
{
    char *ret = dst;
    while (n-- > 0 && *src != '\0') *dst++ = *src++;

    while (n-- > 0) *dst++ = '\0';
    return ret;
}

PUBLIC int strcmp(const char *str1, const char *str2)
{
    while (*str1 != '\0' && *str2 != '\0')
    {
        if (*str1 != *str2) break;
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

PUBLIC int strncmp(const char *str1, const char *str2, size_t n)
{
    while (n-- > 0 && *str1 && *str2)
    {
        if (*str1 != *str2) return *str1 - *str2;
        str1++;
        str2++;
    }
    if (n > 0) return *str1 - *str2;

    return 0;
}

PUBLIC size_t strlen(const char *str)
{
    const char *end = str;
    while (*end++ != 0);
    return end - str - 1;
}
