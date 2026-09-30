// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <kobject.h>
#include <sync/spinlock.h>

void init_kobject(struct kobject *obj, kobject_destroy_t destroy)
{
    init_spinlock(&obj->lock);
    obj->ref_count = 1;
    obj->destroy   = destroy;
    return;
}

int get_kobject(struct kobject *obj)
{
    spin_lock(&obj->lock);
    int ref = obj->ref_count;
    if (obj->ref_count > 0)
    {
        obj->ref_count++;
    }
    spin_unlock(&obj->lock);
    return ref;
}

void put_kobject(struct kobject *obj)
{
    spin_lock(&obj->lock);
    int ref = obj->ref_count;
    if (ref != 0)
    {
        obj->ref_count--;
    }
    spin_unlock(&obj->lock);
    if (ref == 1 && obj->destroy != NULL)
    {
        obj->destroy(obj);
    }
    return;
}