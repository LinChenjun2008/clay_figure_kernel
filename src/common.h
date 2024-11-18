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

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __BOOTLOADER__
typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long int uint64_t;

typedef unsigned long long int addr_t;
#endif

#pragma pack(1)
typedef struct
{
    addr_t   frame_buffer_base;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixel_per_scanline;
} graph_info_t;

typedef struct
{
    uint64_t map_size;
    void    *buffer;
    uint64_t map_key;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
} memory_map_t;

typedef struct
{
    addr_t   base_address;
    uint64_t size;
    uint32_t flag;
} file_info_t;

typedef struct
{
    uint64_t     magic;               // 5a 42 cb 16 13 d4 a6 2f
    memory_map_t memory_map;          // 内存描述符
    graph_info_t graph_info;          // 图形信息
    uint8_t      loaded_files;        // 已加载的文件数
    file_info_t *loaded_file;         // 已加载的文件
    void        *madt_addr;
} boot_info_t;

#pragma pack()

#endif