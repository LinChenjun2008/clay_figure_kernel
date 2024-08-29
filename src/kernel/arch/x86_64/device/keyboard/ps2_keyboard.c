#include <kernel/global.h>
#include <device/keyboard/ps2_keyboard.h>
#include <intr.h>           // register_handle
#include <device/pic.h>     // eois
#include <io.h>             // io_in8
#include <kernel/syscall.h> // inform_intr
#include <lib/fifo.h>       // fifo functions
#include <log.h>

extern fifo_t key_fifo;

PRIVATE bool ctrl_status;
PRIVATE bool shift_status;
PRIVATE bool alt_status;
PRIVATE bool capslock_status;
PRIVATE bool ext_scandcode;

#define KEY_ESC         0
#define KEY_LEFT_CTRL   0
#define KEY_LEFT_SHIFT  0
#define KEY_RIGHT_SHIFT 0
#define KEY_LEFT_ALT    0
#define KEY_CAPSLOCK    0

#define L_SHIFT_MAKE  0x2a
#define R_SHIFT_MAKE  0x36
#define L_ALT_MAKE    0x38
#define R_ALT_MAKE    0xe038
#define R_ALT_BREAK   0xe0b8
#define L_CTRL_MAKE   0x1d
#define R_CTRL_MAKE   0xe01d
#define R_CTRL_BREAK  0xe09d
#define CAPSLOCK_MAKE 0x3a


PRIVATE char key_map [][2] =
{
    /*    shift */
    { 0  ,   0}, /* 0x00 */
    { KEY_ESC, KEY_ESC}, /* 0x01 */
    { '1', '!'}, /* 0x02 */
    { '2', '@'}, /* 0x03 */
    { '3', '#'}, /* 0x04 */
    { '4', '$'}, /* 0x05 */
    { '5', '%'}, /* 0x06 */
    { '6', '^'}, /* 0x07 */
    { '7', '&'}, /* 0x08 */
    { '8', '*'}, /* 0x09 */
    { '9', '('}, /* 0x0a */
    { '0', ')'}, /* 0x0b */
    { '-', '_'}, /* 0x0c */
    { '=', '+'}, /* 0x0d */
    { '\b', '\b'}, /* 0x0e */
    { '\t', '\t'}, /* 0x0f */
    { 'q', 'Q'}, /* 0x10 */
    { 'w', 'W'}, /* 0x11 */
    { 'e', 'E'}, /* 0x12 */
    { 'r', 'R'}, /* 0x13 */
    { 't', 'T'}, /* 0x14 */
    { 'y', 'Y'}, /* 0x15 */
    { 'u', 'U'}, /* 0x16 */
    { 'i', 'I'}, /* 0x17 */
    { 'o', 'O'}, /* 0x18 */
    { 'p', 'P'}, /* 0x19 */
    { '[', '{'}, /* 0x1a */
    { ']', '}'}, /* 0x1b */
    { '\n', '\n'}, /* 0x1c */
    { KEY_LEFT_CTRL, KEY_LEFT_CTRL}, /* 0x1d */
    { 'a', 'A'}, /* 0x1e */
    { 's', 'S'}, /* 0x1f */
    { 'd', 'D'}, /* 0x20 */
    { 'f', 'F'}, /* 0x21 */
    { 'g', 'G'}, /* 0x22 */
    { 'h', 'H'}, /* 0x23 */
    { 'j', 'J'}, /* 0x24 */
    { 'k', 'K'}, /* 0x25 */
    { 'l', 'L'}, /* 0x26 */
    { ';', ':'}, /* 0x27 */
    { '\'', '"'}, /* 0x28 */
    { '`', '~'}, /* 0x29 */
    { KEY_LEFT_SHIFT, KEY_LEFT_SHIFT}, /* 0x2a */
    { '\\', '|'}, /* 0x2b */
    { 'z', 'Z'}, /* 0x2c */
    { 'x', 'X'}, /* 0x2d */
    { 'c', 'C'}, /* 0x2e */
    { 'v', 'V'}, /* 0x2f */
    { 'b', 'B'}, /* 0x30 */
    { 'n', 'N'}, /* 0x31 */
    { 'm', 'M'}, /* 0x32 */
    { ',', '<'}, /* 0x33 */
    { '.', '>'}, /* 0x34 */
    { '/', '?'}, /* 0x35 */
    { KEY_RIGHT_SHIFT, KEY_RIGHT_SHIFT}, /* 0x36 */
    { '*', '*'}, /* 0x37 */
    { KEY_LEFT_ALT, KEY_LEFT_ALT}, /* 0x38 */
    { ' ', ' '}, /* 0x39 */
    { KEY_CAPSLOCK, KEY_CAPSLOCK}, /* 0x3a */
};

