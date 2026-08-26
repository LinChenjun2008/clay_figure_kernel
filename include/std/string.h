// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __STD_STRING_H__
#define __STD_STRING_H__

void  *memset(void *dst, int value, size_t size);
void  *memcpy(void *dst, const void *src, size_t size);
int    memcmp(const void *p1, const void *p2, size_t size);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
int    strcmp(const char *str1, const char *str2);
int    strncmp(const char *str1, const char *str2, size_t n);
size_t strlen(const char *str);
char  *strchr(const char *str, char ch);
char  *strrchr(const char *str, char ch);
char  *strcat(char *dst, char *src);

#endif /* __STD_STRING_H__ */