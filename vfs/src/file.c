// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "file.h"

#include "inode.h"
#include "path.h"
#include <errno.h>
#include <lib/string.h>
#include <user/lib.h>
#include <user/services/vfs.h>

extern struct vfs *fs;

// 初始化一个目录项
static int
init_dentry(struct vfs_dentry *dentry, const char *name, enum file_type type)
{
    strncpy(dentry->file_name, name, MAX_NAME_LEN);
    dentry->type = type;
    dentry->i_no = allocate_inode();
    return 0;
}

// 在dir下创建名为name的目录项并初始化
int make_dentry(struct vfs_dentry *dir, const char *name, enum file_type type)
{
    if (dir->type != FT_DIRECTORY)
    {
        return -ENOTDIR;
    }
    struct vfs_inode *inode = fs->inodes[dir->i_no];
    if (inode == NULL)
    {
        return -ENOENT;
    }

    // 已存在同名目录项: 不允许重复创建
    if (get_dentry(dir, name) != NULL)
    {
        return -EEXIST;
    }

    size_t entries = inode->i_size / sizeof(struct vfs_dentry);
    if (entries >= MAX_DIR_ENTRY)
    {
        return -EAGAIN;
    }

    struct vfs_dentry new_entry;
    memset(&new_entry, 0, sizeof(new_entry));
    int status = init_dentry(&new_entry, name, type);
    if (status < 0)
    {
        return status;
    }

    int offset = (off_t)(entries * sizeof(new_entry));
    status     = vfs_write(dir->i_no, &new_entry, offset, sizeof(new_entry));
    return status;
}

// 在dir下查找名为name的目录项, 命中返回目录数组里的那一项
struct vfs_dentry *get_dentry(struct vfs_dentry *dir, const char *name)
{
    struct vfs_inode *inode = fs->inodes[dir->i_no];
    if (inode == NULL)
    {
        return NULL;
    }

    size_t entries = inode->i_size / sizeof(struct vfs_dentry);
    size_t i;

    for (i = 0; i < entries; i++)
    {
        struct vfs_dentry entry;
        off_t             offset = (off_t)(i * sizeof(entry));
        int status = vfs_read(dir->i_no, &entry, offset, sizeof(entry));
        if (status < 0)
        {
            return NULL;
        }
        if (strcmp(entry.file_name, name) == 0)
        {
            return &((struct vfs_dentry *)inode->i_data)[i];
        }
    }
    return NULL;
}

// 创建path所指向文件的还未创建的目录(mkdir -p)
int vfs_mkdir_parent(const char *path)
{
    if (path[0] != '/')
    {
        return -EINVAL;
    }
    const char        *next = path;
    char               buf[MAX_NAME_LEN + 1];
    struct vfs_dentry *curr_dentry = &fs->root_dir;

    while ((next = path_next(next, buf)) != NULL)
    {
        struct vfs_dentry *dir = get_dentry(curr_dentry, buf);
        if (dir == NULL)
        {
            int status = make_dentry(curr_dentry, buf, FT_DIRECTORY);
            if (status < 0)
            {
                return status;
            }
            dir = get_dentry(curr_dentry, buf);
        }
        else if (dir->type != FT_DIRECTORY)
        {
            return -ENOTDIR;
        }
        curr_dentry = dir;
    }
    return 0;
}

// 打开一个文件,返回i_no
int32_t vfs_open(const char *path, int flags)
{
    if (path == NULL || path[0] != '/')
    {
        dprintf("vfs_open: path must start from root.\n");
        return -EINVAL;
    }
    int32_t i_no = 0;

    const char        *next = path;
    char               buf[MAX_NAME_LEN + 1];
    struct vfs_dentry *curr_dentry = &fs->root_dir;

    while ((next = path_next(next, buf)) != NULL)
    {
        curr_dentry = get_dentry(curr_dentry, buf);
        if (curr_dentry == NULL)
        {
            return -ENOENT;
        }
        if (curr_dentry->type != FT_DIRECTORY)
        {
            return -ENOTDIR;
        }
    }
    // buf为空说明是根目录
    if (buf[0] == '\0')
    {
        return fs->root_dir.i_no;
    }
    struct vfs_dentry *dentry = get_dentry(curr_dentry, buf);
    if (dentry == NULL)
    {
        if (!(flags & VFS_O_CREATE))
        {
            return -ENOENT;
        }
        int status = make_dentry(curr_dentry, buf, FT_FILE);
        if (status < 0)
        {
            return status;
        }
        dentry = get_dentry(curr_dentry, buf);
    }
    i_no = dentry->i_no;
    return i_no;
}

// 写入文件
int vfs_write(int32_t i_no, const void *buf, off_t offset, size_t size)
{
    if (i_no < 0)
    {
        return -EPERM;
    }
    struct vfs_inode *inode = fs->inodes[i_no];
    if (inode == NULL)
    {
        return -ENOENT;
    }
    if (inode->i_size < offset + size)
    {
        void *data = malloc(offset + size);
        if (data == NULL)
        {
            return -ENOMEM;
        }
        memcpy(data, inode->i_data, inode->i_size);
        free(inode->i_data);
        inode->i_data = data;
        inode->i_size = offset + size;
    }
    if (inode->i_data == NULL)
    {
        dprintf("vfs: error: i_data is NULL\n");
        return -EIO;
    }
    memcpy((uint8_t *)inode->i_data + offset, buf, size);

    return 0;
}

// 读取文件
int vfs_read(int32_t i_no, void *buf, off_t offset, size_t size)
{
    if (i_no < 0)
    {
        return -EPERM;
    }
    struct vfs_inode *inode = fs->inodes[i_no];
    if (inode == NULL)
    {
        return -ENOENT;
    }
    if (inode->i_data == NULL)
    {
        return -EIO;
    }
    if (inode->i_size < offset + size)
    {
        return -EIO;
    }
    memcpy(buf, (uint8_t *)inode->i_data + offset, size);
    return 0;
}

// 初始化vfs并创建根目录
int init_vfs()
{
    fs = malloc(sizeof(*fs));
    if (fs == NULL)
    {
        dprintf("Failed to init vfs!\n");
        return -ENOMEM;
    }
    size_t vfs_bitmap_size = MAX_VFS_INODE / 8;
    void  *map             = malloc(vfs_bitmap_size);
    if (map == NULL)
    {
        dprintf("Failed to allocate bitmap!\n");
        return -ENOMEM;
    }
    init_bitmap(&fs->i_bitmap, vfs_bitmap_size, map);

    // make root dir
    return init_dentry(&fs->root_dir, "/", FT_DIRECTORY);
}
