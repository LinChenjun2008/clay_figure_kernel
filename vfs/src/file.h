// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __VFS_FILE_H__
#define __VFS_FILE_H__

#include <lib/bitmap.h>
#include <types.h>

#define MAX_VFS_INODE 32768 // 最大inode总数
#define MAX_NAME_LEN  63    // 最长文件名
#define MAX_DIR_ENTRY 64    // 最多目录项个数

struct vfs_inode;

enum file_type
{
    FT_UNKNOW,
    FT_FILE,
    FT_DIRECTORY,
};

// 目录项
struct vfs_dentry
{
    char           file_name[MAX_NAME_LEN]; // 文件名
    uint32_t       i_no;                    // 对应文件的inode编号
    enum file_type type;                    // 文件类型(目录/文件)
};

struct vfs
{
    struct vfs_inode *inodes[MAX_VFS_INODE];
    struct bitmap     i_bitmap; // inodes位图
    struct vfs_dentry root_dir; // 根目录
};

int make_dentry(struct vfs_dentry *dir, const char *name, enum file_type type);
struct vfs_dentry *get_dentry(struct vfs_dentry *dir, const char *name);
int                vfs_mkdir_parent(const char *path);
int32_t            vfs_open(const char *path, int flags);
int vfs_write(int32_t i_no, const void *buf, off_t offset, size_t size);
int vfs_read(int32_t i_no, void *buf, off_t offset, size_t size);
int init_vfs(void);

#endif /* __VFS_FILE_H__ */