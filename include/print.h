// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __PRINT_H__
#define __PRINT_H__

#include <print/format.h>

struct textbox
{
    uint32_t *fb;   // frame buffer
    uint32_t  ppsl; // pixel per scanline

    int box_size[2];
    int char_size[2];

    int box_positon[2];
    int cur_position[2];

    int char_limit[2];

    uint32_t default_ch_color;
    uint32_t default_bg_color;

    uint32_t ch_color;
    uint32_t bg_color;
};

void basic_put_char(struct textbox *tb, int x, int y, uint8_t c);

void init_print(struct graphic_info *graphic_info);

int printk(const char *fmt, ...);

#endif