// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024 LinChenjun
 */

#include <kernel/global.h>

#include <lib/bitmap.h>
#include <std/string.h> // memset

PUBLIC void init_bitmap(bitmap_t *btmp)
{
    memset(btmp->map, 0, btmp->btmp_bytes_len);
    return;
}

PUBLIC uint8_t bitmap_scan_test(bitmap_t *btmp, int32_t bit_index)
{
    int32_t byte_index = bit_index / 8;
    int32_t bit_odd    = bit_index % 8;
    return btmp->map[byte_index] & (1 << bit_odd);
}

PUBLIC status_t
bitmap_alloc(bitmap_t *btmp, uint8_t value, int32_t cnt, uint32_t *index)
{
    if (index == NULL || (value != 0 && value != 1))
    {
        return K_INVAILD_PARAM;
    }
    uint8_t byte_full = value == 0 ? 0xff : 0;

    int32_t byte_index = 0;
    while ((byte_index < btmp->btmp_bytes_len) &&
           (btmp->map[byte_index] == byte_full))
    {
        byte_index++;
    }
    if (byte_index >= btmp->btmp_bytes_len)
    {
        return K_OUT_OF_RESOURCE;
    }
    int32_t bit_index;
    for (bit_index = byte_index * 8; bit_index < btmp->btmp_bytes_len * 8;
         bit_index++)
    {
        int32_t free_bits;
        for (free_bits = 0; free_bits < cnt; free_bits++)
        {
            uint8_t current_bit = bitmap_scan_test(btmp, bit_index + free_bits);
            if (current_bit == 1 && value == 0)
            {
                break;
            }
            if (current_bit == 0 && value == 1)
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
    return K_OUT_OF_RESOURCE;
}

PUBLIC void bitmap_set(bitmap_t *btmp, int32_t bit_index, uint8_t value)
{
    int32_t byte_index = bit_index / 8;
    int32_t bit_odd    = bit_index % 8;
    switch (value)
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
