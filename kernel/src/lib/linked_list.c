// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#include <base.h>

#include <lib/linked_list.h>

void init_list(struct list *list)
{
    list->head.list = list;
    list->head.prev = NULL;
    list->head.next = &list->tail;

    list->tail.list = list;
    list->tail.prev = &list->head;
    list->tail.next = NULL;

    list->len = 0;
    return;
}

void list_insert(struct list_node *node, struct list_node *in_before)
{
    in_before->prev->next = node;

    node->prev = in_before->prev;
    node->next = in_before;

    in_before->prev = node;

    node->list = in_before->list;
    node->list->len++;
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

    node->list->len--;
    node->list = NULL;
    return;
}

struct list_node *list_pop(struct list *list)
{
    if (list_empty(list))
    {
        return NULL;
    }
    struct list_node *node = list->head.next;
    list_remove(node);
    return node;
}

int list_find(struct list *list, struct list_node *node)
{
    return node->list == list;
}

struct list_node *
list_traversal(struct list *list, list_traversal_func_t func, void *arg)
{
    struct list_node *temp_node = list->head.next;
    struct list_node *temp_next = temp_node->next;
    struct list_node *ret       = NULL;
    size_t            i;
    for (i = 0; i < list->len; i++)
    {
        temp_next = temp_node->next;
        if (func(temp_node, arg))
        {
            ret = temp_node;
            break;
        }
        temp_node = temp_next;
    }
    return ret;
}

struct list_node *
list_traversal_remove(struct list *list, list_traversal_func_t func, void *arg)
{
    struct list_node *ret = list_traversal(list, func, arg);
    if (ret != NULL)
    {
        list_remove(ret);
    }
    return ret;
}

size_t list_len(struct list *list)
{
    return list->len;
}

int list_empty(struct list *list)
{
    return list->len == 0;
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