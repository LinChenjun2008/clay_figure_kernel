// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2026 Lin Chenjun
 */

.text

.global arch_load_gdt
.type arch_load_gdt,@function
arch_load_gdt:
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
    retq

.global arch_lidt
.type arch_lidt,@function
arch_lidt:
    lidt (%rdi)
    retq

.global arch_ltr
.type arch_ltr,@function
arch_ltr:
    ltr %di
    retq

.global io_cli
.type io_cli,@function
io_cli:
    cli
    retq

.global io_sti
.type io_sti,@function
io_sti:
    sti
    retq

.global io_hlt
.type io_hlt,@function
io_hlt:
    hlt
    retq

.global io_stihlt
.type io_stihlt,@function
io_stihlt:
    sti
    hlt
    retq

.global io_lfence
.type io_lfence,@function
io_lfence:
    lfence
    retq

.global io_sfence
.type io_sfence,@function
io_sfence:
    sfence
    retq

.global io_mfence
.type io_mfence,@function
io_mfence:
    mfence
    retq

.global io_in8
.type io_in8,@function
io_in8:
    movw  %di, %dx
    inb (%dx), %al
    retq

.global io_in16
.type io_in16,@function
io_in16:
    movw  %di, %dx
    inw (%dx), %ax
    retq

.global io_in32
.type io_in32,@function
io_in32:
    movw  %di, %dx
    inl (%dx), %eax
    retq

.global io_out8
.type io_out8,@function
io_out8:
    movw %di, %dx
    movw %si, %ax
    outb %al, (%dx)
    retq

.global io_out16
.type io_out16,@function
io_out16:
    movw %di, %dx
    movw %si, %ax
    outw %ax, (%dx)
    retq

.global io_out32
.type io_out32,@function
io_out32:
    movw  %di, %dx
    movl %esi, %eax
    outl %eax, (%dx)
    retq

.global get_flags
.type get_flags,@function
get_flags:
    pushf
    popq %rax
    retq

.global get_rsp
.type get_rsp,@function
get_rsp:
    movq %rsp, %rax
    retq

.global get_cr0
.type get_cr0,@function
get_cr0:
    movq %cr0, %rax
    retq

.global get_cr2
.type get_cr2,@function
get_cr2:
    movq %cr2, %rax
    retq

.global get_cr3
.type get_cr3,@function
get_cr3:
    movq %cr3, %rax
    retq

.global set_cr3
.type set_cr3,@function
set_cr3:
    movq %rdi, %cr3
    retq

.global get_cr4
.type get_cr4,@function
get_cr4:
    movq %cr4, %rax
    retq

.global rdmsr
.type rdmsr,@function
rdmsr:
    pushq %rdx
    movq %rdi, %rcx
    rdmsr
    shl  $32, %rdx
    orq %rdx, %rax
    popq %rdx
    retq

.global wrmsr
.type wrmsr,@function
wrmsr:
    pushq %rcx
    movq %rdi, %rcx
    movq %rsi, %rax
    shr   $32, %rsi
    movq %rsi, %rdx
    wrmsr
    popq %rcx
    retq

.global arch_cpuid
.type arch_cpuid,@function
arch_cpuid:
    pushq %rbx
    subq  $16, %rsp
    movq %rdx, (%rsp)
    movq %rcx, 8(%rsp)

    movl %edi, %eax
    movl %esi, %edx
    movl %edx, %ebx
    cpuid
    movl    %ebx, %esi
    movl    %eax, %edi
    movq  (%rsp), %rax
    movl    %edi, (%rax)
    movq 8(%rsp), %rax
    movl    %esi, (%rax)

    movl %ecx, (%r8)
    movl %edx, (%r9)

    addq $16, %rsp
    popq %rbx
    retq

.global arch_atomic_xchg
.type arch_atomic_xchg,@function
arch_atomic_xchg:
    lock xchgq %rsi, (%rdi)
    movq       %rsi, %rax
    retq

.global arch_atomic_add
.type arch_atomic_add,@function
arch_atomic_add:
    lock xaddq %rsi, (%rdi)
    movq       %rsi, %rax
    retq

.global arch_atomic_sub
.type arch_atomic_sub,@function
arch_atomic_sub:
    negq %rsi
    lock xaddq %rsi, (%rdi)
    movq       %rsi, %rax
    retq

.global arch_atomic_inc
.type arch_atomic_inc,@function
arch_atomic_inc:
    movq         $1, %rsi
    lock xaddq %rsi, (%rdi)
    movq %rsi, %rax
    retq

.global arch_atomic_dec
.type arch_atomic_dec,@function
arch_atomic_dec:
    movq        $-1, %rsi
    lock xaddq %rsi, (%rdi)
    movq %rsi, %rax
    retq

.global arch_atomic_bts
.type arch_atomic_bts,@function
arch_atomic_bts:
    lock btsq %rsi, (%rdi)
    jnc bts_zero
    movq $1, %rax
    retq
    bts_zero:
    movq $0, %rax
    retq

.global arch_atomic_btr
.type arch_atomic_btr,@function
arch_atomic_btr:
    lock btrq %rsi, (%rdi)
    jnc btr_zero
    movq $1, %rax
    retq
    btr_zero:
    movq $0, %rax
    retq

.global arch_atomic_btc
.type arch_atomic_btc,@function
arch_atomic_btc:
    lock btcq %rsi, (%rdi)
    jnc btc_zero
    movq $1, %rax
    retq
    btc_zero:
    movq $0, %rax
    retq

.global arch_pause
.type arch_pause,@function
arch_pause:
    pause
    retq

.global arch_panic
.type arch_panic,@function
arch_panic:
    int $0xff
    retq