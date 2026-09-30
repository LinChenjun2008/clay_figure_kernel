// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __KOBJECT_H__
#define __KOBJECT_H__

#include <sync/spinlock.h>

struct kobject;

typedef void (*kobject_destroy_t)(struct kobject *obj);

struct kobject
{
    struct spinlock   lock;
    int32_t           ref_count;
    kobject_destroy_t destroy;
};

void init_kobject(struct kobject *obj, kobject_destroy_t destroy);
int  get_kobject(struct kobject *obj);
void put_kobject(struct kobject *obj);

#endif /* __KOBJECT_H__ */