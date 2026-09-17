// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_TYPES_SIGNAL_H__
#define __ASM_TYPES_SIGNAL_H__

struct sigcontext
{
    word_t r15;
    word_t r14;
    word_t r13;
    word_t r12;
    word_t r11;
    word_t r10;
    word_t r9;
    word_t r8;

    word_t rdi;
    word_t rsi;
    word_t rbp;
    word_t rbx;
    word_t rdx;
    word_t rax;
    word_t rcx;

    word_t rip;
    word_t rflags;
    word_t rsp;

    word_t cs;
    word_t ss;
    word_t ds;
    word_t es;
    word_t fs;
    word_t gs;
};

#endif /* __ASM_TYPES_SIGNAL_H__ */