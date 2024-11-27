/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <lib/bitmap.h>
#include <std/string.h> // memset

PUBLIC void init_bitmap(bitmap_t *btmp)
{
    memset(btmp->map,0,btmp->btmp_bytes_len);
    return;
}

PUBLIC bool bitmap_scan_test(bitmap_t *btmp,int32_t bit_index)
{
    int32_t byte_index = bit_index / 8;
    int32_t bit_odd = bit_index % 8;
    return btmp->map[byte_index] & (1 << bit_odd);
}

PUBLIC status_t bitmap_alloc(bitmap_t *btmp,int32_t cnt,uint32_t *index)
{
    if (index == NULL)
    {
        return K_ERROR;
    }
    int32_t byte_index = 0;
    while ((byte_index < btmp->btmp_bytes_len)
        && (btmp->map[byte_index] == 0xff))
    {
        byte_index++;
    }
    if (byte_index >= btmp->btmp_bytes_len)
    {
        return K_ERROR;
    }
    int32_t bit_index;
    for (bit_index = byte_index * 8;bit_index < btmp->btmp_bytes_len;bit_index++)
    {
        int32_t free_bits;
        for (free_bits = 0;free_bits < cnt;free_bits++)
        {
            if (bitmap_scan_test(btmp,bit_index + free_bits))
            {
                break;
            }
        }
        if (free_bits == cnt)
        {
            *index = bit_index;
            return K_SUCCESS;
        }
        bit_index += free_bits;
    }
    return K_ERROR;
}

PUBLIC void bitmap_set(bitmap_t *btmp,int32_t bit_index,uint8_t value)
{
    int32_t byte_index = bit_index / 8;
    int32_t bit_odd = bit_index % 8;
    switch(value)
    {
        case 0:
            btmp->map[byte_index] &= ~(1 << bit_odd);
            break;
        case 1:
            btmp->map[byte_index] |= 1 << bit_odd;
            break;
        default:
            break;
    }
    return;
}
