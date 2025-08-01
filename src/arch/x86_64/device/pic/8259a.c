// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <device/pic.h>
#include <io.h> // io_out8

PUBLIC void init_8259a(void)
{
    io_out8(PIC_M_DATA, 0xff); /* 11111111 禁止所有中断 */
    io_out8(PIC_S_DATA, 0xff); /* 11111111 禁止所有中断 */

    io_out8(PIC_M_CTRL, 0x11); /* 边沿触发模式 */
    io_out8(PIC_M_DATA, 0x20); /* IRQ0-7由INT20-27接收 */
    io_out8(PIC_M_DATA, 0x04); /* PIC1由IRQ2连接*/
    io_out8(PIC_M_DATA, 0x01); /* 无缓冲区模式 */

    io_out8(PIC_S_CTRL, 0x11); /* 与上方类似 */
    io_out8(PIC_S_DATA, 0x28); /* IRQ8-15 INT28-2f */
    io_out8(PIC_S_DATA, 0x02); /* PIC1 IRQ2 */
    io_out8(PIC_S_DATA, 0x01); /* 无缓冲区模式 */

    io_out8(PIC_M_DATA, 0xfe); /* 1 1 1 1 1 1 键盘 时钟*/
    io_out8(PIC_S_DATA, 0xff); /* 1 硬盘 1 PS/2鼠标 1 1 1 实时时钟*/
    return;
}