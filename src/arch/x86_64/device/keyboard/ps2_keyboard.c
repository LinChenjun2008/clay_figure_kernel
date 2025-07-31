// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <device/keyboard/ps2_keyboard.h>
#include <device/pic.h>     // eois
#include <intr.h>           // register_handle
#include <io.h>             // io_in8
#include <kernel/syscall.h> // inform_intr
#include <lib/fifo.h>       // fifo functions
#include <service.h>
#include <softirq.h> // softirq

extern fifo_t key_fifo;

PRIVATE uint8_t scancode;

PRIVATE void wait_keyboard_read(void)
{
    while (io_in8(KEYBOARD_STA_PORT) & KEYBOARD_NOTREADY) continue;
}

// PRIVATE void wait_keyboard_write(void)
// {
//     while (io_in8(KEYBOARD_STA_PORT) & 0x01) continue;
// }

PRIVATE void intr_keyboard_handler(intr_stack_t *stack)
{
    send_eoi(stack->int_vector);
    scancode = io_in8(KEYBOARD_DATA_PORT);
    raise_softirq(SOFTIRQ_KEYBOARD);
    return;
}

PRIVATE void keyboard_softirq(void *data)
{
    PR_LOG(LOG_INFO, "Key: [%02x]\n", *(uint8_t *)data);
    inform_intr(KBD_SRV);
    return;
}

PUBLIC void ps2_keyboard_init(void)
{
    // Configure Device
    wait_keyboard_read();
    io_out8(KEYBOARD_CMD_PORT, KEYBOARD_WRITE_MD);
    wait_keyboard_read();
    io_out8(KEYBOARD_DATA_PORT, 0x47);

    register_handle(0x21, intr_keyboard_handler);
    register_softirq(SOFTIRQ_KEYBOARD, keyboard_softirq, &scancode);
    ioapic_enable(1, 0x21);
    return;
}