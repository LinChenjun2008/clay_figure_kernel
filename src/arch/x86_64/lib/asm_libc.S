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