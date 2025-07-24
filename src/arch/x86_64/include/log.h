// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __LOG_H__
#define __LOG_H__


#define ALPHA_MASK  0xff
#define ALPHA_SHIFT 24

#define RED_MASK  0xff
#define RED_SHIFT 16

#define GREEN_MASK  0xff
#define GREEN_SHIFT 8

#define BLUE_MASK  0xff
#define BLUE_SHIFT 0

#define RGB(r, g, b) \
    (SET_FIELD(0, RED, r) | SET_FIELD(0, GREEN, g) | SET_FIELD(0, BLUE, b))

#define ARGB(a, r, g, b) (SET_FIELD(RGB(r, g, b), ALPHA, a))

typedef struct position_s
{
    uint32_t x;
    uint32_t y;
} position_t;

typedef struct textbox_s
{
    position_t cur_pos;
    position_t box_pos;
    uint32_t   xsize;
    uint32_t   ysize;
    uint32_t   char_xsize; // VGA Fonts
    uint32_t   char_ysize; // VGA Fonts
} textbox_t;

#include "stb_truetype.h"
typedef struct
{
    stbtt_fontinfo info;
    int            has_ttf;
    uint8_t       *bitmap;
} ttf_info_t;

// log level
#define DEBUG_LEVEL 6

#define LOG_FATAL 1
#define LOG_ERROR 2
#define LOG_WARN  3
#define LOG_INFO  4
#define LOG_DEBUG 5

PUBLIC void pr_log(int level, const char *log, ...);
PUBLIC void pr_msg(const char *msg, ...);

PUBLIC void
basic_put_char(graph_info_t *gi, textbox_t *tb, unsigned char c, uint32_t col);

PUBLIC void
basic_print(graph_info_t *gi, textbox_t *tb, uint32_t col, const char *str);

PUBLIC void clear_textbox(graph_info_t *gi, textbox_t *tb);

// PUBLIC void pr_log_ttf(const char* str,...);

PUBLIC void panic_spin(
    const char *filename,
    int         line,
    const char *func,
    const char *message
);

#define PR_LOG(LEVEL, MESSAGE, args...) \
    pr_log(LEVEL, "%s: " MESSAGE, __func__, ##args)

#define PANIC(CONDITION, MESSAGE)                              \
    do                                                         \
    {                                                          \
        if (CONDITION)                                         \
        {                                                      \
            panic_spin(__FILE__, __LINE__, __func__, MESSAGE); \
        }                                                      \
    } while (0)

#if defined __DISABLE_ASSERT__

#    define ASSERT(X) ((void)0)

#else

#    define ASSERT(X)                        \
        do                                   \
        {                                    \
            if (!(X))                        \
            {                                \
                PANIC(#X, "ASSERT(" #X ")"); \
            }                                \
        } while (0)

#endif /* __DISABLE_ASSERT__ */

PUBLIC void init_ttf_info(ttf_info_t *ttf_info);
PUBLIC void free_ttf_info(ttf_info_t *ttf_info);

PUBLIC void pr_ch(
    graph_info_t *graph_info,
    ttf_info_t   *ttf_info,
    textbox_t    *tb,
    uint32_t      col,
    uint64_t      ch,
    float         font_size
);

PUBLIC void pr_ttf_str(
    graph_info_t *graph_info,
    ttf_info_t   *ttf_info,
    textbox_t    *tb,
    uint32_t      color,
    const char   *str,
    float         font_size
);

#endif