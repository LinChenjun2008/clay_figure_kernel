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

    wordsize_t int_vector;

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
PUBLIC void intr_init(void);
PUBLIC void ap_intr_init(void);
PUBLIC void register_handle(uint8_t int_vector,void (*handle)(intr_stack_t*));
PUBLIC intr_status_t intr_get_status(void);
PUBLIC intr_status_t intr_set_status(intr_status_t status);
PUBLIC intr_status_t intr_enable(void);
PUBLIC intr_status_t intr_disable(void);

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

INTR_HANDLER(asm_intr0x30_handler,0x30, ZERO)
INTR_HANDLER(asm_intr0x31_handler,0x31, ZERO)
INTR_HANDLER(asm_intr0x32_handler,0x32, ZERO)
INTR_HANDLER(asm_intr0x33_handler,0x33, ZERO)
INTR_HANDLER(asm_intr0x34_handler,0x34, ZERO)
INTR_HANDLER(asm_intr0x35_handler,0x35, ZERO)
INTR_HANDLER(asm_intr0x36_handler,0x36, ZERO)
INTR_HANDLER(asm_intr0x37_handler,0x37, ZERO)
INTR_HANDLER(asm_intr0x38_handler,0x38, ZERO)
INTR_HANDLER(asm_intr0x39_handler,0x39, ZERO)
INTR_HANDLER(asm_intr0x3a_handler,0x3a, ZERO)
INTR_HANDLER(asm_intr0x3b_handler,0x3b, ZERO)
INTR_HANDLER(asm_intr0x3c_handler,0x3c, ZERO)
INTR_HANDLER(asm_intr0x3d_handler,0x3d, ZERO)
INTR_HANDLER(asm_intr0x3e_handler,0x3e, ZERO)
INTR_HANDLER(asm_intr0x3f_handler,0x3f, ZERO)

INTR_HANDLER(asm_intr0x40_handler,0x40, ZERO)
INTR_HANDLER(asm_intr0x41_handler,0x41, ZERO)
INTR_HANDLER(asm_intr0x42_handler,0x42, ZERO)
INTR_HANDLER(asm_intr0x43_handler,0x43, ZERO)
INTR_HANDLER(asm_intr0x44_handler,0x44, ZERO)
INTR_HANDLER(asm_intr0x45_handler,0x45, ZERO)
INTR_HANDLER(asm_intr0x46_handler,0x46, ZERO)
INTR_HANDLER(asm_intr0x47_handler,0x47, ZERO)
INTR_HANDLER(asm_intr0x48_handler,0x48, ZERO)
INTR_HANDLER(asm_intr0x49_handler,0x49, ZERO)
INTR_HANDLER(asm_intr0x4a_handler,0x4a, ZERO)
INTR_HANDLER(asm_intr0x4b_handler,0x4b, ZERO)
INTR_HANDLER(asm_intr0x4c_handler,0x4c, ZERO)
INTR_HANDLER(asm_intr0x4d_handler,0x4d, ZERO)
INTR_HANDLER(asm_intr0x4e_handler,0x4e, ZERO)
INTR_HANDLER(asm_intr0x4f_handler,0x4f, ZERO)

INTR_HANDLER(asm_intr0x50_handler,0x50, ZERO)
INTR_HANDLER(asm_intr0x51_handler,0x51, ZERO)
INTR_HANDLER(asm_intr0x52_handler,0x52, ZERO)
INTR_HANDLER(asm_intr0x53_handler,0x53, ZERO)
INTR_HANDLER(asm_intr0x54_handler,0x54, ZERO)
INTR_HANDLER(asm_intr0x55_handler,0x55, ZERO)
INTR_HANDLER(asm_intr0x56_handler,0x56, ZERO)
INTR_HANDLER(asm_intr0x57_handler,0x57, ZERO)
INTR_HANDLER(asm_intr0x58_handler,0x58, ZERO)
INTR_HANDLER(asm_intr0x59_handler,0x59, ZERO)
INTR_HANDLER(asm_intr0x5a_handler,0x5a, ZERO)
INTR_HANDLER(asm_intr0x5b_handler,0x5b, ZERO)
INTR_HANDLER(asm_intr0x5c_handler,0x5c, ZERO)
INTR_HANDLER(asm_intr0x5d_handler,0x5d, ZERO)
INTR_HANDLER(asm_intr0x5e_handler,0x5e, ZERO)
INTR_HANDLER(asm_intr0x5f_handler,0x5f, ZERO)

