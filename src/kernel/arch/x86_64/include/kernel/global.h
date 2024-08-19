#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define __DISABLE_LOG__  0
#define __DISABLE_SERIAL_LOG__ 1 // available when __DISABLE_LOG__ == FALSE

#define __DISABLE_ASSERT__ 0

// #define __PIC_8259A__

// #define __TIMER_HPET__
#define __TIMER_8254__

#define ERROR(x) ((x) != K_SUCCESS)
#define K_ERROR          0xc0000000
#define K_SUCCESS        0x80000000

#include <kernel/def.h>
#include <kernel/const.h>

#include <common.h>

extern boot_info_t *g_boot_info;

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

#endif