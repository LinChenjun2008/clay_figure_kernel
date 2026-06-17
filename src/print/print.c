// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <asm/mem/page.h> // PHYS_TO_VIRT
#include <asm/sync/spinlock.h>
#include <asm/utils.h>

#include <print.h>
#include <std/stdio.h>
#include <std/string.h> // strncmp

#define IS_TRANSMIT_EMPTY(port) (io_in8(port + 5) & 0x20)
#define SERIAL_PORT             0x3f8

#define X 0
#define Y 1

static uint32_t palette[8][2] = {
    { 0x000000, 0x555555 }, // black
    { 0xaa0000, 0xee3333 }, // red
    { 0x22aa66, 0x44ffaa }, // green
    { 0xcccc00, 0xffee44 }, // yellow
    { 0x3366aa, 0x7799cc }, // blue
    { 0xaa4488, 0xdd66bb }, // magenta
    { 0x0088aa, 0x33eeee }, // cyan
    { 0xaaaaaa, 0xeeeeee }, // white
};

extern uint8_t font_8x16[256][16];

static struct spinlock print_lock;
static struct spinlock panic_lock;

static struct textbox textbox;

void basic_put_char(struct textbox *tb, int x, int y, uint8_t c)
{
    uint8_t *character = font_8x16[(uint32_t)c];

    uint32_t ch_color = tb->ch_color;
    uint32_t bg_color = tb->bg_color;

    uint32_t *pixel;
    uint8_t   data;

    int i, j;
    for (i = 0; i < tb->char_size[Y]; i++)
    {
        pixel = (uint32_t *)tb->fb + (y + i) * tb->ppsl + x;

        for (j = 0; j < tb->char_size[X]; j++)
        {
            pixel[j] = bg_color;
        }
    }
    for (i = 0; i < 16; i++)
    {
        data  = character[i];
        pixel = (uint32_t *)tb->fb + (y + i) * tb->ppsl + x;

        for (j = 0; j < 8; j++)
        {
            pixel[j] = bg_color;
        }
        if ((data & 0x80) != 0) pixel[0] = ch_color;
        if ((data & 0x40) != 0) pixel[1] = ch_color;
        if ((data & 0x20) != 0) pixel[2] = ch_color;
        if ((data & 0x10) != 0) pixel[3] = ch_color;
        if ((data & 0x08) != 0) pixel[4] = ch_color;
        if ((data & 0x04) != 0) pixel[5] = ch_color;
        if ((data & 0x02) != 0) pixel[6] = ch_color;
        if ((data & 0x01) != 0) pixel[7] = ch_color;
    }
    return;
}

static void serial_printk(uint16_t port, char *buf)
{
    while (IS_TRANSMIT_EMPTY(port) == 0);
    if (*buf != 0)
    {
        do
        {
            while (IS_TRANSMIT_EMPTY(port) == 0);
            io_out8(port, *buf);
            if (*buf == '\n')
            {
                io_out8(port, '\r');
            }
        } while (*buf++);
    }
    return;
}

