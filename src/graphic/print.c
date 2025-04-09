/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <std/stdarg.h>
#include <std/stdio.h>
#include <io.h>           // io_hlt,io_cli
#include <device/timer.h> // IRQ0_FREQUENCY
#include <device/cpu.h>

#include <log.h>

extern PUBLIC uint8_t ascii_character[][16];

PUBLIC textbox_t g_tb;

extern uint64_t global_ticks;

PRIVATE char *print_time(char *buf,uint64_t msec)
{
    sprintf(buf,"[%8u.%03u]",msec / IRQ0_FREQUENCY,msec % IRQ0_FREQUENCY);
    return buf;
}

#define IS_TRANSMIT_EMPTY(port) (io_in8(port + 5) & 0x20)

#ifdef __DISABLE_SERIAL_LOG__
#define serial_pr_log(...) (void)0
#else

static void serial_pr_log_sub(uint16_t port,char *buf)
{
    while (IS_TRANSMIT_EMPTY(port) == 0);
    if(*buf != 0)
    {
        do
        {
            while (IS_TRANSMIT_EMPTY(port) == 0);
            io_out8(port,*buf);
        } while (*buf++);
    }
    return;
}

static inline void serial_pr_log(const char *log,va_list ap)
{
    char msg[256];
    char *buf;
    uint16_t port = 0x3f8;
    const char *level[] =
    {
        "\033[32m[ INFO  ]\033[0m ",
        "\033[33m[ DEBUG ]\033[0m ",
        "\033[31m[ ERROR ]\033[0m ",
    };
    if (*log >= 1 && *log <= 3)
    {
        serial_pr_log_sub(port,print_time(msg,global_ticks));
        buf = (char*)level[*log - 1];
        serial_pr_log_sub(port,buf);
    }
    vsprintf(msg,log,ap);
    buf = msg;
    serial_pr_log_sub(port,buf);
    return;
}

#endif /* __DISABLE_SERIAL_LOG__ */

#undef IS_TRANSMIT_EMPTY

PRIVATE void basic_put_char(textbox_t *tb,unsigned char c,uint32_t col);
PRIVATE void basic_print(textbox_t *tb,uint32_t col,const char *str);

PUBLIC void pr_log(const char *log,...)
{
    char msg[256];
    char *buf;
    va_list ap;
    const char *level[] =
    {
        "[ INFO  ]",
        "[ DEBUG ]",
        "[ ERROR ]"

    };

    va_start(ap,log);
    serial_pr_log(log,ap);

    if (*log >= 1 && *log <= 3)
    {
        basic_print(&g_tb,0x00c5c5c5,print_time(msg,global_ticks));
        buf = (char*)level[*log - 1];
        uint32_t color = 0;
        switch (*log)
        {
            case 1:
                color = 0x0000c500;
                break;
            case 2:
                color = 0x00c59900;
                break;
            case 3:
                color = 0x00c50000;
                break;
        }
        basic_print(&g_tb,color,buf);
        log = log + 1;
    }
    va_start(ap,log);
    vsprintf(msg,log,ap);
    buf = msg;
    basic_print(&g_tb,0x00c5c5c5,buf);

    va_end(ap);
    return;
}

