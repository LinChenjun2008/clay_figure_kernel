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

typedef struct list_node_s list_node_t;

struct list_node_s
{
    list_node_t *prev;
    list_node_t *next;
};

typedef struct
{
    list_node_t head;
    list_node_t tail;
} list_t;

void         init_list(list_t *list);
void         list_in(list_node_t *node, list_node_t *in_before);
void         list_push(list_t *list, list_node_t *node);
void         list_append(list_t *list, list_node_t *node);
void         list_remove(list_node_t *node);
list_node_t *list_pop(list_t *list);
int          list_find(list_t *list, list_node_t *objnode);
list_node_t *list_traversal(
    list_t *list,
    int (*func)(list_node_t *, uint64_t),
    uint64_t arg
);
int list_len(list_t *list);
int list_empty(list_t *list);

list_node_t *list_head(list_t *list);
list_node_t *list_tail(list_t *list);
list_node_t *list_next(list_node_t *node);
list_node_t *list_prev(list_node_t *node);

#endif /* __LIST_H__ */