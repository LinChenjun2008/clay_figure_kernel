// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#ifndef __READ_ELF_H__
#define __READ_ELF_H__

PUBLIC int check_elf(void *data);

PUBLIC void elf_pt_load_addr(ramfs_file_t *fp, void *addr_lo, void *addr_hi);

/**
 * @brief 计算ELF文件PT_LOAD段在内存中占用的总大小
 * @param fp
 * @return
 */
PUBLIC size_t elf_load_size(ramfs_file_t *fp);

/**
 * @brief 将ELF文件的PT_LOAD段加载到内存中
 * @param fp
 * @param addr 将PT_LOAD段加载到addr处
 * @return
 */
PUBLIC void elf_load_segment(ramfs_file_t *fp, uintptr_t addr);

#endif