/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#ifndef __CONST_H__
#define __CONST_H__

#define KERNEL_STACK_BASE 0x400000
#define KERNEL_STACK_SIZE 0x10000

#define TSS_D_0 0
#define AR_TSS32                                                      \
    (AR_G_4K | TSS_D_0 | AR_L | AR_AVL | AR_P | AR_DPL_0 | AR_S_SYS | \
     AR_TYPE_TSS)

#define EFLAGS_MBS    (1 << 1)
#define EFLAGS_IF_1   (1 << 9)
#define EFLAGS_IF_0   (0 << 9)
#define EFLAGS_IOPL_0 (0 << 12)
#define EFLAGS_IOPL_3 (3 << 12)

#define AR_G_4K      0x8000
#define AR_D_32      0x4000
#define AR_D_64      0x0000
#define AR_L         0x0000
#define AR_L_64      0x2000
#define AR_AVL       0x0000
#define AR_P         0x0080
#define AR_DPL_0     0x0000
#define AR_DPL_1     0x0020
#define AR_DPL_2     0x0040
#define AR_DPL_3     0x0060
#define AR_S_CODE    0x0010
#define AR_S_DATA    0x0010
#define AR_S_SYS     0x0000
#define AR_TYPE_CODE 0x0008
#define AR_TYPE_DATA 0x0002
#define AR_TYPE_TSS  0x0009

#define AR_CODE32                                                      \
    (AR_G_4K | AR_D_32 | AR_L | AR_AVL | AR_P | AR_DPL_0 | AR_S_CODE | \
     AR_TYPE_CODE)
#define AR_CODE32_DPL3                                                 \
    (AR_G_4K | AR_D_32 | AR_L | AR_AVL | AR_P | AR_DPL_3 | AR_S_CODE | \
     AR_TYPE_CODE)
#define AR_DATA32                                                      \
    (AR_G_4K | AR_D_32 | AR_L | AR_AVL | AR_P | AR_DPL_0 | AR_S_DATA | \
     AR_TYPE_DATA)
#define AR_DATA32_DPL3                                                 \
    (AR_G_4K | AR_D_32 | AR_L | AR_AVL | AR_P | AR_DPL_3 | AR_S_DATA | \
     AR_TYPE_DATA)

#define AR_CODE64                                                         \
    (AR_G_4K | AR_D_64 | AR_L_64 | AR_AVL | AR_P | AR_DPL_0 | AR_S_CODE | \
     AR_TYPE_CODE)
#define AR_CODE64_DPL3                                                    \
    (AR_G_4K | AR_D_64 | AR_L_64 | AR_AVL | AR_P | AR_DPL_3 | AR_S_CODE | \
     AR_TYPE_CODE)
#define AR_DATA64                                                         \
    (AR_G_4K | AR_D_64 | AR_L_64 | AR_AVL | AR_P | AR_DPL_0 | AR_S_DATA | \
     AR_TYPE_DATA)
#define AR_DATA64_DPL3                                                    \
    (AR_G_4K | AR_D_64 | AR_L_64 | AR_AVL | AR_P | AR_DPL_3 | AR_S_DATA | \
     AR_TYPE_DATA)

#define TSS_D_0 0
#define AR_TSS64                                                      \
    (AR_G_4K | TSS_D_0 | AR_L | AR_AVL | AR_P | AR_DPL_0 | AR_S_SYS | \
     AR_TYPE_TSS)

#define RPL0 0x0
#define RPL1 0x1
#define RPL2 0x2
#define RPL3 0x3

#define TI_GDT 0x0
#define TI_LDT 0x4

#define SELECTOR_CODE64_K ((1 << 3) | TI_GDT | RPL0) /* 代码段 */
#define SELECTOR_DATA64_K ((2 << 3) | TI_GDT | RPL0) /* 数据段 */

#define SELECTOR_DATA64_U ((3 << 3) | TI_GDT | RPL3) /* 用户数据段 */
#define SELECTOR_CODE64_U ((4 << 3) | TI_GDT | RPL3) /* 用户代码段 */

#define SELECTOR_TSS(CPU) (((5 + CPU * 2) << 3) | TI_GDT | RPL0) /* TSS段 */

#define AR_DESC_32 0xe
#define AR_DESC_16 0x6

#define AR_IDT_DESC_DPL0 (AR_P | AR_DPL_0 | AR_DESC_32)
#define AR_IDT_DESC_DPL3 (AR_P | AR_DPL_3 | AR_DESC_32)

#define USER_STACK_VADDR_BASE (0x0000800000000000 - PG_SIZE)
// #define USER_VADDR_START 0x804800
#define USER_VADDR_START      0x800000

#define AP_STACK_BASE_PTR 0x1000
#define AP_START_FLAG     0x1008

#endif