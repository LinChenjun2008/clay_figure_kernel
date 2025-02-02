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

#include "stb_truetype.h"
typedef struct
{
    stbtt_fontinfo info;
    int has_ttf;
    uint8_t *bitmap;
} ttf_info_t;

PUBLIC void basic_put_char(unsigned char c,uint32_t col);
PUBLIC void basic_print(uint32_t col,const char *str);

#if defined __DISABLE_SERIAL_LOG__
#define serial_pr_log(...) (void)0
#endif /* __DISABLE_SERIAL_LOG__ */

PUBLIC void pr_log(const char* str,...);

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

#endif