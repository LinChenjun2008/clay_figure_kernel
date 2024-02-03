#ifndef __LOG_H__
#define __LOG_H__

#include <io.h>
#include <std/string.h>
#include <std/stdarg.h>
#include <std/stdio.h>

PUBLIC void basic_put_char(char c,uint32_t col);
PUBLIC void basic_print(uint32_t col,const char *str,...);

#if __DISABLE_LOG__
static inline void pr_log(const char *log,...)
{
    char msg[128];
    char *buf;
    uint16_t port = 0x3f8;
    char *level[] =
    {
        "[ INFO  ] ",
        "[ DEBUG ] ",
        "[ ERROR ] "
    };
    if (*log >= 1 && *log <= 3)
    {
        buf = level[*log - 1];
        basic_print(0x0000ff00,buf);
    }
    va_list ap;
    va_start(ap,log);
    vsprintf(msg,log,ap);
    buf = msg;
    basic_print(0x00ffffff,buf);
    return;
}
#else
#define IS_TRANSMIT_EMPTY(port) (io_in8(port + 5) & 0x20)

static inline void pr_log(const char *log,...)
{
    char msg[128];
    char *buf;
    uint16_t port = 0x3f8;
    char *level[] =
    {
        "\033[32m[ INFO  ]\033[0m ",
        "\033[33m[ DEBUG ]\033[0m ",
        "\033[31m[ ERROR ]\033[0m ",
        "[ INFO  ] ",
        "[ DEBUG ] ",
        "[ ERROR ] "
    };
    if (*log >= 1 && *log <= 3)
    {
        buf = level[*log - 1];
        while (IS_TRANSMIT_EMPTY(port) == 0);
        if(*buf != 0)
        {
            do
            {
                while (IS_TRANSMIT_EMPTY(port) == 0);
                io_out8(port,*buf);
            } while (*buf++);
        }
        buf = level[*log + 2];
        basic_print(0x0000ff00,buf);
    }
    va_list ap;
    va_start(ap,log);
    vsprintf(msg,log,ap);
    buf = msg;
    while (IS_TRANSMIT_EMPTY(port) == 0);
    if(*buf != 0)
    {
        do
        {
            while (IS_TRANSMIT_EMPTY(port) == 0);
            io_out8(port,*buf);
        } while (*buf++);
    }
    buf = msg;
    basic_print(0x00ffffff,buf);
    return;
}
#undef IS_TRANSMIT_EMPTY
#endif

#endif