// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

.text

.global asm_load_gdt
.type asm_load_gdt,@function
asm_load_gdt:
    lgdt (%rdi)
    movw %dx, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    pushq %rsi
    leaq next(%rip), %rax
    pushq %rax
    lretq
next:
    ret

.global asm_lidt
.type asm_lidt,@function
asm_lidt:
    lidt (%rdi)
    ret

.global asm_ltr
.type asm_ltr,@function
asm_ltr:
    ltr %di
    ret

.global io_cli
.type io_cli,@function
io_cli:
    cli
    ret

.global io_sti
.type io_sti,@function
io_sti:
    sti
    ret

.global io_hlt
.type io_hlt,@function
io_hlt:
    hlt
    ret

.global io_stihlt
.type io_stihlt,@function
io_stihlt:
    sti
    hlt
    ret

.global io_mfence
.type io_mfence,@function
io_mfence:
    mfence
    ret

.global io_in8
.type io_in8,@function
io_in8:
    movw %di, %dx
    inb (%dx), %al
    ret

.global io_in16
.type io_in16,@function
io_in16:
    movw %di, %dx
    inw (%dx), %ax
    ret

.global io_in32
.type io_in32,@function
io_in32:
    movw %di, %dx
    inl (%dx), %eax
    ret

.global io_out8
.type io_out8,@function
io_out8:
    movw %di, %dx
    movw %si, %ax
    outb %al, (%dx)
    ret

.global io_out16
.type io_out16,@function
io_out16:
    movw %di, %dx
    movw %si, %ax
    outw %ax, (%dx)
    ret

.global io_out32
.type io_out32,@function
io_out32:
    movw %di, %dx
    movl %esi, %eax
    outl %eax, (%dx)
    ret

.global get_flags
.type get_flags,@function
get_flags:
    pushf
    popq %rax
    ret

.global get_rsp
.type get_rsp,@function
get_rsp:
    movq %rsp, %rax
    ret

.global get_cr2
.type get_cr2,@function
get_cr2:
    movq %cr2, %rax
    ret

.global get_cr3
.type get_cr3,@function
get_cr3:
    movq %cr3, %rax
    ret

.global set_cr3
.type set_cr3,@function
set_cr3:
    movq %rdi, %cr3
    ret

.global rdmsr
.type rdmsr,@function
rdmsr:
    pushq %rdx
    movq %rdi, %rcx
    rdmsr
    shl $32, %rdx
    orq %rdx, %rax
    popq %rdx
    ret

.global wrmsr
.type wrmsr,@function
wrmsr:
    pushq %rcx
    movq %rdi, %rcx
    movq %rsi, %rax
    shr $32, %rsi
    movq %rsi, %rdx
    wrmsr
    popq %rcx
    ret

.global asm_cpuid
.type asm_cpuid,@function
asm_cpuid:
    pushq %rbx
    subq $16, %rsp
    movq %rdx, (%rsp)
    movq %rcx, 8(%rsp)

    movl %edi, %eax
    movl %esi, %edx
    movl %edx, %ebx
    cpuid
    movl %ebx, %esi
    movl %eax, %edi
    movq (%rsp), %rax
    movl %edi, (%rax)
    movq 8(%rsp), %rax
    movl %esi, (%rax)

    movl %ecx, (%r8)
    movl %edx, (%r9)

    addq $16, %rsp
    popq %rbx
    retq