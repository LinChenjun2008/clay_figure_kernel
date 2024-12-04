/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/const.h>

#define INTR_HANDLER(ENTRY,NR,ERROR_CODE) \
.global ENTRY; \
ENTRY:; \
ERROR_CODE; \
\
pushq %r15; \
pushq %r14; \
pushq %r13; \
pushq %r12; \
pushq %r11; \
pushq %r10; \
pushq %r9; \
pushq %r8; \
\
pushq %rdi; \
pushq %rsi; \
pushq %rbp; \
pushq %rdx; \
pushq %rcx; \
pushq %rbx; \
pushq %rax; \
\
movq  $0, %rax; \
movw  %gs, %ax; \
pushq %rax; \
movw  %fs, %ax; \
pushq %rax; \
movw  %es, %ax; \
pushq %rax; \
movw  %ds, %ax; \
pushq %rax; \
\
movq $NR, %rdi; \
movq %rsp, %rsi; \
\
leaq do_irq(%rip), %rax; \
callq *%rax; \
leaq intr_exit(%rip), %rax; \
jmpq *%rax; \

.text

#include <intr.h>

.global intr_exit
intr_exit:

popq %rax
movw %ax, %ds
popq %rax
movw %ax, %es
popq %rax
movw %ax, %fs
popq %rax
movw %ax, %gs

popq %rax
popq %rbx
popq %rcx
popq %rdx
popq %rbp
popq %rsi
popq %rdi

popq %r8
popq %r9
popq %r10
popq %r11
popq %r12
popq %r13
popq %r14
popq %r15

addq $8, %rsp
iretq