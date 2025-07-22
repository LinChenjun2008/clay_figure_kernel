// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2025 LinChenjun
 */

#include <kernel/global.h>

#include <log.h>

#include <config.h>
#include <io.h>
#include <ramfs.h>
#include <std/string.h>

#define IS_SPACE(x) (x == ' ' || x == '\n' || x == '\r' || x == '\t')
#define IS_KEY_NAME(x) \
    ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || x == '_')

typedef struct key_item_s
{
    char   key[64];
    size_t key_len;
    char   val[64];
    size_t val_len;
} key_item_t;

PRIVATE struct
{
    key_item_t items[64];
    int        number_of_items;
} configures;

PRIVATE void skip_space(uint8_t **s, uint8_t *end)
{
    while (IS_SPACE(**s) && *s < end)
    {
        (*s)++;
    }
    return;
}

PRIVATE void skip_line(uint8_t **s, uint8_t *end)
{
    while (**s != '\n' && *s < end)
    {
        (*s)++;
    }
    return;
}

PRIVATE status_t read_item(uint8_t **s, uint8_t *end, key_item_t *item)
{
    item->key_len = 0;
    item->val_len = 0;
    skip_space(s, end);
    // Key name
    while (IS_KEY_NAME(**s) && **s != ':' && *s < end && item->key_len < 63)
    {
        item->key[item->key_len++] = *(*s)++;
    }
    item->key[item->key_len] = 0;

    if (*(*s)++ != ':') return K_ERROR;
    skip_space(s, end);

    if (*(*s)++ != '[') return K_ERROR;
    skip_space(s, end);

    while (**s != ']' && *s < end && item->val_len < 63)
    {
        item->val[item->val_len++] = *(*s)++;
    }
    item->val[item->val_len] = 0;
    (*s)++;

    return K_SUCCESS;
}

PUBLIC void parse_config(ramfs_file_t *fp)
{
    configures.number_of_items = 0;
    uint8_t *s;
    uint8_t *end = ((uint8_t *)fp->data + fp->size);
    for (s = (uint8_t *)fp->data; s < end; s++)
    {
        skip_space(&s, end);

        // END
        if (s >= end)
        {
            return;
        }
        // Comment
        if (*s == '#')
        {
            skip_line(&s, end);
            continue;
        }
        else
        {
            key_item_t item;
            if (ERROR(read_item(&s, end, &item)))
            {
                skip_line(&s, end);
                continue;
            }
            configures.items[configures.number_of_items++] = item;
            pr_log(0, "key=%s,value=%s.\n", item.key, item.val);
        }
    }
}

PUBLIC void read_config(const char *key, char *value, size_t *value_len)
{
    int i;
    for (i = 0; i < configures.number_of_items; i++)
    {
        if (strcmp(key, configures.items[i].key) == 0)
        {
            if (*value_len < configures.items[i].val_len)
            {
                break;
            }
            strcpy((char *)value, configures.items[i].val);
            *value_len = configures.items[i].val_len;
            return;
        }
    }
    *value_len = 0;
    return;
}