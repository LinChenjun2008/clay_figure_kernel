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
#include <io.h> // io_out8

PUBLIC void init_8259a()
{
    io_out8(PIC_M_DATA, 0xff ); /* 11111111 禁止所有中断 */
    io_out8(PIC_S_DATA, 0xff ); /* 11111111 禁止所有中断 */

    io_out8(PIC_M_CTRL, 0x11 ); /* 边沿触发模式 */
    io_out8(PIC_M_DATA, 0x20 ); /* IRQ0-7由INT20-27接收 */
    io_out8(PIC_M_DATA, 0x04 ); /* PIC1由IRQ2连接*/
    io_out8(PIC_M_DATA, 0x01 ); /* 无缓冲区模式 */

    io_out8(PIC_S_CTRL, 0x11 ); /* 与上方类似 */
    io_out8(PIC_S_DATA, 0x28 ); /* IRQ8-15 INT28-2f */
    io_out8(PIC_S_DATA, 0x02 ); /* PIC1 IRQ2 */
    io_out8(PIC_S_DATA, 0x01 ); /* 无缓冲区模式 */

    io_out8(PIC_M_DATA, 0xfe ); /* 1 1 1 1 1 1 键盘 时钟*/
    io_out8(PIC_S_DATA, 0xff ); /* 1 硬盘 1 PS/2鼠标 1 1 1 实时时钟*/
    return;
}