// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright 2024-2025 LinChenjun
 */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define K_NAME    "Clay Figure Kernel"
#define K_VERSION "v0.0.0"

// #define __DISABLE_SERIAL_LOG__

// #define __DISABLE_ASSERT__

// #define __DISABLE_APIC_TIMER__
// #define __TIMER_HPET__

#define K_SUCCESS         0
#define K_ERROR           1
#define K_NOMEM           2
#define K_NOSUPPORT       3
#define K_HW_NOSUPPORT    4
#define K_DEADLOCK        5
#define K_INVAILD_PARAM   6
#define K_INVAILD_ADDR    7
#define K_OUT_OF_RESOURCE 8
#define K_NOT_FOUND       9
#define K_TIMEOUT         10

#define ERROR(x) ((x) != K_SUCCESS)

#include <kernel/const.h>
#include <kernel/def.h>

#include <common.h>

extern boot_info_t  *g_boot_info;
extern graph_info_t *g_graph_info;

extern char _kernel_start[];
extern char _text[];
extern char _etext[];
extern char _data[];
extern char _edata[];
extern char _rodata[];
extern char _erodata[];
extern char _bss[];
extern char _ebss[];
extern char _kernel_end[];

PUBLIC void kernel_main(void);
PUBLIC void ap_kernel_main(void);

#endif