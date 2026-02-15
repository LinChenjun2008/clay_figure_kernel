// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __X86_H__
#define __X86_H__

#define IA32_APIC_BASE        0x0000001b
#define IA32_APIC_BASE_BSP    (1 << 8)
#define IA32_APIC_BASE_ENABLE (1 << 11)

#define IA32_EFER     0xc0000080
#define IA32_EFER_SCE 1

#define IA32_STAR  0xc0000081
#define IA32_LSTAR 0xc0000082
#define IA32_FMASK 0xc0000084

#define IA32_FS_BASE        0xc0000100
#define IA32_GS_BASE        0xc0000101
#define IA32_KERNEL_GS_BASE 0xc0000102

#define EFLAGS_MBS    (1 << 1)
#define EFLAGS_IF_1   (1 << 9)
#define EFLAGS_IF_0   (0 << 9)
#define EFLAGS_IOPL_0 (0 << 12)
#define EFLAGS_IOPL_3 (3 << 12)

#ifndef __ASSEMBLER__

#    pragma pack(1)

typedef struct
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
} pt_regs_t;
#    pragma pack()

#endif /* __ASSEMBLER__ */

#endif /* __X86_H__ */