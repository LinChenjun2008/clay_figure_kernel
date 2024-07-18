#include <kernel/global.h>
#include <io.h>
#include <intr.h>
#include <device/pic.h>
#include <lib/fifo.h>

#include <log.h>

#define KEYBOARD_BUF_PORT 0x60
#define KEYBOARD_WRITE_MD 0x60
#define KEYBOARD_STA_PORT 0x64
#define KEYBOARD_CMD_PORT 0x64
#define KEYBOARD_NOTREADY 0x02

PUBLIC fifo_t keyboard_fifo;
PRIVATE uint8_t keyboard_buf[64];

PRIVATE void intr_keyboard_handler()
{
    eoi(0x21);
    uint16_t scancode;
    scancode = io_in8(KEYBOARD_BUF_PORT);
    pr_log("scancode: %x ",scancode);
    int status = fifo_put(&keyboard_fifo,&scancode);
    if (ERROR(status))
    {
        pr_log("\n");
        pr_log("\3 keyboard fifo filled.\n");
    }
    return;
}

PRIVATE void wait_keyboard_ready()
{
    while(1)
    {
        if((io_in8(KEYBOARD_STA_PORT) & KEYBOARD_NOTREADY) == 0)
        {
            break;
        }
    }
}

PUBLIC void keyboard_init()
{
    wait_keyboard_ready();
    io_out8(KEYBOARD_CMD_PORT,KEYBOARD_WRITE_MD);
    wait_keyboard_ready();
    io_out8(KEYBOARD_BUF_PORT,0x47);
    init_fifo(&keyboard_fifo,keyboard_buf,8,64);
    register_handle(0x21,intr_keyboard_handler);
}