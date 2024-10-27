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

#include <kernel/global.h>
#include <sync/atomic.h> // atomic_t

PUBLIC void atomic_set(atomic_t *atom,uint64_t value)
{
    atom->value = value;
    return;
}

PUBLIC uint64_t atomic_read(atomic_t *atom)
{
    return atom->value;
}

extern void asm_atomic_add(volatile uint64_t *atom,uint64_t value);
extern void asm_atomic_sub(volatile uint64_t *atom,uint64_t value);
extern void asm_atomic_inc(volatile uint64_t *atom);
extern void asm_atomic_dec(volatile uint64_t *atom);
extern void asm_atomic_mask(volatile uint64_t *atom,uint64_t mask);

PUBLIC void atomic_add(atomic_t *atom,uint64_t value)
{
    asm_atomic_add(&atom->value,value);
    return;
}

PUBLIC void atomic_sub(atomic_t *atom,uint64_t value)
{
    asm_atomic_sub(&atom->value,value);
    return;
}

PUBLIC void atomic_inc(atomic_t *atom)
{
    asm_atomic_inc(&atom->value);
    return;
}

PUBLIC void atomic_dec(atomic_t *atom)
{
    asm_atomic_dec(&atom->value);
    return;
}

PUBLIC void atomic_mask(atomic_t *atom,uint64_t mask)
{
    asm_atomic_mask(&atom->value,mask);
    return;
}