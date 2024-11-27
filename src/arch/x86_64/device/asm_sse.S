/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

.text

.global sse_init
sse_init:
    movq %cr0, %rax
    andw $0xfffb, %ax
    orw $0x2, %ax
    movq %rax, %cr0
    movq %cr4, %rax
    orw $(3 << 9), %ax
    movq %rax, %cr4
    ret

.global asm_fxsave
asm_fxsave:
    fxsave (%rdi)
    ret

.global asm_fxrstor
asm_fxrstor:
    fxrstor (%rdi)
    ret