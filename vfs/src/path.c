// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include "path.h"

#include "file.h"
#include <lib/string.h>
#include <user/lib.h>

void path_normalize(char *path)
{
    if (path == NULL || *path == '\0') return;

    char *w = path; // 写指针: 最终结果写到这里
    char *r = path; // 读指针: 原串扫描位置

    while (*r)
    {
        if (*r == '/')
        {
            // 只有写指针前一个字符不是 '/' 时才写一个 '/'
            if (w == path || *(w - 1) != '/') *w++ = '/';
            r++; // 跳过连续的 '/'
        }
        else
        {
            *w++ = *r++; // 普通字符照搬
        }
    }
    *w = '\0';
}

const char *path_next(const char *path, char *buf)
{
    if (path == NULL || buf == NULL)
    {
        if (buf) buf[0] = '\0';
        return NULL;
    }

    /* 1) 跳过前导的连续 '/' */
    while (*path == '/') path++;

    /* 2) 已到末尾: 没有段可提 */
    if (*path == '\0')
    {
        buf[0] = '\0';
        return NULL;
    }

    const char *slash   = strchr(path, '/');
    size_t      raw_len = slash ? (size_t)(slash - path) : strlen(path);

    size_t copy_len = raw_len > MAX_NAME_LEN ? MAX_NAME_LEN : raw_len;
    memcpy(buf, path, copy_len);
    buf[copy_len] = '\0';

    const char *next = path + raw_len;
    while (*next == '/') next++;

    return (*next == '\0') ? NULL : next;
}