// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __BASE_H__
#define __BASE_H__

#define NULL ((void *)0)

#define TRUE  (1 == 1)
#define FALSE (1 == 0)

#define OFFSET_OF(CONTAINER_TYPE, MEMBER_NAME) \
    (uint64_t)(&((CONTAINER_TYPE *)0)->MEMBER_NAME)

#define CONTAINER_OF(CONTAINER_TYPE, MEMBER_NAME, MEMBER_PTR) \
    ((CONTAINER_TYPE *)((uintptr_t)MEMBER_PTR -               \
                        OFFSET_OF(CONTAINER_TYPE, MEMBER_NAME)))

#define GET_FIELD(X, FIELD) (((X) >> FIELD##_SHIFT) & FIELD##_MASK)
#define SET_FIELD(X, FIELD, VALUE)              \
    (((X) & ~(FIELD##_MASK << FIELD##_SHIFT)) | \
     (((VALUE) & (FIELD##_MASK)) << FIELD##_SHIFT))

#define SIGNATURE_32(A, B, C, D) ((D) << 24 | (C) << 16 | (B) << 8 | (A))

#define SYSV_ABI       __attribute__((sysv_abi))
#define WEAK           __attribute__((weak))
#define ALIGNED(ALIGN) __attribute__((aligned(ALIGN)))

#define ASMLINKAGE SYSV_ABI

#define DIV_ROUND_UP(X, STEP) (((X) + (STEP - 1)) / STEP)

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define UNUSED(X) ((void)(X))

#define ALIGN_PAD(X, ALIGN) (((ALIGN) - ((X) & ((ALIGN) - 1))) & ((ALIGN) - 1))

#define BI_INITRAMFS      0
#define BI_INITRAMFS_SIZE 8
#define BI_PAGE_TABLE_POS 16
#define BI_RELOCATE_BASE  24
#define BI_STACK_BASE     32
#define BI_STACK_PAGES    40

#ifndef __ASSEMBLER__

#    include <types.h>

struct system_info;

int main(struct system_info *system_info);
int ap_main(struct system_info *system_info, uintptr_t stack);

#endif /* __ASSEMBLER__ */

#endif /* __BASE_H__ */