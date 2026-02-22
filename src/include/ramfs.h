// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 Lin Chenjun
 */

#ifndef __RAMFS_H__
#define __RAMFS_H__

#define MAX_NAME_LEN 63

struct ramfs_info
{
    uint64_t magic;
    uint64_t files;
};

struct ramfs_file_meta_data
{
    size_t  file_size;
    uint8_t file_name[MAX_NAME_LEN + 1];
};

struct ramfs_file
{
    uint8_t name[MAX_NAME_LEN + 1];
    size_t  size;
    void   *data;
};

#ifndef __RAMFS_TOOLS__

int ramfs_check(void *ramfs_addr);
int ramfs_open(void *ramfs_addr, const char *name, struct ramfs_file *file);

#endif

#endif