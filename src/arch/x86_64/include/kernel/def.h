// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#ifndef __DEF_H__
#define __DEF_H__

#ifndef NULL
#    define NULL ((void *)0)
#endif

#ifndef PUBLIC
#    define PUBLIC
#endif

#ifndef PRIVATE
#    define PRIVATE static
#endif

typedef int bool;
#ifndef TRUE
#    define TRUE (1 == 1)
#endif

#ifndef FALSE
#    define FALSE (1 == 0)
#endif

#define SIGNATURE32(A, B, C, D) ((D) << 24 | (C) << 16 | (B) << 8 | (A))

#define GET_FIELD(X, FIELD) (((X) >> FIELD##_SHIFT) & FIELD##_MASK)
#define SET_FIELD(X, FIELD, VALUE)              \
    (((X) & ~(FIELD##_MASK << FIELD##_SHIFT)) | \
     (((VALUE) & (FIELD##_MASK)) << FIELD##_SHIFT))

#define ASMLINKAGE     __attribute__((sysv_abi))
#define WEAK           __attribute__((weak))
#define ALIGNED(ALIGN) __attribute__((aligned(ALIGN)))

#ifndef STATIC_ASSERT
#    define STATIC_ASSERT(CONDITION, MESSAGE) _Static_assert(CONDITION, MESSAGE)
#endif

#define DIV_ROUND_UP(X, STEP) (((X) + (STEP - 1)) / STEP)

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define UNUSED(X) ((void)(X))

typedef signed char          int8_t;
typedef signed short         int16_t;
typedef signed int           int32_t;
typedef signed long long int int64_t;

typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long int uint64_t;

typedef unsigned long long int uintptr_t;
typedef signed long long int   intptr_t;
typedef unsigned long long int size_t;

typedef int32_t pid_t;

typedef int32_t status_t;


#endif