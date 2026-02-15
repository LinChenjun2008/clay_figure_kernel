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

#endif /* __X86_H__ */