INTR_HANDLER(asm_intr0x60_handler,0x60, ZERO)
INTR_HANDLER(asm_intr0x61_handler,0x61, ZERO)
INTR_HANDLER(asm_intr0x62_handler,0x62, ZERO)
INTR_HANDLER(asm_intr0x63_handler,0x63, ZERO)
INTR_HANDLER(asm_intr0x64_handler,0x64, ZERO)
INTR_HANDLER(asm_intr0x65_handler,0x65, ZERO)
INTR_HANDLER(asm_intr0x66_handler,0x66, ZERO)
INTR_HANDLER(asm_intr0x67_handler,0x67, ZERO)
INTR_HANDLER(asm_intr0x68_handler,0x68, ZERO)
INTR_HANDLER(asm_intr0x69_handler,0x69, ZERO)
INTR_HANDLER(asm_intr0x6a_handler,0x6a, ZERO)
INTR_HANDLER(asm_intr0x6b_handler,0x6b, ZERO)
INTR_HANDLER(asm_intr0x6c_handler,0x6c, ZERO)
INTR_HANDLER(asm_intr0x6d_handler,0x6d, ZERO)
INTR_HANDLER(asm_intr0x6e_handler,0x6e, ZERO)
INTR_HANDLER(asm_intr0x6f_handler,0x6f, ZERO)

INTR_HANDLER(asm_intr0x70_handler,0x70, ZERO)
INTR_HANDLER(asm_intr0x71_handler,0x71, ZERO)
INTR_HANDLER(asm_intr0x72_handler,0x72, ZERO)
INTR_HANDLER(asm_intr0x73_handler,0x73, ZERO)
INTR_HANDLER(asm_intr0x74_handler,0x74, ZERO)
INTR_HANDLER(asm_intr0x75_handler,0x75, ZERO)
INTR_HANDLER(asm_intr0x76_handler,0x76, ZERO)
INTR_HANDLER(asm_intr0x77_handler,0x77, ZERO)
INTR_HANDLER(asm_intr0x78_handler,0x78, ZERO)
INTR_HANDLER(asm_intr0x79_handler,0x79, ZERO)
INTR_HANDLER(asm_intr0x7a_handler,0x7a, ZERO)
INTR_HANDLER(asm_intr0x7b_handler,0x7b, ZERO)
INTR_HANDLER(asm_intr0x7c_handler,0x7c, ZERO)
INTR_HANDLER(asm_intr0x7d_handler,0x7d, ZERO)
INTR_HANDLER(asm_intr0x7e_handler,0x7e, ZERO)
INTR_HANDLER(asm_intr0x7f_handler,0x7f, ZERO)

INTR_HANDLER(asm_intr0x80_handler,0x80, ZERO) // Timer
INTR_HANDLER(asm_intr0x81_handler,0x81, ZERO) // Kernel panic
INTR_HANDLER(asm_intr0x82_handler,0x82, ZERO) // debug
INTR_HANDLER(asm_intr0x83_handler,0x83, ZERO)
INTR_HANDLER(asm_intr0x84_handler,0x84, ZERO)
INTR_HANDLER(asm_intr0x85_handler,0x85, ZERO)
INTR_HANDLER(asm_intr0x86_handler,0x86, ZERO)
INTR_HANDLER(asm_intr0x87_handler,0x87, ZERO)
INTR_HANDLER(asm_intr0x88_handler,0x88, ZERO)
INTR_HANDLER(asm_intr0x89_handler,0x89, ZERO)
INTR_HANDLER(asm_intr0x8a_handler,0x8a, ZERO)
INTR_HANDLER(asm_intr0x8b_handler,0x8b, ZERO)
INTR_HANDLER(asm_intr0x8c_handler,0x8c, ZERO)
INTR_HANDLER(asm_intr0x8d_handler,0x8d, ZERO)
INTR_HANDLER(asm_intr0x8e_handler,0x8e, ZERO)
INTR_HANDLER(asm_intr0x8f_handler,0x8f, ZERO)

