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

.text

.global asm_load_gdt
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
asm_lidt:
    lidt (%rdi)
    ret

.global asm_ltr
asm_ltr:
    ltr %di
    ret

.global io_cli
io_cli:
    cli
    ret

.global io_sti
io_sti:
    sti
    ret

.global io_hlt
io_hlt:
    hlt
    ret

.global io_stihlt
io_stihlt:
    sti
    hlt
    ret

.global io_mfence
io_mfence:
    mfence
    ret

.global io_in8
io_in8:
    movw %di, %dx
    inb (%dx), %al
    ret

.global io_in16
io_in16:
    movw %di, %dx
    inw (%dx), %ax
    ret

.global io_in32
io_in32:
    movw %di, %dx
    inl (%dx), %eax
    ret

.global io_out8
io_out8:
    movw %di, %dx
    movw %si, %ax
    outb %al, (%dx)
    ret

.global io_out16
io_out16:
    movw %di, %dx
    movw %si, %ax
    outw %ax, (%dx)
    ret

.global io_out32
io_out32:
    movw %di, %dx
    movl %esi, %eax
    outl %eax, (%dx)
    ret

.global get_flags
get_flags:
    pushf
    popq %rax
    ret

.global get_rsp
get_rsp:
    movq %rsp, %rax
    ret

.global get_cr2
get_cr2:
    movq %cr2, %rax
    ret

.global get_cr3
get_cr3:
    movq %cr3, %rax
    ret

.global set_cr3
set_cr3:
    movq %rdi, %cr3
    ret

.global rdmsr
rdmsr:
    pushq %rdx
    movq %rdi, %rcx
    rdmsr
    shl $32, %rdx
    orq %rdx, %rax
    popq %rdx
    ret

.global wrmsr
wrmsr:
    pushq %rcx
    movq %rdi, %rcx
    movq %rsi, %rax
    shr $32, %rsi
    movq %rsi, %rdx
    wrmsr
    popq %rcx
    ret

/*
.global asm_cpuid
asm_cpuid:
    pushq %rbx
    subq $44, %rsp
    movl %edi, (%rsp)
    movl %esi, 4(%rsp)
    movq %rdx, 12(%rsp)
    movq %rcx, 20(%rsp)
    movq %r8, 28(%rsp)
    movq %r9, 36(%rsp)

    movl (%rsp), %eax
    movl 4(%rsp), %edx
    movl %edx, %ebx
    cpuid
    movl %ebx, %esi
    movl %eax, %edi
    movq 12(%rsp), %rax
    movl %edi, (%rax)
    movq 20(%rsp), %rax
    movl %esi, (%rax)
    movq 28(%rsp), %rax
    movl %ecx, (%rax)
    movq 36(%rsp), %rax
    movl %edx, (%rax)

    addq $44, %rsp
    popq %rbx
    retq
    */

.global asm_cpuid
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