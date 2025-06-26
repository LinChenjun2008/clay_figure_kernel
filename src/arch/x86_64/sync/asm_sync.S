/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

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