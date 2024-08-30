/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#ifndef __DEF_H__
#define __DEF_H__

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef PUBLIC
#define PUBLIC
#endif

#ifndef PRIVATE
#define PRIVATE static
#endif

typedef int bool;
#ifndef TRUE
#define TRUE  (1 == 1)
#endif

#ifndef FALSE
#define FALSE (1 == 0)
#endif

#define GET_FIELD(X,FIELD)       (((X) >> FIELD##_SHIFT) & FIELD##_MASK)
#define SET_FIELD(X,FIELD,VALUE) (((X) & ~(FIELD##_MASK << FIELD##_SHIFT) | ((VALUE) << FILED##_SHIFT)))

#define KERNEL_VMA_BASE           0xffff800000000000

#define KADDR_P2V(ADDR) ((void*)((addr_t)(ADDR) + KERNEL_VMA_BASE))
#define KADDR_V2P(ADDR) ((void*)((addr_t)(ADDR) - KERNEL_VMA_BASE))

#define ADDR_PML4T_INDEX_SHIFT 39
#define ADDR_PML4T_INDEX_MASK 0x1ff
#define ADDR_PDPT_INDEX_SHIFT 30
#define ADDR_PDPT_INDEX_MASK 0x1ff
#define ADDR_PDT_INDEX_SHIFT 21
#define ADDR_PDT_INDEX_MASK 0x1ff
#define ADDR_OFFSET_SHIFT 0
#define ADDR_OFFSET_MASK 0x1fffff

#define ASMLINKAGE __attribute__((sysv_abi))

#define DIV_ROUND_UP(X ,STEP) (((X) + (STEP - 1)) / STEP)

#define PADDR_AVAILABLE(ADDR) (ADDR <= 0x00007fffffffffff)

typedef signed char          int8_t;
typedef signed short         int16_t;
typedef signed int           int32_t;
typedef signed long long int int64_t;
typedef signed __int128      int128_t;

typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long int uint64_t;
typedef unsigned __int128      uint128_t;

typedef unsigned long long int uintptr_t;
typedef unsigned long long int wordsize_t;
typedef unsigned long long int size_t;

typedef unsigned long long int phy_addr_t;
typedef unsigned long long int addr_t;

typedef uint32_t pid_t;

typedef uint32_t status_t;
typedef uint32_t syscall_status_t;

typedef struct message_s
{
    volatile pid_t    src;
    volatile uint32_t type;
    union
    {
        struct
        {
            uint32_t i1;
            uint32_t i2;
            uint32_t i3;
            uint32_t i4;
        } m1;

        struct
        {
            void* p1;
            void* p2;
            void* p3;
            void* p4;
        } m2;

        struct
        {
            uint32_t  i1;
            uint32_t  i2;
            uint32_t  i3;
            uint32_t  i4;
            uint64_t  l1;
            uint64_t  l2;
            void     *p1;
            void     *p2;
        } m3;
    };
} message_t;

#endif