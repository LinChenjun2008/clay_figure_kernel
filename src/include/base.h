// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __BASE_H__
#define __BASE_H__

#include "errno.h"
#include <std/stddef.h>
#include <std/stdint.h>

#define TRUE  (1 == 1)
#define FALSE (1 == 0)

#define OFFSET(CONTAINER_TYPE, MEMBER_NAME) \
    (uint64_t)(&((CONTAINER_TYPE *)0)->MEMBER_NAME)

#define CONTAINER_OF(CONTAINER_TYPE, MEMBER_NAME, MEMBER_PTR) \
    ((CONTAINER_TYPE *)((uintptr_t)MEMBER_PTR -               \
                        OFFSET(CONTAINER_TYPE, MEMBER_NAME)))

#define GET_FIELD(X, FIELD) (((X) >> FIELD##_SHIFT) & FIELD##_MASK)
#define SET_FIELD(X, FIELD, VALUE)              \
    (((X) & ~(FIELD##_MASK << FIELD##_SHIFT)) | \
     (((VALUE) & (FIELD##_MASK)) << FIELD##_SHIFT))

#define SIGNATURE_32(A, B, C, D) ((D) << 24 | (C) << 16 | (B) << 8 | (A))

#define SYSV_ABI       __attribute__((sysv_abi))
#define WEAK           __attribute__((weak))
#define ALIGNED(ALIGN) __attribute__((aligned(ALIGN)))

#ifndef STATIC_ASSERT
#    define STATIC_ASSERT(CONDITION, MESSAGE) _Static_assert(CONDITION, MESSAGE)
#endif

#define DIV_ROUND_UP(X, STEP) (((X) + (STEP - 1)) / STEP)

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define UNUSED(X) ((void)(X))

#define BARRIER() __asm__ __volatile__("" ::: "memory")

typedef unsigned char char8_t;
typedef unsigned short char16_t;

struct graphic_info
{
    uintptr_t frame_buffer_base;
    uint32_t  horizontal_resolution;
    uint32_t  vertical_resolution;
    uint32_t  pixel_per_scanline;
};

struct memory_map
{
    uint64_t map_size;
    void    *buffer;
    uint64_t map_key;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
};

struct boot_info
{
    void  *initramfs;
    size_t initramfs_size;

    void     *page_table_pos;
    uintptr_t relocate_base; // 重定位后的基地址

    uintptr_t stack_base;
    size_t    stack_pages;

    struct memory_map   memory_map;   // 内存描述符
    struct graphic_info graphic_info; // 图形信息

    uintptr_t **sdt_baseaddr_array;
    uint32_t    sdt_entries;
};

STATIC_ASSERT(
    OFFSET(struct boot_info, page_table_pos) == 16,
    "struct boot_info: offset of 'page_table_pos' not 16."
);

STATIC_ASSERT(
    OFFSET(struct boot_info, relocate_base) == 24,
    "struct boot_info: offset of 'relocate_base' not 24."
);

#endif /* __BASE_H__ */