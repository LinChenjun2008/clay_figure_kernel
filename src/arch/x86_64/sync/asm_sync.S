// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

.text

.global asm_spinlock_lock
.type asm_spinlock_lock,@function
asm_spinlock_lock:
    movq $0,%rax
    lock xchgq (%rdi),%rax
    cmpq $1,%rax
    je .done
    .wait:
    pause
    cmpq $1,(%rdi)
    jne .wait
    jmp asm_spinlock_lock
    .done:
    ret

.global asm_atomic_xchg
.type asm_atomic_xchg,@function
asm_atomic_xchg:
    lock xchgq %rsi,(%rdi)
    movq %rsi,%rax
    ret

.global asm_atomic_add
.type asm_atomic_add,@function
asm_atomic_add:
    lock addq %rsi,(%rdi)
    ret

.global asm_atomic_sub
.type asm_atomic_sub,@function
asm_atomic_sub:
    lock subq %rsi,(%rdi)
    ret

.global asm_atomic_inc
.type asm_atomic_inc,@function
asm_atomic_inc:
    lock incq (%rdi)
    ret

.global asm_atomic_dec
.type asm_atomic_dec,@function
asm_atomic_dec:
    lock decq (%rdi)
    ret

.global asm_atomic_mask
.type asm_atomic_mask,@function
asm_atomic_mask:
    lock andq %rsi,(%rdi)
    ret

.global asm_atomic_bts
.type asm_atomic_bts,@function
asm_atomic_bts:
    lock btsq %rsi,(%rdi)
    jnc bts_zero
    movq $1,%rax
    ret
    bts_zero:
    movq $0,%rax
    ret

.global asm_atomic_btr
.type asm_atomic_btr,@function
asm_atomic_btr:
    lock btrq %rsi,(%rdi)
    jnc btr_zero
    movq $1,%rax
    ret
    btr_zero:
    movq $0,%rax
    ret

.global asm_atomic_btc
.type asm_atomic_btc,@function
asm_atomic_btc:
    lock btcq %rsi,(%rdi)
    jnc btc_zero
    movq $1,%rax
    ret
    btc_zero:
    movq $0,%rax
    ret
