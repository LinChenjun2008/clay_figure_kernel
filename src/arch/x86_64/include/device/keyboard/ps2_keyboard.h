// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __PS2_KEYBOARD_H__
#define __PS2_KEYBOARD_H__

#define KEYBOARD_WRITE_MD 0x60

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STA_PORT  0x64
#define KEYBOARD_CMD_PORT  0x64

#define KEYBOARD_NOTREADY 0x02

PUBLIC void ps2_keyboard_init(void);

#endif