PRIVATE void basic_put_char(textbox_t *tb,unsigned char c,uint32_t col)
{
    uint8_t *character = ascii_character[(uint32_t)c];
    int i;
    for (i = 0;i < 16;i++)
    {
        uint8_t data = character[i];
        uint32_t *pixel = \
            (uint32_t*)g_graph_info->frame_buffer_base \
            + (tb->box_pos.y + tb->cur_pos.y + i) * g_graph_info->pixel_per_scanline \
            + tb->box_pos.x + tb->cur_pos.x;
        int j;
        for (j = 0;j < 8;j++){ pixel[j] = 0x00000000; }
        if ((data & 0x80) != 0){ pixel[0] = col; }
        if ((data & 0x40) != 0){ pixel[1] = col; }
        if ((data & 0x20) != 0){ pixel[2] = col; }
        if ((data & 0x10) != 0){ pixel[3] = col; }
        if ((data & 0x08) != 0){ pixel[4] = col; }
        if ((data & 0x04) != 0){ pixel[5] = col; }
        if ((data & 0x02) != 0){ pixel[6] = col; }
        if ((data & 0x01) != 0){ pixel[7] = col; }
    }
    if (c != 255)
    {
        col = 0x00ffffff;
        character = ascii_character[255];
        for (i = 0;i < 16;i++)
        {
            uint8_t data = character[i];
            uint32_t *pixel = \
                (uint32_t*)g_graph_info->frame_buffer_base \
                + (tb->box_pos.y + tb->cur_pos.y + i) * g_graph_info->pixel_per_scanline \
                + tb->box_pos.x + tb->cur_pos.x + tb->char_xsize;
            int j;
            for (j = 0;j < 8;j++){ pixel[j] = 0x00000000; }
            if ((data & 0x80) != 0){ pixel[0] = col; }
            if ((data & 0x40) != 0){ pixel[1] = col; }
            if ((data & 0x20) != 0){ pixel[2] = col; }
            if ((data & 0x10) != 0){ pixel[3] = col; }
            if ((data & 0x08) != 0){ pixel[4] = col; }
            if ((data & 0x04) != 0){ pixel[5] = col; }
            if ((data & 0x02) != 0){ pixel[6] = col; }
            if ((data & 0x01) != 0){ pixel[7] = col; }
        }
    }

    return;
}

PRIVATE void clear_line(textbox_t *tb)
{
    uint32_t i;
    for (i = 0;i < tb->char_ysize;i++)
    {
        uint32_t j;
        for (j = 0;j < tb->xsize;j++)
        {
            uint32_t *pixel =
            (uint32_t*)g_graph_info->frame_buffer_base
            + (tb->box_pos.y + tb->cur_pos.y + i) * g_graph_info->pixel_per_scanline
            + (j + tb->box_pos.x);
            *pixel = 0x00000000;
        }
    }
}

PRIVATE void basic_print(textbox_t *tb,uint32_t col,const char *str)
{
    const char *s = str;
    while(*s)
    {
        uint32_t max_x,max_y;
        max_x = tb->xsize - tb->char_xsize;
        max_y = tb->ysize - tb->char_ysize;
        if (*s == '\n' || tb->cur_pos.x >= max_x)
        {
            basic_put_char(tb,255,0);
            tb->cur_pos.x = 0;
            tb->cur_pos.y += tb->char_ysize;
            if (tb->cur_pos.y > max_y)
            {
                tb->cur_pos.y = 0;
            }
            s++;
            // clear line
            clear_line(tb);
            basic_put_char(tb,255,0x00ffffff);
            continue;
        }
        if (*s == '\b')
        {
            s++;
            if (tb->cur_pos.x >= tb->char_xsize)
            {
                tb->cur_pos.x -= tb->char_xsize;
            }
            else
            {
                tb->cur_pos.x = 0;
                if (tb->cur_pos.y >= tb->char_ysize)
                {
                    tb->cur_pos.y -= tb->char_ysize;
                }
                else
                {
                    tb->cur_pos.y = 0;
                }
            }
            continue;
        }
        if (*s == '\r')
        {
            s++;
            tb->cur_pos.x = 0;
            continue;
        }
        basic_put_char(tb,*s++,col);
        tb->cur_pos.x += tb->char_xsize;
    }
    return;
}

extern void asm_debug_intr();
PUBLIC void panic_spin(
    const char* filename,
    int line,
    const char* func,
    const char* message)
{
    uint64_t icr;
    icr = make_icr(
        0x81,
        ICR_DELIVER_MODE_FIXED,
        ICR_DEST_MODE_PHY,
        ICR_DELIVER_STATUS_IDLE,
        ICR_LEVEL_DE_ASSEST,
        ICR_TRIGGER_EDGE,
        ICR_ALL_EXCLUDE_SELF,
        0);
    send_IPI(icr);
    io_cli();
    pr_log("\n");
    pr_log("\3 >>> PANIC <<<\n");
    pr_log("\3 %s: In function '%s':\n",filename,func);
    pr_log("\3 %s:%d: %s\n",filename,line,message);
    asm_debug_intr();
    while (1) io_hlt();
}