void init_print(struct graphic_info *graphic_info)
{
    uint32_t  char_xsize = 8;
    uint32_t  char_ysize = 16;
    uint32_t *fb         = PHYS_TO_VIRT(graphic_info->frame_buffer_base);
    uint32_t  ppsl       = graphic_info->pixel_per_scanline;
    uint32_t  xsize      = graphic_info->horizontal_resolution - 2 * char_xsize;
    uint32_t  ysize      = graphic_info->vertical_resolution - 2 * char_ysize;

    textbox.fb   = fb;
    textbox.ppsl = ppsl;

    textbox.box_size[X] = xsize;
    textbox.box_size[Y] = ysize;

    textbox.char_size[X] = char_xsize;
    textbox.char_size[Y] = char_ysize;

    textbox.box_positon[X] = char_xsize;
    textbox.box_positon[Y] = char_ysize;

    textbox.cur_position[X] = 0;
    textbox.cur_position[Y] = 0;

    textbox.char_limit[X] = textbox.box_size[X] / textbox.char_size[X];
    textbox.char_limit[Y] = textbox.box_size[Y] / textbox.char_size[Y];

    textbox.default_ch_color = palette[7][0];
    textbox.default_bg_color = palette[0][0];

    textbox.ch_color = textbox.default_ch_color;
    textbox.bg_color = textbox.default_bg_color;
    init_spinlock(&print_lock);
    init_spinlock(&panic_lock);

    char str[10];
    int  i;
    for (i = 40; i < 48; i++)
    {
        sprintf(str, "\033[%dm   \033[0m", i);
        printk(str);
    }
    printk("\n");
    for (i = 100; i < 108; i++)
    {
        sprintf(str, "\033[%dm   \033[0m", i);
        printk(str);
    }
    printk("\n");
    return;
}

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)
{
    int i = 0;
    while (IS_DIGIT(**s))
    {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

static int analyze_sgr(struct textbox *tb, const char *str, int cur)
{
    const char *s = str + cur;
    if (*s != '\033')
    {
        return (uintptr_t)s - (uintptr_t)str;
    }
    s++;
    if (*s != '[')
    {
        return (uintptr_t)s - (uintptr_t)str;
    }
    s++;

    int param;
    while (*s != 'm' && *s != '\0')
    {
        if (IS_DIGIT(*s))
        {
            param = skip_atoi(&s);
            if (param == 0)
            {
                tb->ch_color = tb->default_ch_color;
                tb->bg_color = tb->default_bg_color;
            }
            // char color
            if (param >= 30 && param <= 37)
                tb->ch_color = palette[param - 30][0];
            if (param >= 90 && param <= 97)
                tb->ch_color = palette[param - 90][1];
            if (param == 39)
            {
                tb->ch_color = tb->default_ch_color;
            }

            // background color
            if (param >= 40 && param <= 47)
                tb->bg_color = palette[param - 40][0];
            if (param >= 100 && param <= 107)
                tb->bg_color = palette[param - 100][1];
            if (param == 49)
            {
                tb->bg_color = tb->default_bg_color;
            }
        }
        else if (*s == ';')
        {
            s++;
        }
        else
        {
            s++;
            break;
        }
    }

    // skip 'm'
    if (*s == 'm') s++;

    return (uintptr_t)s - (uintptr_t)str;
}

static void clear_line(struct textbox *tb)
{
    uint32_t  bg_color = tb->default_bg_color;
    uint32_t *pixel;

    int y0 = tb->box_positon[Y] + tb->cur_position[Y] * tb->char_size[Y];

    int x, y;
    for (y = y0; y < y0 + tb->char_size[Y]; y++)
    {
        for (x = 0; x < tb->char_limit[X] * tb->char_size[X]; x++)
        {
            pixel  = tb->fb + y * tb->ppsl + x;
            *pixel = bg_color;
        }
    }
    return;
}

int printk(const char *fmt, ...)
{
    spin_lock(&print_lock);

    char    buf[256];
    va_list ap;
    va_start(ap, fmt);
    int len;
    len = vsprintf(buf, fmt, ap);
    char    ch;
    int32_t x, y;

    int i;
    for (i = 0; i < len; i++)
    {
        ch = buf[i];

        if (textbox.cur_position[X] + 1 > textbox.char_limit[X])
        {
            textbox.cur_position[X] = 0;
            textbox.cur_position[Y]++;
            clear_line(&textbox);
        }
        if (textbox.cur_position[Y] + 1 > textbox.char_limit[Y])
        {
            textbox.cur_position[Y] = 0;
            clear_line(&textbox);
        }
        switch (ch)
        {
            case '\n':
                textbox.cur_position[Y]++;
                clear_line(&textbox);
            case '\r':
                textbox.cur_position[X] = 0;
                continue;
                break;

            case '\b':
                if (textbox.cur_position[X] > 0)
                {
                    textbox.cur_position[X]--;
                }
                continue;
                break;

            case '\t':
                textbox.cur_position[X] = (textbox.cur_position[X] + 8) & ~0x07;
                continue;
                break;

            case '\033':
                i = analyze_sgr(&textbox, buf, i) - 1;
                continue;
                break;
        }
        x = textbox.box_positon[X] +
            textbox.cur_position[X] * textbox.char_size[X];

        y = textbox.box_positon[Y] +
            textbox.cur_position[Y] * textbox.char_size[Y];

        basic_put_char(&textbox, x, y, ch);
        textbox.cur_position[X]++;
    }
    serial_printk(SERIAL_PORT, buf);

    spin_unlock(&print_lock);
    return len;
}

void panic_spin(const char *function, int line, const char *message)
{
    intr_disable();
    spin_lock(&panic_lock);
    printk(MSG_ERR MSG_HIGHLIGHT("%s:%d: "), function, line);
    printk(message);
    asm_panic();
    while (1);

    return;
}