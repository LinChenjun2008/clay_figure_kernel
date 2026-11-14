// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "inode.h"

#include "file.h"
#include <errno.h>
#include <lib/bitmap.h>
#include <lib/string.h>
#include <user/lib.h>

extern struct vfs *fs;

// 分配一个inode
int32_t allocate_inode()
{
    int32_t i_no = bitmap_find(&fs->i_bitmap, 0, 1);
    if (i_no < 0)
    {
        return -EAGAIN;
    }
    struct vfs_inode *inode = malloc(sizeof(*inode));
    if (inode == NULL)
    {
        return -ENOMEM;
    }
    memset(inode, 0, sizeof(*inode));
    fs->inodes[i_no] = inode;
    bitmap_set(&fs->i_bitmap, i_no, 1, 1);

    inode->i_no         = i_no;
    inode->i_size       = 0;
    inode->i_open_count = 0;
    inode->write_deny   = 0;
    inode->i_data       = NULL;
    return i_no;
}

// 释放inode,以及i_data所占用的空间
void free_inode(int32_t i_no)
{
    if (i_no < 0 || fs->inodes[i_no] == NULL)
    {
        return;
    }
    struct vfs_inode *inode = fs->inodes[i_no];
    free(inode->i_data);
    free(inode);
    fs->inodes[i_no] = NULL;
    bitmap_set(&fs->i_bitmap, i_no, 0, 1);
    return;
}