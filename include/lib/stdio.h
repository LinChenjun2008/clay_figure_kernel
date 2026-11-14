// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIB_STDIO_H__
#define __LIB_STDIO_H__

#include <lib/stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...);

#endif /* __LIB_STDIO_H__ */
