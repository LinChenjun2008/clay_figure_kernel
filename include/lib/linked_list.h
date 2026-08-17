// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIB_LINKED_LIST_H__
#define __LIB_LINKED_LIST_H__

struct list_node
{
    struct list      *list;
    struct list_node *prev;
    struct list_node *next;
};

struct list
{
    struct list_node head;
    struct list_node tail;
    size_t           len;
};

typedef int (*list_traversal_func_t)(struct list_node *node, void *arg);

void init_list(struct list *list);
void list_insert(struct list_node *node, struct list_node *in_before);
void list_push(struct list *list, struct list_node *node);
void list_append(struct list *list, struct list_node *node);
void list_remove(struct list_node *node);
struct list_node *list_pop(struct list *list);
int               list_find(struct list *list, struct list_node *node);
struct list_node *
list_traversal(struct list *list, list_traversal_func_t func, void *arg);
struct list_node *
list_traversal_remove(struct list *list, list_traversal_func_t func, void *arg);
size_t            list_len(struct list *list);
int               list_empty(struct list *list);
struct list_node *list_head(struct list *list);
struct list_node *list_tail(struct list *list);
struct list_node *list_next(struct list_node *node);
struct list_node *list_prev(struct list_node *node);

#endif /* __LIB_LINKED_LIST_H__ */