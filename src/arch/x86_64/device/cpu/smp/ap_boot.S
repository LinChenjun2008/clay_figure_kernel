// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <kernel/const.h>
#include <mem/page.h>

.balign 0x1000
.text
.code16

// 0x10000
.global AP_BOOT_BASE
AP_BOOT_BASE:
    cli
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

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
    movq $KERNEL_VMA_BASE, %rax
    addq %rax, %rbx
    movq %rbx, %rsp

    movq $AP_START_FLAG, %rax
    lock incq (%rax)

    movq $(KERNEL_TEXT_BASE + 0x100000), %rax
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