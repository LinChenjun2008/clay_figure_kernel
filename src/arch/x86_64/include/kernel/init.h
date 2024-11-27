/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#ifndef __INIT_H__
#define __INIT_H__

typedef struct segmdesc_s
{
    uint16_t limit_low;    //  0 - 15 limit1
    uint16_t base_low;     // 16 - 31 base0
    uint8_t  base_mid;     // 32 - 39 base1
    uint8_t  access_right; // 40 - 47 flag descType privilege isVaild
    uint8_t  limit_high;   // 48 - 55 limit1 usused
    uint8_t  base_high;    // 56 - 63 base2
} segmdesc_t;

PUBLIC segmdesc_t make_segmdesc(uint32_t base,uint32_t limit,uint16_t access);
PUBLIC void init_all();
PUBLIC void ap_init_all();

#endif