/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __LOG_H__
#define __LOG_H__


#define ALPHA_MASK 0xff
#define ALPHA_SHIFT 24

#define RED_MASK 0xff
#define RED_SHIFT 16

#define GREEN_MASK 0xff
#define GREEN_SHIFT 8

#define BLUE_MASK 0xff
#define BLUE_SHIFT 0

#define RGB(r,g,b) (SET_FIELD(0,RED,r) \
                   | SET_FIELD(0,GREEN,g) \
                   | SET_FIELD(0,BLUE,b))

#define ARGB(a,r,g,b) (SET_FIELD(0,ALPHA,a) \
                      | SET_FIELD(0,RED,r) \
                      | SET_FIELD(0,GREEN,g) \
                      | SET_FIELD(0,BLUE,b))

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
    int has_ttf;
    uint8_t *bitmap;
} ttf_info_t;

#if defined __DISABLE_SERIAL_LOG__
#define serial_pr_log(...) (void)0
#endif /* __DISABLE_SERIAL_LOG__ */

PUBLIC void pr_log(const char* str,...);
PUBLIC void pr_log_ttf(const char* str,...);

PUBLIC void panic_spin(
    const char* filename,
    int line,
    const char* func,
    const char* message);

#define PANIC(CONDITION,MESSAGE) \
    do \
    { \
        if (CONDITION) {panic_spin (__FILE__,__LINE__,__FUNCTION__,MESSAGE);} \
    } while (0)

#if __DISABLE_ASSERT__

#define ASSERT(X) ((void)0)

#else

#define ASSERT(X) \
    do \
    { \
        if (!(X)) { PANIC(#X,"ASSERT("#X")"); } \
    } while (0)

#endif /* __DISABLE_ASSERT__ */

PUBLIC void init_ttf_info(ttf_info_t* ttf_info);
PUBLIC void free_ttf_info(ttf_info_t* ttf_info);

PUBLIC void pr_ch(graph_info_t* graph_info,
                  ttf_info_t *ttf_info,
                  textbox_t* tb,
                  uint32_t col,
                  uint64_t ch,
                  float font_size);

PUBLIC void pr_ttf_str(graph_info_t* graph_info,
                       ttf_info_t *ttf_info,
                       textbox_t* tb,
                       uint32_t color,
                       const char* str,
                       float font_size);

#endif