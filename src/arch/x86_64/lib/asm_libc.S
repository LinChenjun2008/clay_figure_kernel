/*
   Copyright 2024 LinChenjun

   本文件是Clay Figure Kernel的一部分。
   修改和/或再分发遵循GNU GPL version 3 (or any later version)
  
*/

.text

.global memset
memset:
    pushq %rcx
    movw %si, %ax
    movq %rdx, %rcx
    cld
    rep stosb
    popq %rcx
    ret

.global memcpy
memcpy:
    movq %rsi, %rax
    pushq %rcx
    movq %rdx, %rcx
    cld
    rep movsb
    popq %rcx
    ret

.global memcmp
memcmp:
    pushq %rcx
    movq %rdx, %rcx
    movl $0, %eax

    cld
    repe cmpsb
    je .memcmp_finish
    movl $1, %eax
    jl .memcmp_finish
    negl %eax

    .memcmp_finish:
    popq %rcx
    ret

.global strcpy
strcpy:
    pushq %rdi
    cld
    .strcpy_loop:
    lodsb
    stosb
    testb %al, %al
    jne .strcpy_loop
    popq %rax
    ret

.global strncpy
strncpy:
    movq %rdi, %rax
    pushq %rcx
    movq %rdx, %rcx
    cld
    .strncpy_loop:
    decq %rcx
    js .strncpy_finish
    lodsb
    stosb
    testb %al, %al
    jne .strncpy_loop
    rep stosb
    .strncpy_finish:
    popq %rcx
    ret

.global strcmp
strcmp:
    cld
    .strcmp_loop:
    lodsb
    scasb
    jne .strcmp_label

    testb %al, %al
    jne .strcmp_loop

    xorl %eax, %eax
    jmp .strcmp_return

    .strcmp_label:
    movl $1, %eax
    jl .strcmp_return

    negl %eax
    .strcmp_return:
    ret


.global strncmp
strncmp:
    pushq %rcx
    movq %rdx, %rcx
    cld
    .strncmp_loop:
    decq %rcx
    je .strncmp_equal

    lodsb
    scasb
    jne .strncmp_label

    testb %al, %al
    jne .strncmp_loop

    .strncmp_equal:
    xorl %eax, %eax
    jmp .strncmp_return

    .strncmp_label:
    movl $1, %eax
    jl .strncmp_return

    negl %eax
    .strncmp_return:
    popq %rcx
    ret


.global strlen
strlen:
    pushq %rcx
    movq $-1, %rcx
    movq $0, %rax
    cld
    repne scasb
    notq %rcx
    decq %rcx
    movq %rcx, %rax
    popq %rcx
    ret