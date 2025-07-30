// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#ifndef __BITMAP_H__
#define __BITMAP_H__

typedef struct bitmap_s
{
    size_t   btmp_bytes_len;
    uint8_t *map;
} bitmap_t;

PUBLIC void init_bitmap(bitmap_t *btmp);

/**
 * @brief 检查btmp中的bit_index位是否为1
 * @param btmp 位图结构指针
 * @param bit_index 索引(以bit为单位)
 * @return 当bit_index位为1时返回true
*/
PUBLIC int bitmap_scan_test(bitmap_t *btmp, size_t bit_index);

/**
 * @brief 在位图中分配连续cnt个值为alue的位.
 * @param btmp 位图结构指针
 * @param value 要分配的bit值(0或1)
 * @param cnt 要分配的bit数
 * @param index 如果成功,index指针处保存了分配到的位索引
*/
PUBLIC status_t
bitmap_alloc(bitmap_t *btmp, uint8_t value, size_t cnt, uint32_t *index);

/**
 * @brief 设置位图中的位
 * @param btmp 位图结构指针
 * @param bit_index 索引(以bit为单位)
 * @param value 位值
 * @note value的取值为0或1
 */
PUBLIC void bitmap_set(bitmap_t *btmp, size_t bit_index, uint8_t value);

#endif