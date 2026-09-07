// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TYPES_H__
#define __ASM_TYPES_H__

#define __WORDSIZE 64

typedef signed char          int8_t;
typedef signed short         int16_t;
typedef signed int           int32_t;
typedef signed long long int int64_t;

typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long int uint64_t;

typedef uint64_t phys_addr_t;
typedef uint64_t word_t;

typedef uint64_t uintptr_t;
typedef int64_t  intptr_t;

typedef uint64_t size_t;
typedef int64_t  ssize_t;

_Static_assert(sizeof(int8_t) == 1, "int8_t size mismatch");
_Static_assert(sizeof(int16_t) == 2, "int16_t size mismatch");
_Static_assert(sizeof(int32_t) == 4, "int32_t size mismatch");
_Static_assert(sizeof(int64_t) == 8, "int64_t size mismatch");

_Static_assert(sizeof(uint8_t) == 1, "uint8_t size mismatch");
_Static_assert(sizeof(uint16_t) == 2, "uint16_t size mismatch");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t size mismatch");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t size mismatch");

_Static_assert(sizeof(phys_addr_t) == 8, "phys_addr_t size mismatch");
_Static_assert(sizeof(word_t) == 8, "word_t size mismatch");

_Static_assert(sizeof(uintptr_t) == sizeof(void *), "uintptr_t size mismatch");
_Static_assert(sizeof(intptr_t) == sizeof(void *), "intptr_t size mismatch");

_Static_assert(sizeof(size_t) == sizeof(void *), "size_t size mismatch");
_Static_assert(sizeof(ssize_t) == sizeof(void *), "sszie_t size mismatch");

#endif /* __ASM_TYPES_H__ */