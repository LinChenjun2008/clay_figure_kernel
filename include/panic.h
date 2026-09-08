// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __PANIC_H__
#define __PANIC_H__

#include <print/format.h>

#define PANIC(MESSAGE)                                               \
    do                                                               \
    {                                                                \
        panic_spin(__func__, __LINE__, MSG_HIGHLIGHT(MESSAGE) "\n"); \
    } while (0)

#define ASSERT(CONDITION)                                             \
    do                                                                \
    {                                                                 \
        if (!(CONDITION))                                             \
        {                                                             \
            panic_spin(                                               \
                __func__,                                             \
                __LINE__,                                             \
                "Assertion '" MSG_HIGHLIGHT(#CONDITION) "' failed.\n" \
            );                                                        \
        }                                                             \
    } while (0)

void panic_spin(const char *function, int line, const char *message);

#endif /* __PANIC_H__ */
