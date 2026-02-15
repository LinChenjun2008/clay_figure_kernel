// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib/list.h>

void init_list(list_t *list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
    return;
}

void list_in(list_node_t *node, list_node_t *in_before)
{
    in_before->prev->next = node;

    node->prev = in_before->prev;
    node->next = in_before;

    in_before->prev = node;
    return;
}

void list_push(list_t *list, list_node_t *node)
{
    list_in(node, list->head.next);
    return;
}

void list_append(list_t *list, list_node_t *node)
{
    list_in(node, &list->tail);
    return;
}

void list_remove(list_node_t *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next       = NULL;
    node->prev       = NULL;
    return;
}

list_node_t *list_pop(list_t *list)
{
    list_node_t *node = list->head.next;
    list_remove(node);
    return node;
}

int list_find(list_t *list, list_node_t *objnode)
{
    list_node_t *node = list->head.next;

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

list_node_t *
list_traversal(list_t *list, int (*func)(list_node_t *, uint64_t), uint64_t arg)
{
    list_node_t *node = list_next(list_head(list));
    list_node_t *ret  = NULL;
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

int list_len(list_t *list)
{
    list_node_t *node = list->head.next;

    int len = 0;
    while (node != &list->tail)
    {
        len++;
        node = node->next;
    }
    return len;
}

int list_empty(list_t *list)
{
    return list->head.next == &list->tail;
}

list_node_t *list_head(list_t *list)
{
    return &list->head;
}

list_node_t *list_tail(list_t *list)
{
    return &list->tail;
}

list_node_t *list_next(list_node_t *node)
{
    return node->next;
}

list_node_t *list_prev(list_node_t *node)
{
    return node->prev;
}