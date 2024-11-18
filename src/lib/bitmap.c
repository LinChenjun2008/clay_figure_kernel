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
