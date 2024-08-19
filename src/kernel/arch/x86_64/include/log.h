#ifndef __LOG_H__
#define __LOG_H__

#include <io.h>
#include <std/string.h>
#include <std/stdarg.h>
#include <std/stdio.h>

PUBLIC void basic_put_char(char c,uint32_t col);
PUBLIC void basic_print(uint32_t col,const char *str,...);

#if __DISABLE_LOG__
static inline void pr_log(const char *log,...){(void)log;}
#elif !__DISABLE_LOG__

#define IS_TRANSMIT_EMPTY(port) (io_in8(port + 5) & 0x20)

static inline void serial_pr_log(const char *log,va_list ap)
{
    char msg[256];
    char *buf;
    uint16_t port = 0x3f8;
    char *level[] =
    {
        "\033[32m[ INFO  ]\033[0m ",
        "\033[33m[ DEBUG ]\033[0m ",
        "\033[31m[ ERROR ]\033[0m ",
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
    }
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
    return;
}
#undef IS_TRANSMIT_EMPTY

extern volatile uint32_t global_log_cnt;
static inline void pr_log(const char *log,...)
{
    char msg[256];
    char *buf;
    char *level[] =
    {
        "[ INFO  ] ",
        "[ DEBUG ] ",
        "[ ERROR ] "
    };
    if (*log >= 1 && *log <= 3)
    {
        buf = level[*log - 1];
        if (*log - 1 == 0)
        {
            basic_print(0x0000ff00,buf);
        }
        else if (*log - 1 == 1)
        {
            basic_print(0x00ffcc00,buf);
        }
        else if (*log - 1 == 2)
        {
            basic_print(0x00ff0000,buf);
        }
        basic_print(0x00ffffff,"[%8d]",global_log_cnt);
        __asm__ __volatile__("lock incq %0":"=m"(global_log_cnt)::"memory");
        log = log + 1;
    }
    va_list ap;
    va_start(ap,log);
    vsprintf(msg,log,ap);
    buf = msg;
    basic_print(0x00ffffff,buf);

#if !__DISABLE_SERIAL_LOG__
    va_start(ap,log);
    serial_pr_log(log,ap);
#endif
    return;
}

#endif

PUBLIC void panic_spin(
    char* filename,
    int line,
    const char* func,
    const char* condition);

#define PANIC(...) panic_spin (__FILE__,__LINE__,__FUNCTION__,__VA_ARGS__)

#if __DISABLE_ASSERT__

#define ASSERT(X) ((void)0)

#else

#define ASSERT(X) \
    do \
    { \
        if (!(X)) { PANIC("ASSERT("#X")"); } \
    } while (0)

#endif

#endif