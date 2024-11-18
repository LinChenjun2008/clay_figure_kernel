/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define K_NAME "Clay Figure Kernel"
#define K_VERSION "v0.0.0"

#define __DISABLE_SERIAL_LOG__

// #define __DISABLE_ASSERT__

// #define __PIC_8259A__

// #define __TIMER_HPET__

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