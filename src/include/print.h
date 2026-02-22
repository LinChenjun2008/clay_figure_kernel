// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __PRINT_H__
#define __PRINT_H__

#define MSG_INFO "\033[102;30m INFO  \033[0m "
#define MSG_WARN "\033[103;30m WARN  \033[0m "
#define MSG_DBG  "\033[106;30m DEBUG \033[0m "
#define MSG_ERR  "\033[101;30m ERROR \033[0m "

#define MSG_HIGHLIGHT(MSG) "\033[97m" MSG "\033[0m"

#define PANIC(MESSAGE)                                               \
    do                                                               \
    {                                                                \
        panic_spin(__func__, __LINE__, MSG_HIGHLIGHT(MESSAGE) "\n"); \
    } while (0)

#define ASSERT(CONDITION)                                             \
    do                                                                \
    {                                                                 \
        if (!(CONDITION))                                             \
        {                                                             \
            panic_spin(                                               \
                __func__,                                             \
                __LINE__,                                             \
                "Assertion '" MSG_HIGHLIGHT(#CONDITION) "' failed.\n" \
            );                                                        \
        }                                                             \
    } while (0)

struct textbox
{
    uint32_t *fb;   // frame buffer
    uint32_t  ppsl; // pixel per scanline

    int box_size[2];
    int char_size[2];

    int box_positon[2];  // textbox's position
    int cur_position[2]; // cursor's positon

    int char_limit[2];

    uint32_t default_ch_color;
    uint32_t default_bg_color;

    uint32_t ch_color;
    uint32_t bg_color;
};

void basic_put_char(struct textbox *textbox, int x, int y, uint8_t c);

void init_print(struct graphic_info *graphic_info);

int  printk(const char *fmt, ...);
void panic_spin(const char *function, int line, const char *message);

#endif