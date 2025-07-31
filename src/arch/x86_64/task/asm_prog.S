// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/const.h>

.text

.global asm_switch_to_user
.type asm_switch_to_user,@function
asm_switch_to_user:
    movq %rdx, %rsp
    movq $SELECTOR_DATA64_U,%rax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    pushq %rax               // ss
    pushq %rcx               // rsp
    pushq %r8                // rflags
    pushq $SELECTOR_CODE64_U // cs
    pushq %rdi               // rip
    xchgq %rdi,%rsi          // the parameter of the function in user mode
    iretq
