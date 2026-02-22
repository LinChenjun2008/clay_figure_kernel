// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2026 Lin Chenjun
 */

#ifndef __LIST_H__
#define __LIST_H__

#define OFFSET(CONTAINER_TYPE, MEMBER_NAME) \
    (uint64_t)(&((CONTAINER_TYPE *)0)->MEMBER_NAME)

#define CONTAINER_OF(CONTAINER_TYPE, MEMBER_NAME, MEMBER_PTR) \
    ((CONTAINER_TYPE *)((uintptr_t)MEMBER_PTR -               \
                        OFFSET(CONTAINER_TYPE, MEMBER_NAME)))

struct list_node;

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

void              init_list(struct list *list);
void              list_in(struct list_node *node, struct list_node *in_before);
void              list_push(struct list *list, struct list_node *node);
void              list_append(struct list *list, struct list_node *node);
void              list_remove(struct list_node *node);
struct list_node *list_pop(struct list *list);
int               list_find(struct list *list, struct list_node *objnode);
struct list_node *list_traversal(
    struct list *list,
    int (*func)(struct list_node *, uint64_t),
    uint64_t arg
);
int list_len(struct list *list);
int list_empty(struct list *list);

struct list_node *list_head(struct list *list);
struct list_node *list_tail(struct list *list);
struct list_node *list_next(struct list_node *node);
struct list_node *list_prev(struct list_node *node);

#endif /* __LIST_H__ */