// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025-2026 Lin Chenjun
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <ramfs.h>

void parse_config(struct ramfs_file *fp);

/**
 * @brief 读取配置信息
 * @param name 配置项名
 * @param value 配置项值
 * @param value_len 配置项值长度
 * @return
 */
void read_config(const char *name, char *value, size_t *value_len);

#endif