// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <ramfs.h>
#include <ramfs/struct.h>
#include <std/string.h>

void *ramfs_read(void *fs, const char *filename)
{
    struct file_header *header = fs;

    char    *name = NULL;
    uint8_t *file = NULL;

    while (1)
    {
        name = (char *)(header + 1);
        file = (uint8_t *)name + header->name_len;
        file += ALIGN_PAD(header->name_len, 4);

        if (strncmp(name, "TRAILER!!!", header->name_len) == 0)
        {
            return NULL;
        }
        if (strncmp(name, filename, header->name_len) == 0)
        {
            return file;
        }
        file += header->file_size;
        file += ALIGN_PAD(header->file_size, 4);
        header = (void *)file;
    }
    return NULL;
}