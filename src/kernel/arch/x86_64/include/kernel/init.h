/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.
Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

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