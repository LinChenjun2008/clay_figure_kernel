/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

.text

.global sse_enable
.type sse_enable,@function
sse_enable:
    movq %cr0, %rax
    andw $0xfffb, %ax
    orw $0x2, %ax
    movq %rax, %cr0
    movq %cr4, %rax
    orw $(3 << 9), %ax
    movq %rax, %cr4
    ret

.global sse_init
.type sse_init,@function
sse_init:
    subq $16,%rsp
    movq $0x1f80,(%rsp)
    ldmxcsr (%rsp)
    addq $16,%rsp
    ret

.global asm_fxsave
.type asm_fxsave,@function
asm_fxsave:
    fxsave (%rdi)
    ret

.global asm_fxrstor
.type asm_fxrstor,@function
asm_fxrstor:
    fxrstor (%rdi)
    ret