// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __X86_TSS_H__
#define __X86_TSS_H__

#include <task.h>

#pragma pack(1)
typedef struct
{
    uint32_t reserved1;

    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;

    uint32_t reserved2;
    uint32_t reserved3;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint32_t reserved4;
    uint32_t reserved5;

    uint32_t io_map;
} tss64_t;
#pragma pack()

void tss_init(void);
void update_tss_rsp0(task_struct_t *task);

#endif /* __X86_TSS_H__ */