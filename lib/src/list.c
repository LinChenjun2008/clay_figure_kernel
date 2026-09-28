// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <user/lib.h>
#include <user/list.h>

void lib_init_list(struct list *list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
    return;
}

void lib_list_push(struct list *list, struct list_node *node)
{
    if (!list || !node)
    {
        return;
    }
    node->next       = list->head.next;
    node->next->prev = node;
    node->prev       = &list->head;
    list->head.next  = node;
    return;
}

void lib_list_remove(struct list_node *node)
{
    if (!node || !node->prev || !node->next)
    {
        return;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next       = NULL;
    node->prev       = NULL;
    return;
}

struct list_node *lib_list_pop(struct list *list)
{
    if (!list)
    {
        return NULL;
    }
    if (list->head.next == &list->tail)
    {
        return NULL;
    }
    struct list_node *node = list->head.next;
    lib_list_remove(node);
    return node;
}