PRIVATE void intr_keyboard_handler()
{
    eoi(0x21);
    // bool ctrl_down = ctrl_status;
    bool shift_down = shift_status;
    bool capslock_down = capslock_status;
    uint16_t scancode = io_in8(KEYBOARD_BUF_PORT);
    bool is_break_code;
    if (scancode == 0xe0)
    {
        ext_scandcode = TRUE;
        return;
    }
    if (ext_scandcode)
    {
        scancode |= 0xe000;
        ext_scandcode = FALSE;
    }
    is_break_code = (scancode & 0x0080) != 0;
    if (is_break_code)
    {
        // to make code
        scancode &= 0xff7f;
        switch (scancode)
        {
            case L_SHIFT_MAKE:
            case R_SHIFT_MAKE:
                shift_status = FALSE;
                break;
            case L_CTRL_MAKE:
            case R_CTRL_MAKE:
                ctrl_status = FALSE;
                break;
            case L_ALT_MAKE:
            case R_ALT_MAKE:
                alt_status = FALSE;
                break;
        }
        return;
    }
    // is make code
    else if((scancode < 0x3b && scancode > 0x00)
            || scancode == R_ALT_MAKE || scancode == R_CTRL_MAKE)
    {
        bool shift = FALSE;
        if((scancode < 0x0e)
           || (scancode == 0x29)
           || (scancode == 0x1a)
           || (scancode == 0x1b)
           || (scancode == 0x2b)
           || (scancode == 0x27)
           || (scancode == 0x28)
           || (scancode >= 0x33 && scancode <= 0x35))
        {
            /* shift被按下 */
            if(shift_down == TRUE)
            {
                shift = TRUE;
            }
        }
        else
        {
            if(shift_down && capslock_down)
            {
                shift = FALSE;
            }
            else if(shift_down || capslock_down)
            {
                shift = TRUE;
            }
            else
            {
                shift = FALSE;
            }
        }
        scancode &= 0x00ff;
        char cur_char = key_map[scancode][shift];
        // (void)cur_char;
        fifo_put(&key_fifo,&cur_char);
        inform_intr(KBD_SRV);
        switch (scancode)
        {
            case L_SHIFT_MAKE:
            case R_SHIFT_MAKE:
                shift_status = TRUE;
                break;
            case L_CTRL_MAKE:
            case R_CTRL_MAKE:
                ctrl_status = TRUE;
                break;
            case L_ALT_MAKE:
            case R_ALT_MAKE:
                alt_status = TRUE;
                break;
            case CAPSLOCK_MAKE:
                capslock_status = !capslock_status;
                break;
        }
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

PUBLIC void ps2_keyboard_init()
{
    ctrl_status     = 0;
    shift_status    = 0;
    alt_status      = 0;
    capslock_status = 0;
    ext_scandcode   = 0;
    register_handle(0x21,intr_keyboard_handler);
    ioapic_enable(1,0x21);

    wait_keyboard_ready();
    io_out8(KEYBOARD_CMD_PORT,KEYBOARD_WRITE_MD);
    wait_keyboard_ready();
    io_out8(KEYBOARD_BUF_PORT,0x47);

    return;
}