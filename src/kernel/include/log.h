#ifndef __LOG_H__
#define __LOG_H__

#include <kernel/io.h>

#define IS_TRANSMIT_EMPTY(port) (io_in8(port + 5) & 0x20)
static inline void pr_log(const char* log)
{
    uint16_t port = 0x3f8;
    while (IS_TRANSMIT_EMPTY(port) == 0);
    if(*log != 0)
    {
        do
        {
            while (IS_TRANSMIT_EMPTY(port) == 0);
            io_out8(port,*log);
        } while (*log++);
    }
    return;
}

#endif