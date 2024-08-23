#ifndef __LOG_H__
#define __LOG_H__

#include <io.h>
#include <std/string.h>
#include <std/stdarg.h>
#include <std/stdio.h>

PUBLIC void basic_put_char(unsigned char c,uint32_t col);
PUBLIC void basic_print(uint32_t col,const char *str,...);

#ifdef __DISABLE_LOG__
#define pr_log(...) (void)0
#if defined __DISABLE_SERIAL_LOG__
#define serial_pr_log(...) (void)0
#endif /* __DISABLE_SERIAL_LOG__ */

#else

PUBLIC void pr_log(const char* str,...);

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

#endif /* __DISABLE_ASSERT__ */

#endif /* __DISABLE_LOG__ */

#endif