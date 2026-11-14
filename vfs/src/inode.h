// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __VFS_INODE_H__
#define __VFS_INODE_H__

#include <types.h>

struct vfs_inode
{
    uint32_t i_no;         // inode 编号
    size_t   i_size;       // 文件大小
    uint32_t i_open_count; // 打开次数
    int      write_deny;   // 写标记
    void    *i_data;       // 普通文件:文件数据 目录:vfs_dir_entry数组
};

int32_t allocate_inode(void);
void    free_inode(int32_t i_no);

#endif /* __VFS_INODE_H__ */