/******************
   TrueType Font
******************/

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

PUBLIC void init_ttf_info(ttf_info_t* ttf_info)
{
    ttf_info->has_ttf = 0;
    int i;
    for (i = 0;i < g_boot_info->loaded_files;i++)
    {
        if (g_boot_info->loaded_file[i].flag == 0x80000002)
        {
            stbtt_InitFont(&ttf_info->info,
                          KADDR_P2V(g_boot_info->loaded_file[i].base_address),
                          0);
            status_t status = pmalloc(sizeof(char [512*512]),&ttf_info->bitmap);
            if (ERROR(status))
            {
                pr_log("\3Failed to allocate memory for ttf bitmap.\n");
                return;
            }
            ttf_info->has_ttf = 1;
            break;
        }
    }
    return;
}

PUBLIC void free_ttf_info(ttf_info_t* ttf_info)
{
    if (ttf_info->has_ttf)
    {
        pfree(ttf_info->bitmap);
        ttf_info->has_ttf = 0;
    }
    return;
}

PUBLIC void pr_log_ttf(const char *log,...)
{
    ttf_info_t ttf_info;
    init_ttf_info(&ttf_info);

    char msg[256];
    char *buf;
    va_list ap;
    const char *level[] =
    {
        "[ INFO  ]",
        "[ DEBUG ]",
        "[ ERROR ]"

    };

    va_start(ap,log);
    serial_pr_log(log,ap);

    if (*log >= 1 && *log <= 3)
    {
        if (ttf_info.has_ttf)
        {
            pr_ttf_str(g_graph_info,
                       &ttf_info,
                       &g_tb,
                       0x00c5c5c5,
                       print_time(msg,global_ticks),
                       16.0);
        }
        else
        {
            basic_print(&g_tb,0x00c5c5c5,print_time(msg,global_ticks));
        }
        buf = (char*)level[*log - 1];
        uint32_t color = 0;
        switch (*log)
        {
            case 1:
                color = 0x0000c500;
                break;
            case 2:
                color = 0x00c59900;
                break;
            case 3:
                color = 0x00c50000;
                break;
        }
        if (ttf_info.has_ttf)
        {
            pr_ttf_str(g_graph_info,
                       &ttf_info,
                       &g_tb,
                       color,
                       buf,
                       16.0);
        }
        else
        {
            basic_print(&g_tb,color,buf);
        }
        log = log + 1;
    }
    va_start(ap,log);
    vsprintf(msg,log,ap);
    buf = msg;
    if (ttf_info.has_ttf)
    {
        pr_ttf_str(g_graph_info,
                    &ttf_info,
                    &g_tb,
                    0x00c5c5c5,
                    buf,
                    16.0);
    }
    else
    {
        basic_print(&g_tb,0x00c5c5c5,buf);
    }
    free_ttf_info(&ttf_info);
    va_end(ap);
    return;
}

