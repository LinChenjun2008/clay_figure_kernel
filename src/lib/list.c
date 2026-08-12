// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib/list.h>

void init_list(struct list *list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
    return;
}

void list_insert(struct list_node *node, struct list_node *in_before)
{
    in_before->prev->next = node;

    node->prev = in_before->prev;
    node->next = in_before;

    in_before->prev = node;
    return;
}

void list_push(struct list *list, struct list_node *node)
{
    list_insert(node, list->head.next);
    return;
}

void list_append(struct list *list, struct list_node *node)
{
    list_insert(node, &list->tail);
    return;
}

void list_remove(struct list_node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next       = NULL;
    node->prev       = NULL;
    return;
}

struct list_node *list_pop(struct list *list)
{
    struct list_node *node = list->head.next;
    list_remove(node);
    return node;
}

int list_find(struct list *list, struct list_node *objnode)
{
    struct list_node *node = list->head.next;

    int ret = FALSE;
    while (node != &(list->tail))
    {
        if (node == objnode)
        {
            ret = TRUE;
            break;
        }
        node = node->next;
    }
    return ret;
}

struct list_node *list_traversal(
    struct list *list,
    int (*func)(struct list_node *, uint64_t),
    uint64_t arg
)
{
    struct list_node *node = list_next(list_head(list));
    struct list_node *ret  = NULL;
    while (node != list_tail(list))
    {
        if (func(node, arg))
        {
            ret = node;
            break;
        }
        node = list_next(node);
    }
    return ret;
}

int list_len(struct list *list)
{
    struct list_node *node = list->head.next;

    int len = 0;
    while (node != &list->tail)
    {
        len++;
        node = node->next;
    }
    return len;
}

int list_empty(struct list *list)
{
    return list->head.next == &list->tail;
}

struct list_node *list_head(struct list *list)
{
    return &list->head;
}

struct list_node *list_tail(struct list *list)
{
    return &list->tail;
}

struct list_node *list_next(struct list_node *node)
{
    return node->next;
}

struct list_node *list_prev(struct list_node *node)
{
    return node->prev;
}