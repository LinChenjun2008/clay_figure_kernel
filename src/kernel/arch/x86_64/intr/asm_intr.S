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
movq  $0,%rax; \
movw  %gs,%ax; \
pushq %rax; \
movw  %fs,%ax; \
pushq %rax; \
movw  %es,%ax; \
pushq %rax; \
movw  %ds,%ax; \
pushq %rax; \
\
movq $NR,%rdi; \
movq %rsp,%rsi; \
\
leaq do_irq(%rip),%rax; \
callq *%rax; \
leaq intr_exit(%rip),%rax; \
jmpq *%rax; \

#include <intr.h>

.global intr_exit
intr_exit:

popq %rax
movw %ax,%ds
popq %rax
movw %ax,%es
popq %rax
movw %ax,%fs
popq %rax
movw %ax,%gs

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

addq $8,%rsp
iretq