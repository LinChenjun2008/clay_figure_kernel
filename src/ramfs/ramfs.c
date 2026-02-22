// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <ramfs.h>
#include <std/string.h>

int ramfs_check(void *ramfs_addr)
{
    struct ramfs_info *ramfs_info;
    ramfs_info = ramfs_addr;
    if (ramfs_info->magic != 0xffaaffaaffaaffaa)
    {
        return -ERROR;
    }
    uintptr_t                    data;
    struct ramfs_file_meta_data *fdata = NULL;

    data          = (uintptr_t)ramfs_addr + sizeof(*ramfs_info);
    fdata         = (struct ramfs_file_meta_data *)data;
    size_t offset = 0;

    uint64_t i;
    for (i = 0; i < ramfs_info->files; i++)
    {
        data += offset;
        offset = sizeof(*fdata) + fdata->file_size;
        fdata  = (struct ramfs_file_meta_data *)(data + offset);
    }
    if (!strncmp((char *)data, "TRAILER!!!", 10))
    {
        return -ERROR;
    }
    return 0;
}

int ramfs_open(void *ramfs_addr, const char *name, struct ramfs_file *file)
{
    if (name == NULL || strlen(name) > MAX_NAME_LEN)
    {
        return -ENOTFOUND;
    }
    struct ramfs_info *ramfs_info;
    ramfs_info = ramfs_addr;
    if (ramfs_info->magic != 0xffaaffaaffaaffaa)
    {
        return -ERROR;
    }
    uintptr_t                    data;
    struct ramfs_file_meta_data *fdata = NULL;

    data  = (uintptr_t)ramfs_addr + sizeof(*ramfs_info);
    fdata = (struct ramfs_file_meta_data *)data;
    size_t offset;

    uint64_t i;
    for (i = 0; i < ramfs_info->files; i++)
    {
        if (strcmp((const char *)fdata->file_name, name) == 0)
        {
            if (file != NULL)
            {
                memcpy(file->name, fdata->file_name, MAX_NAME_LEN);
                file->size = fdata->file_size;
                file->data = (uint8_t *)fdata + sizeof(*fdata);
            }
            return 0;
        }
        offset = sizeof(*fdata) + fdata->file_size;
        fdata  = (struct ramfs_file_meta_data *)((uintptr_t)fdata + offset);
    }
    return -ENOTFOUND;
}