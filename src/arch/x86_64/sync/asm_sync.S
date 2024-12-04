/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

.text

.global asm_spinlock_lock
asm_spinlock_lock:
    lock decq (%rdi)
    jz .down
    .wait:
    pause
    cmpq $1,(%rdi)
    jne .wait
    jmp asm_spinlock_lock
    .down:
    ret


.global asm_atomic_add
asm_atomic_add:
    lock addq %rsi,(%rdi)
    ret

.global asm_atomic_sub
asm_atomic_sub:
    lock subq %rsi,(%rdi)
    ret

.global asm_atomic_inc
asm_atomic_inc:
    lock incq (%rdi)
    ret

.global asm_atomic_dec
asm_atomic_dec:
    lock decq (%rdi)
    ret

.global asm_atomic_mask
asm_atomic_mask:
    lock andq %rsi,(%rdi)
    ret