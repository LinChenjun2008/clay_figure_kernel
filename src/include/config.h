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
 * @param key 键名
 * @param value 键值
 * @param value_len 键长
 * @return
 */
PUBLIC void read_config(const char *key, char *value, size_t *value_len);

#endif