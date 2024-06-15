#include <kernel/global.h>
#include <std/stdarg.h>
#include <std/stdio.h>

extern PUBLIC uint8_t ascii_character[][16];

typedef struct
{
    uint32_t x;
    uint32_t y;
} position_t;

#define X_START 10U
#define Y_START 10U

position_t pos = {X_START,Y_START};

PUBLIC void basic_put_char(char c,uint32_t col)
{
    if (c >= ' ' && c <= '~')
    {
        uint8_t *character = ascii_character[c - ' '];
        int i;
        for (i = 0;i < 16;i++)
        {
            uint8_t data = character[i];
            uint32_t *pixel = \
                (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                + (pos.y + i) * g_boot_info->graph_info.horizontal_resolution \
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
    }
    return;
}

PUBLIC void basic_print(uint32_t col,const char *str,...)
{
    char msg[128];
    va_list ap;
    va_start(ap,str);
    vsprintf(msg,str,ap);
    va_end(ap);
    char *s = msg;
    while(*s)
    {
        if (*s == '\n' || pos.x >= g_boot_info->graph_info.horizontal_resolution - 8)
        {
            pos.x = X_START;
            pos.y += 16;
            pos.y > g_boot_info->graph_info.vertical_resolution - 16 ? pos.y = Y_START : 0;
            s++;
            int i;
            for (i = 0;i < 16;i++)
            {
                uint32_t *pixel = \
                    (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                    + (pos.y + i) * g_boot_info->graph_info.horizontal_resolution;
                uint32_t j;
                for (j = 0;j < g_boot_info->graph_info.horizontal_resolution;j++){ pixel[j] = 0x00000000; }
            }
            continue;
        }
        if (*s == '\b')
        {
            pos.x -= 8;
            if (pos.x < X_START)
            {
                pos.x = X_START;
                pos.y -= 16;
            }
            pos.y < Y_START ? pos.y = Y_START : 0;
        }
        basic_put_char(*s++,col);
        pos.x += 8;
    }
    return;
}