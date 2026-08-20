// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib/bitmap.h>
#include <std/string.h>

void init_bitmap(struct bitmap *bitmap, size_t size, void *map)
{
    bitmap->map_size = size;
    bitmap->map      = map;
    memset(bitmap->map, 0, bitmap->map_size);
    return;
}

int bitmap_scan_test(struct bitmap *bitmap, size_t bit_index)
{
    size_t byte_index = bit_index >> 3;
    size_t bit_odd    = bit_index & 7;
    if (byte_index >= bitmap->map_size)
    {
        return -1;
    }
    return bitmap->map[byte_index] & (1 << bit_odd) ? 1 : 0;
}

size_t bitmap_find(struct bitmap *bitmap, int value, size_t count)
{
    if (value != 0 && value != 1)
    {
        return -1;
    };
    if (count == 0)
    {
        return -1;
    }

    uint8_t byte_full  = value ? 0 : 0xff;
    size_t  byte_index = 0;
    for (byte_index = 0; byte_index < bitmap->map_size; byte_index++)
    {
        if (bitmap->map[byte_index] != byte_full)
        {
            break;
        }
    }

    if (byte_index >= bitmap->map_size)
    {
        return -1;
    }

    size_t bit_index;
    size_t bit_size = bitmap->map_size << 3;
    if (count > bit_size)
    {
        return -1;
    }

    size_t scan_size = bit_size - count;
    for (bit_index = byte_index << 3; bit_index <= scan_size; bit_index++)
    {
        size_t free_bits;
        for (free_bits = 0; free_bits < count; free_bits++)
        {
            int curr_bit = bitmap_scan_test(bitmap, bit_index + free_bits);
            // if (curr_bit < 0)
            if (curr_bit && value == 0)
            {
                break;
            }
            if (!curr_bit && value == 1)
            {
                break;
            }
        }
        if (free_bits == count)
        {
            return bit_index;
        }
        bit_index += free_bits;
    }
    return -1;
}

int bitmap_set(struct bitmap *bitmap, size_t bit_index, int value, size_t bits)
{
    if (value != 0 && value != 1)
    {
        return -1;
    }
    if (bits == 0)
    {
        return 0;
    }

    size_t bit_size = bitmap->map_size << 3;
    if (bit_index >= bit_size || bits > bit_size - bit_index)
    {
        return -1;
    }

    size_t first_byte = bit_index >> 3;
    size_t last_byte  = (bit_index + bits - 1) >> 3;
    size_t first_off  = bit_index & 7;
    size_t last_off   = (bit_index + bits - 1) & 7;

    size_t b;
    for (b = first_byte; b <= last_byte; b++)
    {
        uint8_t start_mask;
        uint8_t end_mask;

        start_mask = (b == first_byte) ? (uint8_t)(0xff << first_off) : 0xff;
        end_mask = (b == last_byte) ? (uint8_t)(0xff >> (7 - last_off)) : 0xff;
        uint8_t mask = start_mask & end_mask;

        if (value)
        {
            bitmap->map[b] |= mask;
        }
        else
        {
            bitmap->map[b] &= (uint8_t)~mask;
        }
    }
    return 0;
}