PUBLIC void pr_ch(graph_info_t* graph_info,
                  ttf_info_t *ttf_info,
                  textbox_t* tb,
                  uint32_t col,
                  uint64_t ch,
                  float font_size)
{
    if (!ttf_info->has_ttf)
    {
        return;
    }
    /* scale = font_size / (ascent - descent) */
    float scale = stbtt_ScaleForPixelHeight(&ttf_info->info, font_size);
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&ttf_info->info, &ascent, &descent, &lineGap);
    ascent = ceil(ascent * scale);
    descent = ceil(descent * scale);

    int advanceWidth = 0;
    int leftSideBearing = 0;
    stbtt_GetCodepointHMetrics(&ttf_info->info,
                               ch,
                               &advanceWidth,
                               &leftSideBearing);
    int c_x1, c_y1, c_x2, c_y2;
    stbtt_GetCodepointBitmapBox(&ttf_info->info,
                                ch,
                                scale,
                                scale,
                                &c_x1,
                                &c_y1,
                                &c_x2,
                                &c_y2);

    int y = ascent + c_y1;

    int x = 0;
    int byteOffset = x + ceil(leftSideBearing * scale) + (y * font_size);
    stbtt_MakeCodepointBitmap(&ttf_info->info,
                              ttf_info->bitmap + byteOffset,
                              c_x2 - c_x1,
                              c_y2 - c_y1,
                              (int)font_size,
                              scale,
                              scale,
                              ch);
    uint32_t* frame_buffer = (uint32_t*)graph_info->frame_buffer_base;
    unsigned int xsize = graph_info->pixel_per_scanline;

    int x0,y0;
    for (x0 = 0;x0 < font_size;x0++)
    {
        for (y0 = 0;y0 < font_size;y0++)
        {
            if (ttf_info->bitmap[y0 * (int)font_size + x0])
            {
                uint8_t r,g,b;
                uint8_t alpha = ttf_info->bitmap[y0 * (int)font_size + x0];
                r = (alpha * GET_FIELD(col,RED)) >> 8;
                g = (alpha * GET_FIELD(col,GREEN)) >> 8;
                b = (alpha * GET_FIELD(col,BLUE)) >> 8;
                uint32_t *buf;
                buf = frame_buffer
                      + (tb->box_pos.y + tb->cur_pos.y + y0) * xsize
                      + tb->box_pos.x + tb->cur_pos.x + x0;
                *buf = RGB(r,g,b);
            }
        }
    }
}

PRIVATE uint64_t utf8_decode(const char** _str)
{
    unsigned char* str = *((unsigned char**)_str);
    uint64_t code = 0;
    if ((*str >> 7) == 0)
    {
        code = *str;
        str++;
    }
    else if (((*str >> 5) & 0x0f) == 0x6)
    {
        code = (*str & 0x1f) << 6;
        str++;
        code |= (*str & 0x3f);
        str++;
    }
    else if (((*str >> 4) & 0xf) == 0xe)
    {
        code = (*str & 0x0f) << 12;
        str++;
        code |= (*str & 0x3f) << 6;
        str++;
        code |= (*str & 0x3f) << 0;
        str++;
    }
    *_str = (char*)str;
    return code;
}

PUBLIC void pr_ttf_str(graph_info_t* graph_info,
                       ttf_info_t *ttf_info,
                       textbox_t* tb,
                       uint32_t color,
                       const char* str,
                       float font_size)
{
    if (!ttf_info->has_ttf)
    {
        basic_print(tb,color,str);
        return;
    }
    font_size *= 2;
    float scale = stbtt_ScaleForPixelHeight(&ttf_info->info, font_size);
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&ttf_info->info, &ascent, &descent, &lineGap);
    ascent = ceil(ascent * scale);
    descent = ceil(descent * scale);
    uint64_t code = 0;
    while (*str)
    {
        code = utf8_decode(&str);
        int advanceWidth = 0;
        int leftSideBearing = 0;
        const char* next = str;
        stbtt_GetCodepointHMetrics(&ttf_info->info,
                                   code,
                                   &advanceWidth,
                                   &leftSideBearing);
        int kern = stbtt_GetCodepointKernAdvance(&ttf_info->info,
                                                 code,
                                                 utf8_decode(&next));

        uint32_t char_xsize = ceil(advanceWidth * scale) + ceil(kern * scale);
        uint32_t char_ysize = (ascent - descent + lineGap);
        if (code == '\n')
        {
            tb->cur_pos.x = 0;
            tb->cur_pos.y += char_ysize;
            continue;
        }
        if (code == ' ')
        {
            tb->cur_pos.x += char_xsize;
            continue;
        }
        if (code == '\r')
        {
            tb->cur_pos.x = 0;
            continue;
        }
        memset(ttf_info->bitmap,0,sizeof(char [512*512]));
        pr_ch(graph_info,ttf_info,tb,color,code,font_size);

        tb->cur_pos.x += char_xsize;

        uint32_t max_x,max_y;
        max_x = tb->xsize - char_xsize;
        max_y = tb->ysize - char_ysize;
        if (tb->cur_pos.x >= max_x)
        {
            tb->cur_pos.x = 0;
            tb->cur_pos.y += char_ysize;
            if (tb->cur_pos.y > max_y)
            {
                tb->cur_pos.y = 0;
            }
        }
    }
    return;
}
