/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define K_NAME    "Clay Figure Kernel"
#define K_VERSION "v0.0.0"

#define __DISABLE_SERIAL_LOG__

// #define __DISABLE_ASSERT__

// #define __DISABLE_APIC_TIMER__
// #define __TIMER_HPET__

#define ERROR(x)          ((x) != K_SUCCESS)
#define K_ERROR           0xc0000000
#define K_UDF_BEHAVIOR    0xc0000001
#define K_NOMEM           0xc0000002
#define K_NOSUPPORT       0xc0000003
#define K_HW_NOSUPPORT    0xc0000004
#define K_DEADLOCK        0xc0000005
#define K_INVAILD_PARAM   0xc0000006
#define K_INVAILD_ADDR    0xc0000007
#define K_OUT_OF_RESOURCE 0xc0000008
#define K_NOT_FOUND       0xc0000009
#define K_TIMEOUT         0xc000000a
#define K_SUCCESS         0x80000000

#include <kernel/def.h>
#include <kernel/const.h>

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