INTR_HANDLER(asm_intr0x90_handler,0x90, ZERO)
INTR_HANDLER(asm_intr0x91_handler,0x91, ZERO)
INTR_HANDLER(asm_intr0x92_handler,0x92, ZERO)
INTR_HANDLER(asm_intr0x93_handler,0x93, ZERO)
INTR_HANDLER(asm_intr0x94_handler,0x94, ZERO)
INTR_HANDLER(asm_intr0x95_handler,0x95, ZERO)
INTR_HANDLER(asm_intr0x96_handler,0x96, ZERO)
INTR_HANDLER(asm_intr0x97_handler,0x97, ZERO)
INTR_HANDLER(asm_intr0x98_handler,0x98, ZERO)
INTR_HANDLER(asm_intr0x99_handler,0x99, ZERO)
INTR_HANDLER(asm_intr0x9a_handler,0x9a, ZERO)
INTR_HANDLER(asm_intr0x9b_handler,0x9b, ZERO)
INTR_HANDLER(asm_intr0x9c_handler,0x9c, ZERO)
INTR_HANDLER(asm_intr0x9d_handler,0x9d, ZERO)
INTR_HANDLER(asm_intr0x9e_handler,0x9e, ZERO)
INTR_HANDLER(asm_intr0x9f_handler,0x9f, ZERO)

INTR_HANDLER(asm_intr0xa0_handler,0xa0, ZERO)
INTR_HANDLER(asm_intr0xa1_handler,0xa1, ZERO)
INTR_HANDLER(asm_intr0xa2_handler,0xa2, ZERO)
INTR_HANDLER(asm_intr0xa3_handler,0xa3, ZERO)
INTR_HANDLER(asm_intr0xa4_handler,0xa4, ZERO)
INTR_HANDLER(asm_intr0xa5_handler,0xa5, ZERO)
INTR_HANDLER(asm_intr0xa6_handler,0xa6, ZERO)
INTR_HANDLER(asm_intr0xa7_handler,0xa7, ZERO)
INTR_HANDLER(asm_intr0xa8_handler,0xa8, ZERO)
INTR_HANDLER(asm_intr0xa9_handler,0xa9, ZERO)
INTR_HANDLER(asm_intr0xaa_handler,0xaa, ZERO)
INTR_HANDLER(asm_intr0xab_handler,0xab, ZERO)
INTR_HANDLER(asm_intr0xac_handler,0xac, ZERO)
INTR_HANDLER(asm_intr0xad_handler,0xad, ZERO)
INTR_HANDLER(asm_intr0xae_handler,0xae, ZERO)
INTR_HANDLER(asm_intr0xaf_handler,0xaf, ZERO)

INTR_HANDLER(asm_intr0xb0_handler,0xb0, ZERO)
INTR_HANDLER(asm_intr0xb1_handler,0xb1, ZERO)
INTR_HANDLER(asm_intr0xb2_handler,0xb2, ZERO)
INTR_HANDLER(asm_intr0xb3_handler,0xb3, ZERO)
INTR_HANDLER(asm_intr0xb4_handler,0xb4, ZERO)
INTR_HANDLER(asm_intr0xb5_handler,0xb5, ZERO)
INTR_HANDLER(asm_intr0xb6_handler,0xb6, ZERO)
INTR_HANDLER(asm_intr0xb7_handler,0xb7, ZERO)
INTR_HANDLER(asm_intr0xb8_handler,0xb8, ZERO)
INTR_HANDLER(asm_intr0xb9_handler,0xb9, ZERO)
INTR_HANDLER(asm_intr0xba_handler,0xba, ZERO)
INTR_HANDLER(asm_intr0xbb_handler,0xbb, ZERO)
INTR_HANDLER(asm_intr0xbc_handler,0xbc, ZERO)
INTR_HANDLER(asm_intr0xbd_handler,0xbd, ZERO)
INTR_HANDLER(asm_intr0xbe_handler,0xbe, ZERO)
INTR_HANDLER(asm_intr0xbf_handler,0xbf, ZERO)

INTR_HANDLER(asm_intr0xc0_handler,0xc0, ZERO)
INTR_HANDLER(asm_intr0xc1_handler,0xc1, ZERO)
INTR_HANDLER(asm_intr0xc2_handler,0xc2, ZERO)
INTR_HANDLER(asm_intr0xc3_handler,0xc3, ZERO)
INTR_HANDLER(asm_intr0xc4_handler,0xc4, ZERO)
INTR_HANDLER(asm_intr0xc5_handler,0xc5, ZERO)
INTR_HANDLER(asm_intr0xc6_handler,0xc6, ZERO)
INTR_HANDLER(asm_intr0xc7_handler,0xc7, ZERO)
INTR_HANDLER(asm_intr0xc8_handler,0xc8, ZERO)
INTR_HANDLER(asm_intr0xc9_handler,0xc9, ZERO)
INTR_HANDLER(asm_intr0xca_handler,0xca, ZERO)
INTR_HANDLER(asm_intr0xcb_handler,0xcb, ZERO)
INTR_HANDLER(asm_intr0xcc_handler,0xcc, ZERO)
INTR_HANDLER(asm_intr0xcd_handler,0xcd, ZERO)
INTR_HANDLER(asm_intr0xce_handler,0xce, ZERO)
INTR_HANDLER(asm_intr0xcf_handler,0xcf, ZERO)

