#ifndef __LOG_H__
#define __LOG_H__

#include <io.h>
#include <std/string.h>
#include <std/stdarg.h>
#include <std/stdio.h>

#if __DISABLE_LOG__
static inline void pr_log(const char *log,...){;}
#else
#define IS_TRANSMIT_EMPTY(port) (io_in8(port + 5) & 0x20)

static inline void pr_log(const char *log,...)
{
    char msg[128];
    char *buf = msg;

    char *level[] =
    {
        "\033[32m[ INFO  ]\033[0m ",
        "\033[33m[ DEBUG ]\033[0m ",
        "\033[31m[ ERROR ]\033[0m "
    };
    if (*log >= 1 && *log <= 3)
    {
        strcpy(buf,level[*log - 1]);
        buf += strlen(level[*log - 1]);
    }
    va_list ap;
    va_start(ap,log);
    vsprintf(buf,log,ap);
    buf = msg;
    uint16_t port = 0x3f8;
    while (IS_TRANSMIT_EMPTY(port) == 0);
    if(*log != 0)
    {
        do
        {
            while (IS_TRANSMIT_EMPTY(port) == 0);
            io_out8(port,*buf);
        } while (*buf++);
    }
    return;
}
#undef IS_TRANSMIT_EMPTY
#endif

#endif