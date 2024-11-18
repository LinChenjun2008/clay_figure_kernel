/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU Lesser General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 宽通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 宽通用公共许可证，了解详情。

你应该随程序获得一份 GNU 宽通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#include <kernel/const.h>

.balign 0x1000
.text
.code16

// 0x7c00
.global AP_BOOT_BASE
AP_BOOT_BASE:
    cli
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    mov $0x7c00, %sp

    mov %cs, %ax
    mov $0, %ebp
    mov %ax, %bp
    shll $4, %ebp

    leal (AP_CODE32_BASE - AP_BOOT_BASE)(%ebp), %eax
    movl %eax,CODE32_VECTOR - AP_BOOT_BASE

    leal (AP_CODE64_BASE - AP_BOOT_BASE)(%ebp), %eax
    movl %eax,CODE64_VECTOR - AP_BOOT_BASE

    leal (GDT - AP_BOOT_BASE)(%ebp), %eax
    movl %eax,(GDT_PTR - AP_BOOT_BASE + 2)

    lgdtl GDT_PTR - AP_BOOT_BASE

    movl %cr0, %eax
    bts $0, %eax
    movl %eax, %cr0

    ljmpl *(CODE32_VECTOR - AP_BOOT_BASE)

.balign 4
.code32
AP_CODE32_BASE:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    mov $0x7c00, %esp

    // PAE
    movl %cr4, %eax
    bts $5, %eax
    movl %eax, %cr4

    // page table
    movl $KERNEL_PAGE_DIR_TABLE_POS, %eax
    movl %eax, %cr3

    // enable long mode
    movl $0xc0000080, %ecx
    rdmsr
    bts $8, %eax
    wrmsr

    movl %cr0, %eax
    bts $0, %eax
    bts $31, %eax
    mov %eax, %cr0

    ljmp *(CODE64_VECTOR - AP_BOOT_BASE)(%ebp)


.balign 4
.code64
AP_CODE64_BASE:
    movq $0x20, %rax
    movq %rax, %ds
    movq %rax, %es
    movq %rax, %fs
    movq %rax, %gs
    movq %rax, %ss

    movq $1, %rax
    movq $0, %rbx
    cpuid
    shrq $24, %rbx
    shlq $16, %rbx
    movq $AP_STACK_BASE_PTR, %rax
    addq (%rax), %rbx
    movq $0xffff800000000000, %rax
    addq %rax, %rbx
    movq %rbx, %rsp

    movq $0xffff800000100000, %rax
    jmpq *%rax


.balign 4
GDT:
    .quad 0
    .quad 0x00cf9a000000ffff
    .quad 0x00cf92000000ffff
    .quad 0x0020980000000000
    .quad 0x0000920000000000
GDT_END:

.balign 4
GDT_PTR:
    .short  GDT_END - GDT - 1
    .long GDT - AP_BOOT_BASE
    .short 0

.balign 4
CODE32_VECTOR:
    .long AP_CODE32_BASE - AP_BOOT_BASE
    .word 0x08,0

.balign 4
CODE64_VECTOR:
    .long AP_CODE64_BASE - AP_BOOT_BASE
    .word 0x18,0

.global AP_BOOT_END
AP_BOOT_END: