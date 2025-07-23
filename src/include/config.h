// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <ramfs.h>

PUBLIC void parse_config(ramfs_file_t *fp);

/**
 * @brief 读取配置信息
 * @param name 配置项名
 * @param value 配置项值
 * @param value_len 配置项值长度
 * @return
 */
PUBLIC void read_config(const char *name, char *value, size_t *value_len);

#endif