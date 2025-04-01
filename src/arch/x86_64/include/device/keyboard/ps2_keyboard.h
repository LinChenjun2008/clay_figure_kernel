/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __PS2_KEYBOARD_H__
#define __PS2_KEYBOARD_H__

#define KEYBOARD_WRITE_MD  0x60

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STA_PORT  0x64
#define KEYBOARD_CMD_PORT  0x64

#define KEYBOARD_NOTREADY  0x02

PUBLIC void ps2_keyboard_init(void);

PUBLIC void keyboard_main(void);

#endif