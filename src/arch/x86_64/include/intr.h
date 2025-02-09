/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

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

    wordsize_t nr;

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

PUBLIC void ASMLINKAGE do_irq(intr_stack_t *stack);
PUBLIC void intr_init();
PUBLIC void ap_intr_init();
PUBLIC void register_handle(uint8_t nr,void (*handle)(intr_stack_t*));
PUBLIC intr_status_t intr_get_status();
PUBLIC intr_status_t intr_set_status(intr_status_t status);
PUBLIC intr_status_t intr_enable();
PUBLIC intr_status_t intr_disable();

#else

#define CODE 0
#define ZERO 8

INTR_HANDLER(asm_intr0x00_handler,0x00, ZERO)
INTR_HANDLER(asm_intr0x01_handler,0x01, ZERO)
INTR_HANDLER(asm_intr0x02_handler,0x02, ZERO)
INTR_HANDLER(asm_intr0x03_handler,0x03, ZERO)
INTR_HANDLER(asm_intr0x04_handler,0x04, ZERO)
INTR_HANDLER(asm_intr0x05_handler,0x05, ZERO)
INTR_HANDLER(asm_intr0x06_handler,0x06, ZERO)
INTR_HANDLER(asm_intr0x07_handler,0x07, ZERO)
INTR_HANDLER(asm_intr0x08_handler,0x08, CODE)
INTR_HANDLER(asm_intr0x09_handler,0x09, ZERO)
INTR_HANDLER(asm_intr0x0a_handler,0x0a, CODE)
INTR_HANDLER(asm_intr0x0b_handler,0x0b, CODE)
INTR_HANDLER(asm_intr0x0c_handler,0x0c, ZERO)
INTR_HANDLER(asm_intr0x0d_handler,0x0d, CODE)
INTR_HANDLER(asm_intr0x0e_handler,0x0e, CODE)
INTR_HANDLER(asm_intr0x0f_handler,0x0f, ZERO)
INTR_HANDLER(asm_intr0x10_handler,0x10, ZERO)
INTR_HANDLER(asm_intr0x11_handler,0x11, CODE)
INTR_HANDLER(asm_intr0x12_handler,0x12, ZERO)
INTR_HANDLER(asm_intr0x13_handler,0x13, ZERO)
INTR_HANDLER(asm_intr0x14_handler,0x14, ZERO)
INTR_HANDLER(asm_intr0x15_handler,0x15, ZERO)
INTR_HANDLER(asm_intr0x16_handler,0x16, ZERO)
INTR_HANDLER(asm_intr0x17_handler,0x17, ZERO)
INTR_HANDLER(asm_intr0x18_handler,0x18, CODE)
INTR_HANDLER(asm_intr0x19_handler,0x19, ZERO)
INTR_HANDLER(asm_intr0x1a_handler,0x1a, CODE)
INTR_HANDLER(asm_intr0x1b_handler,0x1b, CODE)
INTR_HANDLER(asm_intr0x1c_handler,0x1c, ZERO)
INTR_HANDLER(asm_intr0x1d_handler,0x1d, CODE)
INTR_HANDLER(asm_intr0x1e_handler,0x1e, CODE)
INTR_HANDLER(asm_intr0x1f_handler,0x1f, ZERO)

INTR_HANDLER(asm_intr0x20_handler,0x20, ZERO)
INTR_HANDLER(asm_intr0x21_handler,0x21, ZERO)
INTR_HANDLER(asm_intr0x22_handler,0x22, ZERO)
INTR_HANDLER(asm_intr0x23_handler,0x23, ZERO)
INTR_HANDLER(asm_intr0x24_handler,0x24, ZERO)
INTR_HANDLER(asm_intr0x25_handler,0x25, ZERO)
INTR_HANDLER(asm_intr0x26_handler,0x26, ZERO)
INTR_HANDLER(asm_intr0x27_handler,0x27, ZERO)
INTR_HANDLER(asm_intr0x28_handler,0x28, ZERO)
INTR_HANDLER(asm_intr0x29_handler,0x29, ZERO)
INTR_HANDLER(asm_intr0x2a_handler,0x2a, ZERO)
INTR_HANDLER(asm_intr0x2b_handler,0x2b, ZERO)
INTR_HANDLER(asm_intr0x2c_handler,0x2c, ZERO)
INTR_HANDLER(asm_intr0x2d_handler,0x2d, ZERO)
INTR_HANDLER(asm_intr0x2e_handler,0x2e, ZERO)
INTR_HANDLER(asm_intr0x2f_handler,0x2f, ZERO)

INTR_HANDLER(asm_intr0x30_handler,IRQ_XHCI,ZERO)

// IPI
INTR_HANDLER(asm_intr0x80_handler,0x80, ZERO) // Timer
INTR_HANDLER(asm_intr0x81_handler,0x81, ZERO) // Kernel Panic

#endif

#endif