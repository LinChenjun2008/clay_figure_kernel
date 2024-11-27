/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

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