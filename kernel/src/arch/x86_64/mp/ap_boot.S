// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <asm/desc.h>
#include <asm/page.h>
#include <asm/mp.h>

.balign 0x1000
.text
.code16

// 0x10000
.global AP_BOOT_START
AP_BOOT_START:
    cli
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    mov %cs, %ax
    mov  $0, %ebp
    mov %ax, %bp
    shll $4, %ebp

    leal (AP_CODE32_BASE - AP_BOOT_START)(%ebp), %eax
    movl                                   %eax, CODE32_VECTOR - AP_BOOT_START

    leal (AP_CODE64_BASE - AP_BOOT_START)(%ebp), %eax
    movl                                   %eax, CODE64_VECTOR - AP_BOOT_START

    leal (GDT - AP_BOOT_START)(%ebp), %eax
    movl                        %eax, (GDT_PTR - AP_BOOT_START + 2)

    lgdtl GDT_PTR - AP_BOOT_START

    movl %cr0, %eax
    bts    $0, %eax
    movl %eax, %cr0

    ljmpl *(CODE32_VECTOR - AP_BOOT_START)

.balign 4
.code32
AP_CODE32_BASE:
    mov $0x10, %ax
    mov   %ax, %ds
    mov   %ax, %es
    mov   %ax, %ss
    mov   %ax, %fs
    mov   %ax, %gs

    // PAE
    movl %cr4, %eax
    bts    $5, %eax
    movl %eax, %cr4

    // page table
    movl AP_PAGE_TABLE_PTR, %eax
    movl              %eax, %cr3

    // enable long mode
    movl $0xc0000080, %ecx
    rdmsr
    bts $8, %eax
    wrmsr

    movl %cr0, %eax
    bts    $0, %eax
    bts   $31, %eax
    mov  %eax, %cr0

    ljmp *(CODE64_VECTOR - AP_BOOT_START)(%ebp)

.balign 4
.code64
AP_CODE64_BASE:
    movq $SELECTOR_KERNEL_DATA64, %rax
    movq                    %rax, %ds
    movq                    %rax, %es
    movq                    %rax, %fs
    movq                    %rax, %gs
    movq                    %rax, %ss

.ap_flag_loop:
    movq AP_BOOT_FLAG, %rax
    cmpq           $0, %rax
    jz .ap_loop
    lock incq AP_BOOT_FLAG

.ap_stack_loop:
    movq $0, %rdi
    lock xchgq AP_STACK, %rdi
    cmpq $0, %rdi
    jz .ap_stack_loop
    movq %rdi, %rsp

.ap_loop:
    movq AP_ENTRY, %rax
    cmpq       $0, %rax
    jz .ap_loop

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
    .long GDT - AP_BOOT_START
    .short 0

.balign 4
CODE32_VECTOR:
    .long AP_CODE32_BASE - AP_BOOT_START
    .word 0x08,0

.balign 4
CODE64_VECTOR:
    .long AP_CODE64_BASE - AP_BOOT_START
    .word 0x18,0

.global AP_BOOT_END
AP_BOOT_END:
