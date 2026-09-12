// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIB_BITMAP_H__
#define __LIB_BITMAP_H__

struct bitmap
{
    size_t   map_size;
    uint8_t *map;
};

void    init_bitmap(struct bitmap *bitmap, size_t size, void *map);
int     bitmap_scan_test(struct bitmap *bitmap, size_t bit_index);
ssize_t bitmap_find(struct bitmap *bitmap, int value, size_t count);
int bitmap_set(struct bitmap *bitmap, size_t bit_index, int value, size_t bits);

#endif /* __LIB_BITMAP_H__ */