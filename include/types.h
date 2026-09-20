// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

/// @file: types.h
//  汇总了内核与用户程序通用的类型

#ifndef __TYPES_H__
#define __TYPES_H__

#include <asm/types.h>

typedef int pid_t;

typedef unsigned char char8_t;
typedef unsigned short char16_t;

typedef uint32_t cap_handle_t;

// other types
#include <types/message.h>
#include <types/signal.h>

#endif