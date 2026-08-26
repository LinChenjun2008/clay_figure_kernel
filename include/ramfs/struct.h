// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __RAMFS_STRUCT_H__
#define __RAMFS_STRUCT_H__

// header1 | name | file | header2 | name | file | ...

struct file_header
{
    uint32_t magic;
    uint32_t name_len;
    uint32_t file_size;
};

#endif /* __RAMFS_STRUCT_H__ */