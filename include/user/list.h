// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __USER_LIST_H__
#define __USER_LIST_H__

struct list_node
{
    struct list_node *prev;
    struct list_node *next;
};

struct list
{
    struct list_node head;
    struct list_node tail;
};

void lib_init_list(struct list *list);

void              lib_list_push(struct list *list, struct list_node *node);
struct list_node *lib_list_pop(struct list *list);
void              lib_list_remove(struct list_node *node);

#endif /* __USER_LIST_H__ */