// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __ASM_PTRACE_H__
#define __ASM_PTRACE_H__

#define REG_DS 0x00
#define REG_ES 0x08
#define REG_FS 0x10
#define REG_GS 0x18

#define REG_RAX 0x20
#define REG_RBX 0x28
#define REG_RCX 0x30
#define REG_RDX 0x38
#define REG_RBP 0x40
#define REG_RSI 0x48
#define REG_RDI 0x50

#define REG_R8  0x58
#define REG_R9  0x60
#define REG_R10 0x68
#define REG_R11 0x70
#define REG_R12 0x78
#define REG_R13 0x80
#define REG_R14 0x88
#define REG_R15 0x90

#define REG_INT_VECTOR 0x98
#define REG_ERROR_CODE 0xa0
#define REG_RIP        0xa8
#define REG_CS         0xb0
#define REG_RFLAGS     0xb8
#define REG_RSP        0xc0
#define REG_SS         0xc8

#define REGS_SIZE 0xd0

#ifndef __ASSEMBLER__
#    pragma pack(1)

struct pt_regs
{
    uint64_t ds;
    uint64_t es;
    uint64_t fs;
    uint64_t gs;

    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;

    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t int_vector;

    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

struct task_context
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;

    uint64_t rbp;
    uint64_t rbx;
    uint64_t rsi;
    uint64_t rdi;
};

#    pragma pack()

#endif /* __ASSEMBLER__ */

#endif /* __ASM_PTRACE_H__ */