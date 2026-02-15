// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "../../../std/string.c"

#include "bootloader.h"

char16_t *strcpy16(char16_t *dst, const char16_t *src)
{
    char16_t *ret = dst;
    while ((*dst++ = *(char16_t *)src++) != '\0');
    return ret;
}

char16_t *strncpy16(char16_t *dst, const char16_t *src, size_t n)
{
    char16_t *ret = dst;
    while (n-- > 0 && *src != '\0') *dst++ = *src++;

    while (n-- > 0) *dst++ = '\0';
    return ret;
}

int strcmp16(const char16_t *str1, const char16_t *str2)
{
    while (*str1 != '\0' && *str2 != '\0')
    {
        if (*str1 != *str2) break;
        str1++;
        str2++;
    }
    return *str1 - *str2;
}


int strncmp16(const char16_t *str1, const char16_t *str2, size_t n)
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

size_t strlen16(const char16_t *str)
{
    const char16_t *end = str;
    while (*end++ != '\0');
    return end - str - 1;
}

char16_t *strchr16(const char16_t *str, char16_t ch)
{
    char16_t *ret = NULL;
    while (*str != '\0')
    {
        if (*str == ch)
        {
            ret = (char16_t *)str;
            break;
        }
        str++;
    }
    return ret;
}

char16_t *strrchr16(const char16_t *str, char16_t ch)
{
    char16_t *ret = NULL;
    while (*str != '\0')
    {
        if (*str == ch)
        {
            ret = (char16_t *)str;
        }
        str++;
    }
    return ret;
}

char16_t *strcat16(char16_t *dst, char16_t *src)
{
    char16_t *ret = dst;
    while (*dst++ != '\0') continue;
    dst--;
    while ((*dst++ = *src++) != '\0') continue;
    return ret;
}