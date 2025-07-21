// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#ifndef __RAMFS_H__
#define __RAMFS_H__

#define MAX_NAME_LEN 63

typedef struct ramfs_info_s
{
    uint64_t magic;
    uint64_t files;
} ramfs_info_t;

typedef struct ramfs_file_meta_data_s
{
    size_t  file_size;
    uint8_t file_name[MAX_NAME_LEN + 1];
} ramfs_file_meta_data_t;

typedef struct ramfs_file_s
{
    uint8_t name[MAX_NAME_LEN + 1];
    size_t  size;
    void   *data;
} ramfs_file_t;

#ifndef __RAMFS_TOOLS__

PUBLIC status_t ramfs_check(void *ramfs_addr);
PUBLIC status_t
ramfs_open(void *ramfs_addr, const char *name, ramfs_file_t *file);

#endif

#endif