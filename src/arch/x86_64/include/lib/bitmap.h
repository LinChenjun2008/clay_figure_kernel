/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#ifndef __BITMAP_H__
#define __BITMAP_H__

typedef struct bitmap_s
{
    int32_t  btmp_bytes_len;
    uint8_t *map;
} bitmap_t;

PUBLIC void init_bitmap(bitmap_t *btmp);

/**
 * 检查btmp中的bit_index位是否为1.
 * 当bit_index位为1时返回true.
*/
PUBLIC bool bitmap_scan_test(bitmap_t *btmp,int32_t bit_index);

/**
 * 在位图中分配cnt个bit.
 * 如果成功,index指针处保存了分配到的位.
*/
PUBLIC status_t bitmap_alloc(bitmap_t *btmp,int32_t cnt,uint32_t *index);

/**
 * 设置位图中的位
 * value的取值为0或1.
 */
PUBLIC void bitmap_set(bitmap_t *btmp,int32_t bit_index,uint8_t value);

#endif