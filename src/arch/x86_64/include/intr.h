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

#ifndef __INTR_H__
#define __INIT_H__

#ifndef INTR_HANDLER

typedef struct intr_stack_s
{
    /* 低地址 */
    wordsize_t ds;
    wordsize_t es;
    wordsize_t fs;
    wordsize_t gs;

    wordsize_t rax;
    wordsize_t rbx;
    wordsize_t rcx;
    wordsize_t rdx;
    wordsize_t rbp;
    wordsize_t rsi;
    wordsize_t rdi;

    wordsize_t r8;
    wordsize_t r9;
    wordsize_t r10;
    wordsize_t r11;
    wordsize_t r12;
    wordsize_t r13;
    wordsize_t r14;
    wordsize_t r15;

    wordsize_t error_code;
    void       (*rip)(void);
    wordsize_t cs;
    wordsize_t rflags;
    wordsize_t rsp;
    wordsize_t ss;
    /* 高地址 */
} intr_stack_t;

typedef enum intr_status_e
{
    INTR_OFF = 0,
    INTR_ON,
} intr_status_t;

PUBLIC void ASMLINKAGE do_irq(uint8_t nr,intr_stack_t *stack);
PUBLIC void intr_init();
PUBLIC void ap_intr_init();
PUBLIC void register_handle(uint8_t nr,void (*handle)(intr_stack_t*));
PUBLIC intr_status_t intr_get_status();
PUBLIC intr_status_t intr_set_status(intr_status_t status);
PUBLIC intr_status_t intr_enable();
PUBLIC intr_status_t intr_disable();
#else
INTR_HANDLER(asm_intr0x00_handler,0x00, pushq $0)
INTR_HANDLER(asm_intr0x01_handler,0x01, pushq $0)
INTR_HANDLER(asm_intr0x02_handler,0x02, pushq $0)
INTR_HANDLER(asm_intr0x03_handler,0x03, pushq $0)
INTR_HANDLER(asm_intr0x04_handler,0x04, pushq $0)
INTR_HANDLER(asm_intr0x05_handler,0x05, pushq $0)
INTR_HANDLER(asm_intr0x06_handler,0x06, pushq $0)
INTR_HANDLER(asm_intr0x07_handler,0x07, pushq $0)
INTR_HANDLER(asm_intr0x08_handler,0x08,      nop)
INTR_HANDLER(asm_intr0x09_handler,0x09, pushq $0)
INTR_HANDLER(asm_intr0x0a_handler,0x0a,      nop)
INTR_HANDLER(asm_intr0x0b_handler,0x0b,      nop)
INTR_HANDLER(asm_intr0x0c_handler,0x0c, pushq $0)
INTR_HANDLER(asm_intr0x0d_handler,0x0d,      nop)
INTR_HANDLER(asm_intr0x0e_handler,0x0e,      nop)
INTR_HANDLER(asm_intr0x0f_handler,0x0f, pushq $0)
INTR_HANDLER(asm_intr0x10_handler,0x10, pushq $0)
INTR_HANDLER(asm_intr0x11_handler,0x11,      nop)
INTR_HANDLER(asm_intr0x12_handler,0x12, pushq $0)
INTR_HANDLER(asm_intr0x13_handler,0x13, pushq $0)
INTR_HANDLER(asm_intr0x14_handler,0x14, pushq $0)
INTR_HANDLER(asm_intr0x15_handler,0x15, pushq $0)
INTR_HANDLER(asm_intr0x16_handler,0x16, pushq $0)
INTR_HANDLER(asm_intr0x17_handler,0x17, pushq $0)
INTR_HANDLER(asm_intr0x18_handler,0x18,      nop)
INTR_HANDLER(asm_intr0x19_handler,0x19, pushq $0)
INTR_HANDLER(asm_intr0x1a_handler,0x1a,      nop)
INTR_HANDLER(asm_intr0x1b_handler,0x1b,      nop)
INTR_HANDLER(asm_intr0x1c_handler,0x1c, pushq $0)
INTR_HANDLER(asm_intr0x1d_handler,0x1d,      nop)
INTR_HANDLER(asm_intr0x1e_handler,0x1e,      nop)
INTR_HANDLER(asm_intr0x1f_handler,0x1f, pushq $0)

INTR_HANDLER(asm_intr0x20_handler,0x20, pushq $0)
INTR_HANDLER(asm_intr0x21_handler,0x21, pushq $0)
INTR_HANDLER(asm_intr0x22_handler,0x22, pushq $0)
INTR_HANDLER(asm_intr0x23_handler,0x23, pushq $0)
INTR_HANDLER(asm_intr0x24_handler,0x24, pushq $0)
INTR_HANDLER(asm_intr0x25_handler,0x25, pushq $0)
INTR_HANDLER(asm_intr0x26_handler,0x26, pushq $0)
INTR_HANDLER(asm_intr0x27_handler,0x27, pushq $0)
INTR_HANDLER(asm_intr0x28_handler,0x28, pushq $0)
INTR_HANDLER(asm_intr0x29_handler,0x29, pushq $0)
INTR_HANDLER(asm_intr0x2a_handler,0x2a, pushq $0)
INTR_HANDLER(asm_intr0x2b_handler,0x2b, pushq $0)
INTR_HANDLER(asm_intr0x2c_handler,0x2c, pushq $0)
INTR_HANDLER(asm_intr0x2d_handler,0x2d, pushq $0)
INTR_HANDLER(asm_intr0x2e_handler,0x2e, pushq $0)
INTR_HANDLER(asm_intr0x2f_handler,0x2f, pushq $0)

INTR_HANDLER(asm_intr0x30_handler,IRQ_XHCI,pushq $0)

// IPI
INTR_HANDLER(asm_intr0x80_handler,0x80, pushq $0) // Timer
INTR_HANDLER(asm_intr0x81_handler,0x81, pushq $0) // Kernel Panic

#endif

#endif