// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <ramfs.h>
#include <std/string.h>

PUBLIC status_t ramfs_check(void *ramfs_addr)
{
    ramfs_info_t *ramfs_info;
    ramfs_info = ramfs_addr;
    if (ramfs_info->magic != 0xffaaffaaffaaffaa)
    {
        return K_ERROR;
    }
    uint8_t                *data  = (uint8_t *)ramfs_addr + sizeof(*ramfs_info);
    ramfs_file_meta_data_t *fdata = (ramfs_file_meta_data_t *)data;
    size_t                  offset = 0;

    uint64_t i;
    for (i = 0; i < ramfs_info->files; i++)
    {
        pr_log(
            LOG_INFO,
            "File %d: name=%s, size=%d.\n",
            i + 1,
            fdata->file_name,
            fdata->file_size
        );
        data += offset;
        offset = sizeof(*fdata) + fdata->file_size;
        fdata  = (ramfs_file_meta_data_t *)(data + offset);
    }
    fdata = (ramfs_file_meta_data_t *)((uint8_t *)fdata - offset);
    if (!strncmp((char *)data, "TRAILER!!!", 10))
    {
        return K_ERROR;
    }
    return K_SUCCESS;
}

PUBLIC status_t
ramfs_open(void *ramfs_addr, const char *name, ramfs_file_t *file)
{
    if (name == NULL || strlen(name) > MAX_NAME_LEN)
    {
        return K_NOT_FOUND;
    }
    ramfs_info_t *ramfs_info;
    ramfs_info = ramfs_addr;
    ASSERT(ramfs_info->magic == 0xffaaffaaffaaffaa);
    uint8_t                *data  = (uint8_t *)ramfs_addr + sizeof(*ramfs_info);
    ramfs_file_meta_data_t *fdata = (ramfs_file_meta_data_t *)data;
    size_t                  offset;

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
            return K_SUCCESS;
        }
        offset = sizeof(*fdata) + fdata->file_size;
        fdata  = (ramfs_file_meta_data_t *)((uint8_t *)fdata + offset);
    }
    return K_NOT_FOUND;
}