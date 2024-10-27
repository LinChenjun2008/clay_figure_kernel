/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

.text

.global asm_switch_to
asm_switch_to:
    subq $16, %rsp
    movq %rsi,8(%rsp)
    movq %rdi,(%rsp)
    leaq asm_switch_context(%rip), %rax
    callq *%rax
    addq $16, %rsp
    retq

asm_switch_context:
    pushq %rdi
    pushq %rsi
    pushq %rbx
    pushq %rbp

    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    /* 栈切换 */
    movq 72(%rsp), %rax
    movq %rsp,(%rax)
    movq 80(%rsp), %rax
    movq (%rax), %rsp

    popq %r15
    popq %r14
    popq %r13
    popq %r12

    popq %rbp
    popq %rbx
    popq %rsi
    popq %rdi

    retq


.extern intr_exit
.global asm_process_start
asm_process_start:
    movq %rdi, %rsp
    jmp intr_exit