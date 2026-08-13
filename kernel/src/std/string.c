// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 Lin Chenjun
 */

#include <base.h>

#include <std/string.h>

void *memset(void *dst, int value, size_t size)
{
    uint8_t *p          = dst;
    uint8_t  byte_value = (uint8_t)value;
    while (size-- > 0) *p++ = byte_value;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t size)
{
    uint8_t *d = dst;
    uint8_t *s = (uint8_t *)src;
    while (size-- > 0) *d++ = *s++;
    return dst;
}

int memcmp(const void *p1, const void *p2, size_t size)
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

char *strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *(char *)src++) != '\0');
    return ret;
}
char *strncpy(char *dst, const char *src, size_t n)
{
    char  *ret = dst;
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return ret;
}

int strcmp(const char *str1, const char *str2)
{
    while (*str1 != '\0' && *str2 != '\0')
    {
        if (*str1 != *str2) break;
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

int strncmp(const char *str1, const char *str2, size_t n)
{
    if (n == 0) return 0;

    while (n-- > 0)
    {
        if (*str1 != *str2)
        {
            return *str1 - *str2;
        }
        if (*str1 == '\0')
        {
            break;
        }
        str1++;
        str2++;
    }
    return 0;
}

size_t strlen(const char *str)
{
    const char *end = str;
    while (*end++ != '\0');
    return end - str - 1;
}

char *strchr(const char *str, char ch)
{
    char *ret = NULL;
    while (*str != '\0')
    {
        if (*str == ch)
        {
            ret = (char *)str;
            break;
        }
        str++;
    }
    return ret;
}

char *strrchr(const char *str, char ch)
{
    char *ret = NULL;
    while (*str != '\0')
    {
        if (*str == ch)
        {
            ret = (char *)str;
        }
        str++;
    }
    return ret;
}

char *strcat(char *dst, char *src)
{
    char *ret = dst;
    while (*dst++ != '\0') continue;
    dst--;
    while ((*dst++ = *src++) != '\0') continue;
    return ret;
}