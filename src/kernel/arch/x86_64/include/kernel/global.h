#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define __DISABLE_APIC__ 1
#define __DISABLE_LOG__  0
#define __DISABLE_SERIAL_LOG__ 1 // available when __DISABLE_LOG__ == FALSE

// #define __TIMER_HPET__
#define __TIMER_8254__

#define DEFAULT_PRIORITY 3

#include <kernel/def.h>
#include <kernel/const.h>

#include <common.h>

extern boot_info_t *g_boot_info;

#endif