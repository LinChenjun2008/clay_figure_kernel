// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025-2026 Lin Chenjun
 */

#include <base.h>

#include <config.h>
#include <std/string.h>

#define IS_SPACE(x) (x == ' ' || x == '\n' || x == '\r' || x == '\t')
#define IS_KEY_NAME(x) \
    ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || x == '_')

struct conf_item
{
    char   name[64];
    size_t name_len;
    char   val[64];
    size_t val_len;
};

static struct
{
    struct conf_item items[64];
    int              number_of_items;
} configures;

static void skip_space(uint8_t **src, uint8_t *end)
{
    while (IS_SPACE(**src) && *src < end)
    {
        (*src)++;
    }
    return;
}

static void skip_line(uint8_t **src, uint8_t *end)
{
    while (**src != '\n' && *src < end)
    {
        (*src)++;
    }
    return;
}

static int read_item(uint8_t **src, uint8_t *end, struct conf_item *item)
{
    item->name_len = 0;
    item->val_len  = 0;
    skip_space(src, end);
    // Name
    while (IS_KEY_NAME(**src) && *src < end && item->name_len < 63)
    {
        item->name[item->name_len++] = *(*src)++;
    }
    item->name[item->name_len] = 0;
    skip_space(src, end);
    if (*(*src)++ != '=') return -ERROR;
    skip_space(src, end);

    if (*(*src)++ != '[') return -ERROR;
    skip_space(src, end);

    while (**src != ']' && *src < end && item->val_len < 63)
    {
        item->val[item->val_len++] = *(*src)++;
    }
    item->val[item->val_len] = 0;
    (*src)++;

    return 0;
}

void parse_config(ramfs_file_t *fp)
{
    configures.number_of_items = 0;
    uint8_t *src;
    uint8_t *end = ((uint8_t *)fp->data + fp->size);
    for (src = (uint8_t *)fp->data; src < end; src++)
    {
        skip_space(&src, end);

        // END
        if (src >= end)
        {
            return;
        }
        // Comment
        if (*src == '#')
        {
            skip_line(&src, end);
            continue;
        }
        else
        {
            struct conf_item item;
            if (read_item(&src, end, &item) < 0)
            {
                skip_line(&src, end);
                continue;
            }
            configures.items[configures.number_of_items++] = item;
        }
    }
    return;
}

void read_config(const char *name, char *value, size_t *value_len)
{
    int i;
    for (i = 0; i < configures.number_of_items; i++)
    {
        struct conf_item *item = &configures.items[i];
        if (strcmp(name, item->name) == 0)
        {
            if (*value_len < item->val_len)
            {
                break;
            }
            strcpy((char *)value, item->val);
            *value_len = item->val_len;
            return;
        }
    }
    *value_len = 0;
    return;
}