/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <std/stdarg.h>
#include <std/stdio.h>
#include <io.h>         // io_hlt,io_cli
#include <device/cpu.h>

#include <log.h>

extern PUBLIC uint8_t ascii_character[][16];

typedef struct position_s
{
    uint32_t x;
    uint32_t y;
} position_t;

#define X_START (8  + 1)
#define Y_START (16 + 2)

#define CHAR_X_SIZE (8  + 1)
#define CHAR_Y_SIZE (16 + 2)

PRIVATE position_t pos = {X_START,Y_START};



#define IS_TRANSMIT_EMPTY(port) (io_in8(port + 5) & 0x20)

#ifdef __DISABLE_SERIAL_LOG__
#define serial_pr_log(...) (void)0
#else
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
        buf = (char*)level[*log - 1];
        while (IS_TRANSMIT_EMPTY(port) == 0);
        if(*buf != 0)
        {
            do
            {
                while (IS_TRANSMIT_EMPTY(port) == 0);
                io_out8(port,*buf);
            } while (*buf++);
        }
    }
    vsprintf(msg,log,ap);
    buf = msg;
    while (IS_TRANSMIT_EMPTY(port) == 0);
    if(*buf != 0)
    {
        do
        {
            while (IS_TRANSMIT_EMPTY(port) == 0);
            io_out8(port,*buf++);
        } while (*buf);

    }
    return;
}
#endif /* __DISABLE_SERIAL_LOG__ */

#undef IS_TRANSMIT_EMPTY

PUBLIC void pr_log(const char *log,...)
{
    char msg[256];
    char *buf;
    const char *level[] =
    {
        "[ INFO  ]",
        "[ DEBUG ]",
        "[ ERROR ]"

    };
    if (*log >= 1 && *log <= 3)
    {
        buf = (char*)level[*log - 1];
        if (*log - 1 == 0)
        {
            basic_print(0x0000c500,buf);
        }
        else if (*log - 1 == 1)
        {
            basic_print(0x00c59900,buf);
        }
        else if (*log - 1 == 2)
        {
            basic_print(0x00c50000,buf);
        }
        log = log + 1;
    }
    va_list ap;
    va_start(ap,log);
    vsprintf(msg,log,ap);
    buf = msg;
    basic_print(0x00c5c5c5,buf);

    va_start(ap,log);
    serial_pr_log(log,ap);
    return;
}

PUBLIC void basic_put_char(unsigned char c,uint32_t col)
{
    uint8_t *character = ascii_character[(uint32_t)c];
    int i;
    for (i = 0;i < 16;i++)
    {
        uint8_t data = character[i];
        uint32_t *pixel = \
            (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
            + (pos.y + i) * g_boot_info->graph_info.pixel_per_scanline \
            + pos.x;
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
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                + (pos.y + i) * g_boot_info->graph_info.pixel_per_scanline \
                + pos.x + CHAR_X_SIZE;
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

PRIVATE void clear_line(position_t *pos)
{
    int i;
    for (i = 0;i < CHAR_Y_SIZE;i++)
    {
        uint32_t j;
        for (j = 0;j < g_boot_info->graph_info.pixel_per_scanline;j++)
        {
            uint32_t *pixel =
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base
                + (i + pos->y) * g_boot_info->graph_info.pixel_per_scanline
                + j;
            *pixel = 0x00000000;
        }
    }
}

PUBLIC void basic_print(uint32_t col,const char *str)
{
    const char *s = str;
    while(*s)
    {
        if (*s == '\n'
            || pos.x
                 >= g_boot_info->graph_info.pixel_per_scanline - CHAR_X_SIZE)
        {
            basic_put_char(255,0);
            pos.x = X_START;
            pos.y += CHAR_Y_SIZE;
            pos.y > g_boot_info->graph_info.vertical_resolution - CHAR_Y_SIZE ?
                 pos.y = Y_START : 0;
            s++;
            // clear line
            clear_line(&pos);
            basic_put_char(255,0x00ffffff);
            continue;
        }
        if (*s == '\b')
        {
            s++;
            pos.x -= CHAR_X_SIZE;
            if (pos.x < X_START)
            {
                pos.x = X_START;
                pos.y -= CHAR_Y_SIZE;
            }
            pos.y < Y_START ? pos.y = Y_START : 0;
            continue;
        }
        if (*s == '\r')
        {
            s++;
            pos.x = X_START;
            continue;
        }
        int i;
        for (i = 0;i < CHAR_Y_SIZE;i++)
        {
            uint32_t *pixel =
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base
                + (pos.y + i) * g_boot_info->graph_info.pixel_per_scanline;
            uint32_t j;
            for (j = pos.x;j < pos.x + CHAR_X_SIZE;j++)
            {
                pixel[j] = 0x00000000;
            }
        }
        basic_put_char(*s++,col);
        pos.x += CHAR_X_SIZE;
    }
    return;
}

PUBLIC void panic_spin(
    const char* filename,
    int line,
    const char* func,
    const char* condition)
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
    pr_log("\3 %s:%d: %s\n",filename,line,condition);
    while (1) io_hlt();
}