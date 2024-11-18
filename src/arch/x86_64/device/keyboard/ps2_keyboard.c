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
#include <device/keyboard/ps2_keyboard.h>
#include <intr.h>           // register_handle
#include <device/pic.h>     // eois
#include <io.h>             // io_in8
#include <kernel/syscall.h> // inform_intr
#include <lib/fifo.h>       // fifo functions

#include <log.h>

extern fifo_t key_fifo;

PRIVATE void wait_keyboard_read()
{
    while (io_in8(KEYBOARD_STA_PORT) & KEYBOARD_NOTREADY) continue;
}

PRIVATE void wait_keyboard_write()
{
    while (io_in8(KEYBOARD_STA_PORT) & 0x01) continue;
}

PRIVATE void intr_keyboard_handler()
{
    eoi(0x21);
    do
    {
        uint8_t scancode = io_in8(KEYBOARD_DATA_PORT);
        pr_log("\n");
        pr_log("\1 Key: [%02x]\n",scancode);
        if (scancode != 0xfa)
        {
            fifo_put(&key_fifo,&scancode);
            inform_intr(KBD_SRV);
        }
    } while (io_in8(KEYBOARD_STA_PORT) & 0x01);
    return;
}

PUBLIC void ps2_keyboard_init()
{
    // Restart
    uint8_t ack;
    do
    {
        wait_keyboard_read();
        io_out8(KEYBOARD_CMD_PORT,0xff);
        ack = io_in8(KEYBOARD_DATA_PORT);
    } while (ack == 0xfe);
    // Flush output buffer
    do
    {
        io_in8(KEYBOARD_DATA_PORT);
    } while (io_in8(KEYBOARD_STA_PORT) & 0x01);

    // Configure Device
    wait_keyboard_read();
    io_out8(KEYBOARD_CMD_PORT,KEYBOARD_WRITE_MD);
    wait_keyboard_read();
    io_out8(KEYBOARD_DATA_PORT,0x47);

    // Set scanning rate
    do
    {
        wait_keyboard_read();
        io_out8(KEYBOARD_CMD_PORT,0xf3);
        wait_keyboard_read();
        io_out8(KEYBOARD_CMD_PORT,0x00);
        wait_keyboard_write();
    } while (io_in8(KEYBOARD_DATA_PORT) == 0xfe);

    pr_log("\1 Keyboard init done.\n");

    register_handle(0x21,intr_keyboard_handler);
    ioapic_enable(1,0x21);
    return;
}