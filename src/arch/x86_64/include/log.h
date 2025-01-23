/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __LOG_H__
#define __LOG_H__

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