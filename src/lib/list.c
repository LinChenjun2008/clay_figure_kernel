/*
   Copyright 2024 LinChenjun

 * 本文件是Clay Figure Kernel的一部分。
 * 修改和/或分发遵循GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <lib/list.h>
#include <intr.h> // intr functions

PUBLIC void list_init(list_t *list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
    return;
}

PUBLIC void list_in(list_node_t *node,list_node_t *in_before)
{
    intr_status_t intr_status= intr_disable();

    in_before->prev->next = node;

    node->prev = in_before->prev;
    node->next = in_before;

    in_before->prev = node;

    intr_set_status(intr_status);
    return;
}

PUBLIC void list_push(list_t *list,list_node_t *node)
{
    list_in(node,list->head.next);
    return;
}

PUBLIC void list_append(list_t *list,list_node_t *node)
{
    list_in(node,&list->tail);
    return;
}

PUBLIC void list_remove(list_node_t *node)
{
    intr_status_t intr_status = intr_disable();
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
    intr_set_status(intr_status);
    return;
}

PUBLIC list_node_t* list_pop(list_t *list)
{
    intr_status_t intr_status = intr_disable();
    list_node_t *node = list->head.next;
    list_remove(node);
    intr_set_status(intr_status);
    return node;
}

PUBLIC bool list_find(list_t *list,list_node_t *objnode)
{
    intr_status_t intr_status = intr_disable();
    list_node_t *node = list->head.next;
    bool res = FALSE;
    while (node != &(list->tail))
    {
        if (node == objnode)
        {
            res = TRUE;
            break;
        }
        node = node->next;
    }
    intr_set_status(intr_status);
    return res;
}

PUBLIC list_node_t* list_traversal(list_t *list,func function,int arg)
{
    intr_status_t intr_status = intr_disable();
    list_node_t *node = list->head.next;
    list_node_t *res = NULL;
    while (node != &list->tail)
    {
        if (function(node,arg))
        {
            res = node;
            break;
        }
        node = node->next;
    }
    intr_set_status(intr_status);
    return res;
}

PUBLIC int list_len(list_t *list)
{
    intr_status_t intr_status = intr_disable();
    list_node_t *node = list->head.next;
    int len = 0;
    while (node != &list->tail)
    {
        len++;
        node = node->next;
    }
    intr_set_status(intr_status);
    return len;
}

PUBLIC bool list_empty(list_t *list)
{
    return list->head.next == &list->tail;
}

PUBLIC list_node_t* list_next(list_node_t *node)
{
    return node->next;
}

PUBLIC list_node_t* list_prev(list_node_t *node)
{
    return node->prev;
}