INTR_HANDLER(asm_intr0xd0_handler,0xd0, ZERO)
INTR_HANDLER(asm_intr0xd1_handler,0xd1, ZERO)
INTR_HANDLER(asm_intr0xd2_handler,0xd2, ZERO)
INTR_HANDLER(asm_intr0xd3_handler,0xd3, ZERO)
INTR_HANDLER(asm_intr0xd4_handler,0xd4, ZERO)
INTR_HANDLER(asm_intr0xd5_handler,0xd5, ZERO)
INTR_HANDLER(asm_intr0xd6_handler,0xd6, ZERO)
INTR_HANDLER(asm_intr0xd7_handler,0xd7, ZERO)
INTR_HANDLER(asm_intr0xd8_handler,0xd8, ZERO)
INTR_HANDLER(asm_intr0xd9_handler,0xd9, ZERO)
INTR_HANDLER(asm_intr0xda_handler,0xda, ZERO)
INTR_HANDLER(asm_intr0xdb_handler,0xdb, ZERO)
INTR_HANDLER(asm_intr0xdc_handler,0xdc, ZERO)
INTR_HANDLER(asm_intr0xdd_handler,0xdd, ZERO)
INTR_HANDLER(asm_intr0xde_handler,0xde, ZERO)
INTR_HANDLER(asm_intr0xdf_handler,0xdf, ZERO)

INTR_HANDLER(asm_intr0xe0_handler,0xe0, ZERO)
INTR_HANDLER(asm_intr0xe1_handler,0xe1, ZERO)
INTR_HANDLER(asm_intr0xe2_handler,0xe2, ZERO)
INTR_HANDLER(asm_intr0xe3_handler,0xe3, ZERO)
INTR_HANDLER(asm_intr0xe4_handler,0xe4, ZERO)
INTR_HANDLER(asm_intr0xe5_handler,0xe5, ZERO)
INTR_HANDLER(asm_intr0xe6_handler,0xe6, ZERO)
INTR_HANDLER(asm_intr0xe7_handler,0xe7, ZERO)
INTR_HANDLER(asm_intr0xe8_handler,0xe8, ZERO)
INTR_HANDLER(asm_intr0xe9_handler,0xe9, ZERO)
INTR_HANDLER(asm_intr0xea_handler,0xea, ZERO)
INTR_HANDLER(asm_intr0xeb_handler,0xeb, ZERO)
INTR_HANDLER(asm_intr0xec_handler,0xec, ZERO)
INTR_HANDLER(asm_intr0xed_handler,0xed, ZERO)
INTR_HANDLER(asm_intr0xee_handler,0xee, ZERO)
INTR_HANDLER(asm_intr0xef_handler,0xef, ZERO)

INTR_HANDLER(asm_intr0xf0_handler,0xf0, ZERO)
INTR_HANDLER(asm_intr0xf1_handler,0xf1, ZERO)
INTR_HANDLER(asm_intr0xf2_handler,0xf2, ZERO)
INTR_HANDLER(asm_intr0xf3_handler,0xf3, ZERO)
INTR_HANDLER(asm_intr0xf4_handler,0xf4, ZERO)
INTR_HANDLER(asm_intr0xf5_handler,0xf5, ZERO)
INTR_HANDLER(asm_intr0xf6_handler,0xf6, ZERO)
INTR_HANDLER(asm_intr0xf7_handler,0xf7, ZERO)
INTR_HANDLER(asm_intr0xf8_handler,0xf8, ZERO)
INTR_HANDLER(asm_intr0xf9_handler,0xf9, ZERO)
INTR_HANDLER(asm_intr0xfa_handler,0xfa, ZERO)
INTR_HANDLER(asm_intr0xfb_handler,0xfb, ZERO)
INTR_HANDLER(asm_intr0xfc_handler,0xfc, ZERO)
INTR_HANDLER(asm_intr0xfd_handler,0xfd, ZERO)
INTR_HANDLER(asm_intr0xfe_handler,0xfe, ZERO)
INTR_HANDLER(asm_intr0xff_handler,0xff, ZERO)

#endif /* INTR_HANDLER */

#endif