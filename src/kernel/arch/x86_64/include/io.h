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

#ifndef __IO_H__
#define __IO_H__

static inline void io_hlt(void)
{
    __asm__ __volatile__ ("hlt":::"memory");
    return;
}

static inline void io_sti(void)
{
    __asm__ __volatile__ ("sti":::"memory");
    return;
}

static inline void io_cli(void)
{
    __asm__ __volatile__ ("cli":::"memory");
    return;
}

static inline void io_stihlt(void)
{
    __asm__ __volatile__ ("sti\n\t""hlt;":::"memory");
    return;
}

static inline void io_mfence(void)
{
    __asm__ __volatile("mfence":::"memory");
}

static inline uint32_t io_in8(uint32_t port)
{
    uint32_t data;
    __asm__ __volatile__
    (
        "inb %w1,%b0"
        :"=a"(data)
        :"d"(port)
        :"memory"
    );
    return data;
}

static inline uint32_t io_in16(uint32_t port)
{
    uint32_t data;
    __asm__ __volatile__
    (
        "inw %w1,%w0"
        :"=a"(data)
        :"d"(port)
        :"memory"
    );
    return data;
}

static inline uint32_t io_in32(uint32_t port)
{
    uint32_t data;
    __asm__ __volatile__
    (
        "inl %w1,%k0"
        :"=a"(data)
        :"d"(port)
        :"memory"
    );
    return data;
}



static inline void io_out8(uint32_t port,uint32_t data)
{
    __asm__ __volatile__
    (
        "outb %b0,%w1"
        :
        :"a"(data),"d"(port)
        :"memory"
    );
}

static inline void io_out16(uint32_t port,uint32_t data)
{
    __asm__ __volatile__
    (
        "outw %w0,%w1"
        :
        :"a"(data),"d"(port)
        :"memory"
    );
}

static inline void io_out32(uint32_t port,uint32_t data)
{
    __asm__ __volatile__
    (
        "outl %k0,%w1"
        :
        :"a"(data),"d"(port)
        :"memory"
    );
}

static inline uint32_t get_flages()
{
    uint32_t flages;
    __asm__ __volatile__
    (
        "pushf;"      /* 将flage寄存器压栈 */
        "popq %q0"
        :"=a"(flages)
        :
        :"memory"
    );
    return flages;
}

#endif