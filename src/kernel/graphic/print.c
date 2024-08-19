#include <kernel/global.h>
#include <std/stdarg.h>
#include <std/stdio.h>
#include <device/cpu.h>
#include <device/spinlock.h>

#include <log.h>

extern PUBLIC uint8_t ascii_character[][16];

PUBLIC volatile uint32_t global_log_cnt;

typedef struct position_s
{
    uint32_t x;
    uint32_t y;
} position_t;

#define X_START 10U
#define Y_START 10U

#define CHAR_X_SIZE (8  + 1)
#define CHAR_Y_SIZE (16 + 1)

PRIVATE position_t pos = {X_START,Y_START};

PUBLIC void basic_put_char(char c,uint32_t col)
{
    uint8_t *character = ascii_character[(uint32_t)c];
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
        if (*s == '\n'
            || pos.x
                 >= g_boot_info->graph_info.horizontal_resolution - CHAR_X_SIZE)
        {
            pos.x = X_START;
            pos.y += CHAR_Y_SIZE;
            pos.y > g_boot_info->graph_info.vertical_resolution - CHAR_Y_SIZE ?
                 pos.y = Y_START : 0;
            s++;
            int i;
            for (i = 0;i < CHAR_Y_SIZE;i++)
            {
                uint32_t *pixel = \
                    (uint32_t*)g_boot_info->graph_info.frame_buffer_base \
                    + (pos.y + i) * g_boot_info->graph_info.horizontal_resolution;
                uint32_t j;
                for (j = 0;j < g_boot_info->graph_info.horizontal_resolution;j++)
                {
                    pixel[j] = 0x00000000;
                }
            }
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
        basic_put_char(*s++,col);
        pos.x += CHAR_X_SIZE;
    }
    return;
}

PRIVATE spinlock_t log_lock = { 1 };

PUBLIC void panic_spin(
    char* filename,
    int line,
    const char* func,
    const char* condition)
{
    spinlock_lock(&log_lock);
    __asm__ __volatile__ ("cli":::);
    pr_log("\n");
    pr_log("\3 >>> PANIC <<<\n");
    pr_log("\3 %s: In function '%s':\n",filename,func);
    pr_log("\3 %s:%d: %s\n",filename,line,condition);
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
    while (1) __asm__ __volatile("cli\n\t""hlt":::);
}