// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ARCH_TASK_H__
#define __ARCH_TASK_H__

// 任务上下文结构
typedef struct task_context_s
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;

    uint64_t rbp;
    uint64_t rbx;
    uint64_t rsi;
    uint64_t rdi;
} task_context_t;

#endif /* __ARCH_TASK_H__ */